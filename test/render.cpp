// Renders notes through Wander's sound to a 24-bit WAV, without JUCE or a host:
//   wander_render PRESET OUT.wav NOTE,START,LENGTH[,VELOCITY] ...   (times in seconds, velocity 1..127)
// PRESET is a preset number (0 = Init). Used to check pitch and level against the JSFX version.
#include "Params.h"
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

namespace
{
struct Note { int note; double start, length; float velocity; };

void put32 (std::FILE* f, unsigned v) { unsigned char b[4] = { (unsigned char) v, (unsigned char) (v >> 8), (unsigned char) (v >> 16), (unsigned char) (v >> 24) }; std::fwrite (b, 1, 4, f); }
void put16 (std::FILE* f, unsigned v) { unsigned char b[2] = { (unsigned char) v, (unsigned char) (v >> 8) }; std::fwrite (b, 1, 2, f); }
} // namespace

int main (int argc, char** argv)
{
    if (argc < 4)
    {
        std::fprintf (stderr, "usage: wander_render PRESET OUT.wav NOTE,START,LENGTH[,VELOCITY] ...\n");
        return 2;
    }
    const int preset = std::atoi (argv[1]);
    if (preset < 0 || preset >= (int) wander::presets().size()) { std::fprintf (stderr, "no preset %d\n", preset); return 2; }

    std::vector<Note> notes;
    double end = 0;
    for (int i = 3; i < argc; ++i)
    {
        Note n { 60, 0, 1, 100 / 127.0f };
        int vel = 100;
        if (std::sscanf (argv[i], "%d,%lf,%lf,%d", &n.note, &n.start, &n.length, &vel) < 3) { std::fprintf (stderr, "bad note %s\n", argv[i]); return 2; }
        n.velocity = vel / 127.0f;
        notes.push_back (n);
        end = std::max (end, n.start + n.length);
    }

    const double sr = 48000;
    wander::Synth synth;
    synth.prepare (sr);
    synth.set (wander::toSettings (wander::presets()[(size_t) preset].values));

    const long total = (long) ((end + 3) * sr);
    std::vector<float> out ((size_t) total);
    for (long s = 0; s < total; ++s)
    {
        const double t = s / sr;
        for (const auto& n : notes)
        {
            if (s == (long) (n.start * sr)) synth.noteOn (n.note, n.velocity);
            if (s == (long) ((n.start + n.length) * sr)) synth.noteOff (n.note);
        }
        (void) t;
        out[(size_t) s] = synth.process();
    }

    std::FILE* f = std::fopen (argv[2], "wb");
    if (f == nullptr) { std::fprintf (stderr, "cannot write %s\n", argv[2]); return 1; }
    const unsigned data = (unsigned) total * 3;
    std::fwrite ("RIFF", 1, 4, f); put32 (f, 36 + data); std::fwrite ("WAVEfmt ", 1, 8, f);
    put32 (f, 16); put16 (f, 1); put16 (f, 1); put32 (f, (unsigned) sr); put32 (f, (unsigned) sr * 3); put16 (f, 3); put16 (f, 24);
    std::fwrite ("data", 1, 4, f); put32 (f, data);
    for (float v : out)
    {
        int x = (int) (std::max (-1.0f, std::min (1.0f, v)) * 8388607.0f);
        unsigned char b[3] = { (unsigned char) x, (unsigned char) (x >> 8), (unsigned char) (x >> 16) };
        std::fwrite (b, 1, 3, f);
    }
    std::fclose (f);
    std::printf ("%s: preset %s, %.2f s\n", argv[2], wander::presets()[(size_t) preset].name, total / sr);
    return 0;
}
