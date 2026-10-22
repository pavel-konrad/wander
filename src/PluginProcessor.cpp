#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>

namespace
{
juce::String valueText (const wander::ParamInfo& info, float v)
{
    using U = wander::Unit;
    switch (info.unit)
    {
        case U::Octave:    return juce::String (juce::roundToInt (v));
        case U::Percent:   return juce::String (v * 100, 1) + " %";
        case U::Cents:     return juce::String (v, 1) + " ct";
        case U::Level:     return v < 0.0001f ? juce::String ("-inf dB") : juce::String (20 * std::log10 (v), 1) + " dB";
        case U::Hz:        return v >= 1000 ? juce::String (v / 1000, 1) + " kHz" : v < 10 ? juce::String (v, 2) + " Hz" : juce::String (juce::roundToInt (v)) + " Hz";
        case U::Seconds:   return v < 1 ? juce::String (v * 1000, v < 0.01f ? 2 : 0) + " ms" : juce::String (v, 2) + " s";
        case U::Oct:       return juce::String (v, 2) + " oct";
        case U::Semitones: return juce::String (v, 1) + " st";
        case U::Db:        return juce::String (v, 1) + " dB";
        case U::None:
        case U::Choice:
        default:           return juce::String (v, 2);
    }
}

// A knob range that moves logarithmically for frequencies and times, linearly for the rest.
juce::NormalisableRange<float> rangeOf (const wander::ParamInfo& info)
{
    if (! info.log)
        return { info.min, info.max };
    const float lo = info.min, hi = info.max;
    return { lo, hi,
             [lo, hi] (float, float, float t) { return lo * std::pow (hi / lo, t); },
             [lo, hi] (float, float, float v) { return std::log (juce::jlimit (lo, hi, v) / lo) / std::log (hi / lo); },
             [lo, hi] (float, float, float v) { return juce::jlimit (lo, hi, v); } };
}
} // namespace

juce::AudioProcessorValueTreeState::ParameterLayout WanderProcessor::layout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout l;
    for (const auto& info : wander::params())
    {
        if (info.choices != nullptr)
            l.add (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { info.id, 1 }, info.name,
                                                                 juce::StringArray::fromTokens (info.choices, "|", ""), (int) info.def));
        else if (info.unit == wander::Unit::Octave)
            l.add (std::make_unique<juce::AudioParameterInt> (juce::ParameterID { info.id, 1 }, info.name, (int) info.min, (int) info.max, (int) info.def));
        else
            l.add (std::make_unique<juce::AudioParameterFloat> (
                juce::ParameterID { info.id, 1 }, info.name, rangeOf (info), info.def,
                juce::AudioParameterFloatAttributes().withStringFromValueFunction ([&info] (float v, int) { return valueText (info, v); })));
    }
    return l;
}

WanderProcessor::WanderProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      state (*this, nullptr, "Wander", layout())
{
    for (size_t i = 0; i < wander::numParams; ++i)
        raw[i] = state.getRawParameterValue (wander::params()[i].id);
    for (auto& v : voiceOn) v = false;
}

bool WanderProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    return out == juce::AudioChannelSet::stereo() || out == juce::AudioChannelSet::mono();
}

void WanderProcessor::prepareToPlay (double sampleRate, int)
{
    synth.prepare (sampleRate);
    synth.set (readSettings());
}

wander::Settings WanderProcessor::readSettings() const
{
    std::array<float, wander::numParams> v {};
    for (size_t i = 0; i < wander::numParams; ++i)
        v[i] = raw[i]->load();
    return wander::toSettings (v);
}

void WanderProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    synth.set (readSettings());
    buffer.clear();

    const int n = buffer.getNumSamples();
    int pos = 0, w = scopeWrite.load();
    auto renderTo = [&] (int end)
    {
        for (; pos < end; ++pos)
        {
            const float s = synth.process();
            for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
                buffer.setSample (ch, pos, s);
            scope[(size_t) w] = s;
            w = (w + 1) % scopeSize;
        }
    };

    // notes land on the sample they are timed at
    for (const auto meta : midi)
    {
        renderTo (juce::jmin (meta.samplePosition, n));
        const auto m = meta.getMessage();
        if (m.isNoteOn()) synth.noteOn (m.getNoteNumber(), m.getFloatVelocity());
        else if (m.isNoteOff()) synth.noteOff (m.getNoteNumber());
        else if (m.isAllNotesOff() || m.isAllSoundOff()) synth.allNotesOff();
    }
    renderTo (n);
    scopeWrite = w;
    for (int v = 0; v < wander::numVoices; ++v)
        voiceOn[(size_t) v] = synth.voiceSounding (v);
}

void WanderProcessor::setCurrentProgram (int index)
{
    if (index < 0 || index >= getNumPrograms()) return;
    currentProgram = index;
    const auto& values = wander::presets()[(size_t) index].values;
    for (size_t i = 0; i < wander::numParams; ++i)
        if (auto* p = state.getParameter (wander::params()[i].id))
            p->setValueNotifyingHost (p->convertTo0to1 (values[i]));
}

const juce::String WanderProcessor::getProgramName (int index)
{
    return index >= 0 && index < getNumPrograms() ? juce::String (wander::presets()[(size_t) index].name) : juce::String();
}

void WanderProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    auto tree = state.copyState();
    tree.setProperty ("program", currentProgram, nullptr);
    if (auto xml = tree.createXml())
        copyXmlToBinary (*xml, dest);
}

void WanderProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
        if (xml->hasTagName (state.state.getType()))
        {
            auto tree = juce::ValueTree::fromXml (*xml);
            currentProgram = tree.getProperty ("program", 0);
            state.replaceState (tree);
        }
}

juce::AudioProcessorEditor* WanderProcessor::createEditor() { return new WanderEditor (*this); }

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new WanderProcessor(); }
