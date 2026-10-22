#include "PluginEditor.h"
#include <cmath>

// ---------------------------------------------------------------- colours and type
namespace colour
{
const juce::Colour frame { 0xff161616 };
const juce::Colour panel { 0xff242424 };
const juce::Colour well { 0xff111111 };
const juce::Colour head { 0xffa5d6e8 };
const juce::Colour ink { 0xff191919 };
const juce::Colour label { 0xffbdbdbd };
const juce::Colour value { 0xffdbdbdb };
const juce::Colour track { 0xff4f4f4f };
}

juce::Colour look::accent (int a)
{
    switch (a)
    {
        case orange: return juce::Colour (0xfff4a340);
        case blue:   return juce::Colour (0xff5ab4d6);
        case pink:   return juce::Colour (0xffe86a7f);
        default:     return juce::Colour (0xff9a9a9a);
    }
}

juce::Font look::font (float size, bool bold)
{
    return juce::Font (juce::FontOptions ("Avenir Next", size, bold ? juce::Font::bold : juce::Font::plain));
}

namespace
{
constexpr float pi = juce::MathConstants<float>::pi;

juce::String textOf (juce::RangedAudioParameter* p) { return p->getCurrentValueAsText(); }

// one cycle of a shape into a box: osc waves (with pulse width) or LFO shapes
void drawShape (juce::Graphics& g, juce::Rectangle<float> r, int wave, bool lfo, float pulseWidth, float thickness)
{
    juce::Path path;
    const int n = juce::jmax (8, (int) r.getWidth());
    for (int i = 0; i <= n; ++i)
    {
        const float t = (float) i / (float) n;
        float v;
        if (lfo)
            v = wave == 0 ? std::sin (2 * pi * t) : wave == 1 ? 1 - 4 * std::abs (t - 0.5f) : wave == 2 ? 1 - 2 * t
              : wave == 3 ? (t < 0.5f ? 1.0f : -1.0f) : (float) ((int) (t * 6) * 3 % 5) / 2.0f - 1;
        else
            v = wave == 0 ? std::sin (2 * pi * t) : wave == 1 ? 1 - 4 * std::abs (t - 0.5f) : wave == 2 ? 2 * t - 1
              : (t < pulseWidth ? 1.0f : -1.0f);
        const juce::Point<float> pt (r.getX() + t * r.getWidth(), r.getCentreY() - v * r.getHeight() * 0.42f);
        if (i == 0) path.startNewSubPath (pt); else path.lineTo (pt);
    }
    g.strokePath (path, juce::PathStrokeType (thickness, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}
} // namespace

// ---------------------------------------------------------------- look and feel
WanderLook::WanderLook()
{
    setColour (juce::PopupMenu::backgroundColourId, colour::panel);
    setColour (juce::PopupMenu::textColourId, colour::value);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, look::accent (look::blue));
    setColour (juce::PopupMenu::highlightedTextColourId, colour::ink);
}

void WanderLook::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos, float start, float end, juce::Slider& s)
{
    // a thick track, the value arc in the column colour (from 0 on two-sided knobs), a needle from the centre
    const auto bounds = juce::Rectangle<int> (x, y, w, h).toFloat().reduced (3);
    const float r = juce::jmin (bounds.getWidth(), bounds.getHeight()) / 2 - 1.5f;
    const auto c = bounds.getCentre();
    const float angle = start + pos * (end - start);
    const auto accent = look::accent ((int) s.getProperties().getWithDefault ("accent", 0));

    juce::Path track;
    track.addCentredArc (c.x, c.y, r, r, 0, start, end, true);
    g.setColour (colour::track);
    g.strokePath (track, juce::PathStrokeType (3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::butt));

    const double lo = s.getMinimum(), hi = s.getMaximum();
    const float zero = (lo < 0 && hi > 0) ? start + (float) s.valueToProportionOfLength (0) * (end - start) : start;
    if (std::abs (angle - zero) > 0.01f)
    {
        juce::Path arc;
        arc.addCentredArc (c.x, c.y, r, r, 0, juce::jmin (zero, angle), juce::jmax (zero, angle), true);
        g.setColour (s.isMouseOverOrDragging() ? accent.brighter (0.25f) : accent);
        g.strokePath (arc, juce::PathStrokeType (3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::butt));
    }
    g.setColour (s.isMouseOverOrDragging() ? accent.brighter (0.25f) : accent);
    g.drawLine ({ c, c.getPointOnCircumference (r, angle) }, 2.0f);
}

void WanderLook::drawLinearSlider (juce::Graphics& g, int x, int y, int w, int h, float, float, float,
                                   juce::Slider::SliderStyle, juce::Slider& s)
{
    // the value box: a dark field filled to the value (from 0 on two-sided values)
    const auto r = juce::Rectangle<int> (x, y, w, h).toFloat();
    g.setColour (colour::well);
    g.fillRect (r);
    const float t = (float) s.valueToProportionOfLength (s.getValue());
    const float z = (s.getMinimum() < 0 && s.getMaximum() > 0) ? (float) s.valueToProportionOfLength (0) : 0.0f;
    const auto accent = look::accent ((int) s.getProperties().getWithDefault ("accent", 0));
    g.setColour (accent.withAlpha (s.isMouseOverOrDragging() ? 0.7f : 0.55f));
    g.fillRect (r.withX (r.getX() + juce::jmin (t, z) * r.getWidth()).withWidth (std::abs (t - z) * r.getWidth()));
}

// ---------------------------------------------------------------- knob
Knob::Knob (WanderProcessor& p, const char* paramId, const juce::String& l, int a)
    : param (p.state.getParameter (paramId)), label (l), accent (a)
{
    slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    slider.setRotaryParameters (-0.75f * pi, 0.75f * pi, true);
    slider.getProperties().set ("accent", a);
    slider.setMouseDragSensitivity (180);
    slider.setVelocityModeParameters (1.0, 1, 0.0, false, juce::ModifierKeys::shiftModifier);
    slider.onValueChange = [this] { repaint(); };
    addAndMakeVisible (slider);
    attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (p.state, paramId, slider);
    slider.setDoubleClickReturnValue (true, param->convertFrom0to1 (param->getDefaultValue()));
}

void Knob::resized() { slider.setBounds (getLocalBounds().withTrimmedTop (16).withTrimmedBottom (16)); }

void Knob::paint (juce::Graphics& g)
{
    g.setFont (look::font (12.5f));
    g.setColour (colour::label);
    g.drawText (label, getLocalBounds().removeFromTop (16), juce::Justification::centred);
    g.setFont (look::font (11.5f));
    g.setColour (accent == look::grey ? colour::value : look::accent (accent));
    g.drawText (textOf (param), getLocalBounds().removeFromBottom (16), juce::Justification::centred);
}

// ---------------------------------------------------------------- value box
ValueBox::ValueBox (WanderProcessor& p, const char* paramId, const juce::String& l, int a)
    : param (p.state.getParameter (paramId)), label (l)
{
    slider.setSliderStyle (juce::Slider::LinearBarVertical);
    slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    slider.getProperties().set ("accent", a);
    slider.setMouseDragSensitivity (180);
    slider.setVelocityModeParameters (1.0, 1, 0.0, false, juce::ModifierKeys::shiftModifier);
    slider.onValueChange = [this] { repaint(); };
    slider.setInterceptsMouseClicks (true, false);
    addAndMakeVisible (slider);
    attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (p.state, paramId, slider);
    slider.setDoubleClickReturnValue (true, param->convertFrom0to1 (param->getDefaultValue()));
}

void ValueBox::resized() { slider.setBounds (getLocalBounds().withTrimmedTop (16)); }

void ValueBox::paint (juce::Graphics& g)
{
    g.setFont (look::font (12.5f));
    g.setColour (colour::label);
    g.drawText (label, getLocalBounds().removeFromTop (16), juce::Justification::centredLeft);
}

// the number sits on top of the bar, so it is drawn by a component above the slider
namespace
{
class BoxText final : public juce::Component
{
public:
    explicit BoxText (juce::RangedAudioParameter* p) : param (p) { setInterceptsMouseClicks (false, false); }
    void paint (juce::Graphics& g) override
    {
        g.setFont (look::font (11.5f));
        g.setColour (colour::value);
        g.drawText (textOf (param), getLocalBounds(), juce::Justification::centred);
    }
    juce::RangedAudioParameter* param;
};
}

// ---------------------------------------------------------------- choice
Choice::Choice (WanderProcessor& p, const char* paramId, Style s, int a)
    : param (dynamic_cast<juce::AudioParameterChoice*> (p.state.getParameter (paramId))), style (s), accent (a)
{
    jassert (param != nullptr);
}

void Choice::paint (juce::Graphics& g)
{
    const auto r = getLocalBounds().toFloat();
    const int index = param->getIndex();
    const auto c = look::accent (accent);
    if (style == filterBox)
    {
        g.setColour (look::accent (look::orange));
        g.fillRect (r);
        g.setColour (colour::ink);
        g.setFont (look::font (12.5f, true));
        g.drawText (index == 0 ? "LP" : index == 1 ? "HP" : "BP", r, juce::Justification::centred);
        return;
    }
    juce::Path arrow;
    arrow.addTriangle (r.getRight() - 9, r.getCentreY() - 2, r.getRight() - 1, r.getCentreY() - 2, r.getRight() - 5, r.getCentreY() + 3);
    g.setColour (c);
    g.fillPath (arrow);
    if (style == waveIcon)
        drawShape (g, r.withTrimmedRight (14).reduced (1, 3), index, false, 0.5f, 1.6f);
    else
    {
        g.setFont (look::font (12.5f, true));
        g.drawText (param->getCurrentChoiceName(), r.withTrimmedRight (12), juce::Justification::centredLeft);
    }
}

void Choice::mouseDown (const juce::MouseEvent&)
{
    juce::PopupMenu menu;
    const auto names = param->choices;
    for (int i = 0; i < names.size(); ++i)
        menu.addItem (i + 1, names[i], true, i == param->getIndex());
    menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this), [this] (int picked)
    {
        if (picked <= 0) return;
        param->beginChangeGesture();
        param->setValueNotifyingHost (param->convertTo0to1 ((float) (picked - 1)));
        param->endChangeGesture();
        repaint();
    });
}

// ---------------------------------------------------------------- the content: every widget at its place
class WanderEditor::Content final : public juce::Component
{
public:
    explicit Content (WanderProcessor& p) : proc (p)
    {
        // Osc 1, Osc 2, Pitch Mod
        choice ("osc1_wave", Choice::waveIcon, look::orange, 56, 42, 40, 22);
        knob ("osc1_oct", "Oct", look::orange, 130, 58);
        knob ("osc1_shape", "Shape", look::grey, 195, 58);
        choice ("osc2_wave", Choice::waveIcon, look::blue, 56, 112, 40, 22);
        knob ("osc2_oct", "Oct", look::blue, 130, 128);
        knob ("osc2_detune", "Detune", look::blue, 205, 128);
        box ("env2_pitch", "Env 2", look::orange, 14, 206, 56);
        box ("lfo_pitch", "LFO", look::blue, 78, 206, 56);
        // Osc Mix
        knob ("osc1_level", "Osc 1", look::orange, 330, 58);
        knob ("osc2_level", "Osc 2", look::blue, 330, 128);
        knob ("noise_level", "Noise", look::pink, 330, 204);
        // Filter
        knob ("cutoff", "Freq", look::blue, 410, 60, 44);
        choice ("filter_type", Choice::filterBox, look::orange, 458, 42, 50, 20);
        box ("key_tracking", "Key", look::grey, 458, 96, 50);
        knob ("resonance", "Res", look::grey, 410, 136);
        box ("env2_cutoff", "Env 2", look::blue, 376, 206, 64);
        box ("lfo_cutoff", "LFO", look::blue, 446, 206, 64);
        // Envelopes: 1 = amp, 2 = mod
        knob ("amp_attack", "Attack", look::grey, 572, 58);
        knob ("amp_decay", "Decay", look::grey, 636, 58);
        knob ("amp_sustain", "Sustain", look::blue, 700, 58);
        knob ("amp_release", "Release", look::grey, 764, 58);
        knob ("mod_attack", "Attack", look::grey, 572, 204);
        knob ("mod_decay", "Decay", look::grey, 636, 204);
        knob ("mod_sustain", "Sustain", look::blue, 700, 204);
        knob ("mod_release", "Release", look::grey, 764, 204);
        // LFO
        knob ("lfo_rate", "Rate", look::grey, 878, 58);
        choice ("lfo_wave", Choice::text, look::orange, 834, 100, 88, 20);
        knob ("lfo_shape", "> Shape", look::blue, 878, 204);
        // global
        box ("vel_level", "Vel > Vol", look::blue, 946, 86, 100);
        box ("drift", "Drift", look::blue, 946, 126, 100);
        knob ("volume", "Volume", look::blue, 996, 184);
    }

    void paint (juce::Graphics& g) override
    {
        g.fillAll (colour::frame);

        // header
        g.setColour (colour::head);
        g.fillRect (0, 0, baseWidth, 22);
        g.setColour (look::accent (look::orange));
        g.fillEllipse (8, 5, 12, 12);
        g.setColour (colour::ink);
        g.drawEllipse (8, 5, 12, 12, 1);
        g.setFont (look::font (14, true));
        g.drawText ("Wander", 26, 0, 80, 22, juce::Justification::centredLeft);
        g.setFont (look::font (12.5f));
        g.drawText ("Osc Mix", 272, 0, 92, 22, juce::Justification::centred);
        g.drawText ("Filter", 368, 0, 150, 22, juce::Justification::centred);
        g.drawText ("Envelopes", 522, 0, 300, 22, juce::Justification::centred);
        g.drawText ("LFO", 826, 0, 104, 22, juce::Justification::centred);
        g.setFont (look::font (12.5f, true));
        g.drawText (presetName(), presetArea(), juce::Justification::centredLeft);
        // one dot per voice, lit while it sounds
        for (int v = 0; v < wander::numVoices; ++v)
        {
            g.setColour (proc.voiceOn[(size_t) v] ? look::accent (look::orange) : colour::ink.withAlpha (0.55f));
            g.fillEllipse (958.0f + (float) v * 11, 8, 6, 6);
        }

        // column panels
        panel (g, 6, 26, 262, 68); panel (g, 6, 96, 262, 70); panel (g, 6, 168, 262, 78);
        panel (g, 272, 26, 92, 220);
        panel (g, 368, 26, 150, 220);
        panel (g, 522, 26, 300, 68); well (g, 526, 96, 292, 70); panel (g, 522, 168, 300, 78);
        panel (g, 826, 26, 104, 68); well (g, 826, 96, 104, 70); panel (g, 826, 168, 104, 78);
        panel (g, 934, 26, 120, 220);

        g.setFont (look::font (12.5f));
        g.setColour (colour::label);
        g.drawText ("Osc 1", 14, 46, 40, 16, juce::Justification::centredLeft);
        g.drawText ("Osc 2", 14, 116, 40, 16, juce::Justification::centredLeft);
        g.drawText ("Pitch Mod", 14, 172, 100, 16, juce::Justification::centredLeft);
        g.drawText ("Freq Mod", 376, 172, 100, 16, juce::Justification::centredLeft);
        g.drawText ("1", 528, 30, 12, 16, juce::Justification::centredLeft);
        g.drawText ("2", 528, 172, 12, 16, juce::Justification::centredLeft);
        g.drawText ("Mode", 946, 28, 100, 16, juce::Justification::centredLeft);
        g.setColour (colour::value);
        g.drawText ("Poly  8", 946, 44, 100, 16, juce::Justification::centredLeft);
        square (g, 278, 46, "1", look::orange);
        square (g, 278, 116, "2", look::blue);
        square (g, 278, 192, "N", look::pink);

        // pitch mod display: the LFO shape that bends the pitch
        well (g, 144, 180, 116, 58);
        g.setColour (colour::value);
        drawShape (g, { 150, 186, 104, 46 }, (int) value ("lfo_wave"), true, 0.5f, 1.4f);

        // envelope display: both, the selected one bright, tabs 1 and 2
        drawEnvelope (g, envSelected == 1, "mod_");
        drawEnvelope (g, envSelected == 0, "amp_");
        tab (g, { 530, 100, 18, 22 }, "1", envSelected == 0);
        tab (g, { 530, 140, 18, 22 }, "2", envSelected == 1);

        // LFO display
        g.setColour (look::accent (look::blue));
        drawShape (g, { 832, 126, 92, 34 }, (int) value ("lfo_wave"), true, 0.5f, 1.8f);

        // scope: the output from a rising zero crossing, so a steady tone stands still
        well (g, 946, 222, 100, 20);
        const int end = proc.scopeWrite.load();
        int start = (end - 1400 + WanderProcessor::scopeSize) % WanderProcessor::scopeSize;
        for (int j = 0; j < 800; ++j)
        {
            const float a = proc.scope[(size_t) ((start + j) % WanderProcessor::scopeSize)];
            const float b = proc.scope[(size_t) ((start + j + 1) % WanderProcessor::scopeSize)];
            if (a <= 0 && b > 0) { start = (start + j) % WanderProcessor::scopeSize; break; }
        }
        juce::Path wave;
        for (int i = 0; i < 100; ++i)
        {
            const float s = proc.scope[(size_t) ((start + i * 3) % WanderProcessor::scopeSize)];
            const juce::Point<float> pt (946.0f + (float) i, 232.0f - s * 8.0f);
            if (i == 0) wave.startNewSubPath (pt); else wave.lineTo (pt);
        }
        g.setColour (look::accent (look::orange));
        g.strokePath (wave, juce::PathStrokeType (1.2f));
    }

    void mouseDown (const juce::MouseEvent& e) override
    {
        const auto p = e.getPosition();
        if (juce::Rectangle<int> (530, 100, 18, 22).contains (p)) { envSelected = 0; repaint(); }
        if (juce::Rectangle<int> (530, 140, 18, 22).contains (p)) { envSelected = 1; repaint(); }
        if (presetArea().contains (p))
        {
            juce::PopupMenu menu;
            for (int i = 0; i < proc.getNumPrograms(); ++i)
                menu.addItem (i + 1, proc.getProgramName (i), true, i == proc.getCurrentProgram());
            menu.showMenuAsync (juce::PopupMenu::Options().withTargetScreenArea (localAreaToGlobal (presetArea())),
                                [this] (int picked) { if (picked > 0) { proc.setCurrentProgram (picked - 1); repaint(); } });
        }
    }

    void refresh()
    {
        repaint();
        for (auto* t : boxTexts) t->repaint();
    }

private:
    juce::Rectangle<int> presetArea() const { return { 104, 0, 160, 22 }; }
    juce::String presetName() { return proc.getProgramName (proc.getCurrentProgram()) + "  ▾"; }
    float value (const char* id) const { return proc.state.getRawParameterValue (id)->load(); }

    void knob (const char* id, const juce::String& label, int accent, int cx, int cy, int size = 34)
    {
        auto* k = knobs.add (new Knob (proc, id, label, accent));
        k->setBounds (cx - 34, cy - size / 2 - 16, 68, size + 32);
        addAndMakeVisible (k);
    }
    void box (const char* id, const juce::String& label, int accent, int x, int y, int w)
    {
        auto* b = boxes.add (new ValueBox (proc, id, label, accent));
        b->setBounds (x, y - 16, w, 34);
        addAndMakeVisible (b);
        auto* t = boxTexts.add (new BoxText (proc.state.getParameter (id)));
        t->setBounds (x, y, w, 18);
        addAndMakeVisible (t);
        b->slider.onValueChange = [t] { t->repaint(); };
    }
    void choice (const char* id, Choice::Style style, int accent, int x, int y, int w, int h)
    {
        auto* c = choices.add (new Choice (proc, id, style, accent));
        c->setBounds (x, y, w, h);
        addAndMakeVisible (c);
        if (style == Choice::filterBox)
            typeLabelY = y - 16;
    }

    static void panel (juce::Graphics& g, int x, int y, int w, int h) { g.setColour (colour::panel); g.fillRect (x, y, w, h); }
    static void well (juce::Graphics& g, int x, int y, int w, int h) { g.setColour (colour::well); g.fillRect (x, y, w, h); }
    static void square (juce::Graphics& g, int x, int y, const juce::String& t, int accent)
    {
        g.setColour (look::accent (accent));
        g.fillRect (x, y, 18, 24);
        g.setColour (colour::ink);
        g.setFont (look::font (12.5f, true));
        g.drawText (t, x, y, 18, 24, juce::Justification::centred);
    }
    static void tab (juce::Graphics& g, juce::Rectangle<float> r, const juce::String& t, bool on)
    {
        g.setColour (look::accent (look::blue));
        if (on) g.fillRect (r); else g.drawRect (r, 1.0f);
        g.setColour (on ? colour::ink : look::accent (look::blue));
        g.setFont (look::font (12.5f, true));
        g.drawText (t, r, juce::Justification::centred);
    }

    void drawEnvelope (juce::Graphics& g, bool bright, const juce::String& prefix)
    {
        // attack, decay, a held sustain, release; times on a square-root scale; handles at the corners
        const float a = std::sqrt (value ((prefix + "attack").toRawUTF8()));
        const float d = std::sqrt (value ((prefix + "decay").toRawUTF8()));
        const float s = value ((prefix + "sustain").toRawUTF8());
        const float r = std::sqrt (value ((prefix + "release").toRawUTF8()));
        const float x = 556, y = 104, w = 254, h = 54, hold = 0.6f;
        const float scale = w / (a + d + hold + r);
        const float x1 = x + a * scale, x2 = x1 + d * scale, x3 = x2 + hold * scale;
        juce::Path p;
        p.startNewSubPath (x, y + h);
        p.lineTo (x1, y);
        p.lineTo (x2, y + h - s * h);
        p.lineTo (x3, y + h - s * h);
        p.lineTo (x + w, y + h);
        g.setColour (look::accent (look::blue).withAlpha (bright ? 1.0f : 0.3f));
        g.strokePath (p, juce::PathStrokeType (bright ? 1.8f : 1.2f));
        if (bright)
        {
            g.setColour (look::accent (look::orange));
            for (auto pt : { juce::Point<float> (x1, y), juce::Point<float> (x2, y + h - s * h), juce::Point<float> (x + w, y + h) })
                g.drawRect (juce::Rectangle<float> (7, 7).withCentre (pt), 1.2f);
        }
    }

    void paintOverChildren (juce::Graphics& g) override
    {
        g.setFont (look::font (12.5f));
        g.setColour (colour::label);
        g.drawText ("Type", 458, typeLabelY, 50, 16, juce::Justification::centredLeft);
    }

    WanderProcessor& proc;
    juce::OwnedArray<Knob> knobs;
    juce::OwnedArray<ValueBox> boxes;
    juce::OwnedArray<BoxText> boxTexts;
    juce::OwnedArray<Choice> choices;
    int typeLabelY = 26, envSelected = 0;
};

// ---------------------------------------------------------------- editor
WanderEditor::WanderEditor (WanderProcessor& p) : AudioProcessorEditor (p)
{
    setLookAndFeel (&lookAndFeel);
    content = std::make_unique<Content> (p);
    content->setSize (baseWidth, baseHeight);
    addAndMakeVisible (*content);
    setResizable (true, true);
    getConstrainer()->setFixedAspectRatio ((double) baseWidth / baseHeight);
    setResizeLimits (baseWidth * 3 / 4, baseHeight * 3 / 4, baseWidth * 2, baseHeight * 2);
    setSize (baseWidth * 5 / 4, baseHeight * 5 / 4);
    startTimerHz (30);
}

WanderEditor::~WanderEditor() { setLookAndFeel (nullptr); }

void WanderEditor::paint (juce::Graphics& g) { g.fillAll (colour::frame); }

void WanderEditor::resized()
{
    const float scale = (float) getWidth() / (float) baseWidth;
    content->setTransform (juce::AffineTransform::scale (scale));
}

void WanderEditor::mouseDown (const juce::MouseEvent&) {}

void WanderEditor::timerCallback() { content->refresh(); }
