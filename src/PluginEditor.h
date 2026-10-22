// Wander's editor, laid out like Ableton's Drift: a header bar, then Osc / Osc Mix / Filter / Envelopes /
// LFO / global columns, drawn on a 1060 x 250 layout and scaled to the window.
#pragma once
#include "PluginProcessor.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_utils/juce_audio_utils.h>

namespace look
{
enum Accent { grey = 0, orange = 1, blue = 2, pink = 3 };
juce::Colour accent (int a);
juce::Font font (float size, bool bold = false);
}

class WanderLook final : public juce::LookAndFeel_V4
{
public:
    WanderLook();
    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos, float start, float end, juce::Slider&) override;
    void drawLinearSlider (juce::Graphics&, int x, int y, int w, int h, float pos, float min, float max,
                           juce::Slider::SliderStyle, juce::Slider&) override;
};

// A knob with its label above and its value below, in one column colour.
class Knob final : public juce::Component
{
public:
    Knob (WanderProcessor&, const char* paramId, const juce::String& label, int accent);
    void paint (juce::Graphics&) override;
    void resized() override;
    juce::Slider slider;

private:
    juce::RangedAudioParameter* param;
    juce::String label;
    int accent;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
};

// A value field filled to its value, like Drift's percentage boxes: drag up or down.
class ValueBox final : public juce::Component
{
public:
    ValueBox (WanderProcessor&, const char* paramId, const juce::String& label, int accent);
    void paint (juce::Graphics&) override;
    void resized() override;
    juce::Slider slider;

private:
    juce::RangedAudioParameter* param;
    juce::String label;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
};

// A choice (wave, filter type, LFO shape): click for a menu. Oscillator waves draw as an icon.
class Choice final : public juce::Component
{
public:
    enum Style { waveIcon, filterBox, text };
    Choice (WanderProcessor&, const char* paramId, Style, int accent);
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;

private:
    juce::AudioParameterChoice* param;
    Style style;
    int accent;
};

class WanderEditor final : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit WanderEditor (WanderProcessor&);
    ~WanderEditor() override;
    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;

    static constexpr int baseWidth = 1060, baseHeight = 250;

private:
    class Content;
    void timerCallback() override;

    WanderLook lookAndFeel;
    std::unique_ptr<Content> content;
};
