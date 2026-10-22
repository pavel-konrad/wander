// Draws the editor to a PNG without a host, after playing a note so the scope and voice dots show:
//   wander_snapshot OUT.png [--preset N] [--scale S] [--note N] [--render OUT.wav]
#include "PluginEditor.h"
#include <juce_audio_formats/juce_audio_formats.h>

int main (int argc, char** argv)
{
    if (argc < 2)
    {
        std::fprintf (stderr, "usage: wander_snapshot OUT.png [--preset N] [--scale S] [--note N]\n");
        return 2;
    }
    juce::ScopedJuceInitialiser_GUI gui;
    int preset = 0, note = 57;
    float scale = 2;
    for (int i = 2; i + 1 < argc; i += 2)
    {
        const juce::String opt (argv[i]), value (argv[i + 1]);
        if (opt == "--preset") preset = value.getIntValue();
        else if (opt == "--scale") scale = value.getFloatValue();
        else if (opt == "--note") note = value.getIntValue();
    }

    WanderProcessor proc;
    proc.setPlayConfigDetails (0, 2, 48000, 512);
    proc.prepareToPlay (48000, 512);
    proc.setCurrentProgram (preset);

    // half a second of one held note
    juce::AudioBuffer<float> buffer (2, 512);
    for (int b = 0; b < 48; ++b)
    {
        juce::MidiBuffer midi;
        if (b == 0) midi.addEvent (juce::MidiMessage::noteOn (1, note, (juce::uint8) 100), 0);
        proc.processBlock (buffer, midi);
    }

    std::unique_ptr<juce::AudioProcessorEditor> editor (proc.createEditor());
    editor->setSize (WanderEditor::baseWidth, WanderEditor::baseHeight);
    const auto image = editor->createComponentSnapshot (editor->getLocalBounds(), true, scale);
    juce::File out (juce::File::getCurrentWorkingDirectory().getChildFile (argv[1]));
    out.deleteFile();
    juce::FileOutputStream stream (out);
    juce::PNGImageFormat png;
    if (! stream.openedOk() || ! png.writeImageToStream (image, stream))
    {
        std::fprintf (stderr, "cannot write %s\n", argv[1]);
        return 1;
    }
    std::printf ("%s: preset %s\n", argv[1], proc.getProgramName (preset).toRawUTF8());
    return 0;
}
