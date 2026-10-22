#pragma once
#include "Params.h"
#include <juce_audio_processors/juce_audio_processors.h>
#include <atomic>

class WanderProcessor final : public juce::AudioProcessor
{
public:
    WanderProcessor();

    void prepareToPlay (double sampleRate, int blockSize) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Wander"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 8.0; }

    // presets are the host's programs
    int getNumPrograms() override { return (int) wander::presets().size(); }
    int getCurrentProgram() override { return currentProgram; }
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& dest) override;
    void setStateInformation (const void* data, int size) override;

    juce::AudioProcessorValueTreeState state;

    // for the GUI: the last output samples and which voices sound
    static constexpr int scopeSize = 4096;
    std::array<float, scopeSize> scope {};
    std::atomic<int> scopeWrite { 0 };
    std::array<std::atomic<bool>, wander::numVoices> voiceOn {};

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout layout();
    wander::Settings readSettings() const;

    wander::Synth synth;
    std::array<std::atomic<float>*, wander::numParams> raw {};
    int currentProgram = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WanderProcessor)
};
