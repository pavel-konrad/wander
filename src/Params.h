// Wander's parameters and presets, in the order of the JSFX sliders (slider1 .. slider31), so a recipe
// written for one version is the same list of numbers in the other.
#pragma once
#include "Synth.h"
#include <array>

namespace wander
{
enum class Unit { None, Octave, Percent, Cents, Level, Hz, Seconds, Oct, Semitones, Db, Choice };

struct ParamInfo
{
    const char* id;
    const char* name;
    float min, max, def;
    bool log;             // logarithmic knob travel (frequencies, times)
    Unit unit;
    const char* choices;  // "A|B|C" for a choice parameter, else nullptr
};

constexpr int numParams = 31;

inline const std::array<ParamInfo, numParams>& params()
{
    static const std::array<ParamInfo, numParams> table { {
        { "osc1_wave",    "Osc 1 Wave",        0, 3, 2, false, Unit::Choice, "Sine|Triangle|Saw|Pulse" },
        { "osc1_oct",     "Osc 1 Octave",     -2, 2, 0, false, Unit::Octave, nullptr },
        { "osc1_shape",   "Osc 1 Shape",    0.05f, 0.95f, 0.5f, false, Unit::Percent, nullptr },
        { "osc2_wave",    "Osc 2 Wave",        0, 3, 2, false, Unit::Choice, "Sine|Triangle|Saw|Pulse" },
        { "osc2_oct",     "Osc 2 Octave",     -2, 2, 0, false, Unit::Octave, nullptr },
        { "osc2_detune",  "Osc 2 Detune",   -100, 100, 7, false, Unit::Cents, nullptr },
        { "osc1_level",   "Osc 1 Level",       0, 1, 0.5f, false, Unit::Level, nullptr },
        { "noise_level",  "Noise Level",       0, 1, 0, false, Unit::Level, nullptr },
        { "filter_type",  "Filter Type",       0, 2, 0, false, Unit::Choice, "Low-pass|High-pass|Band-pass" },
        { "cutoff",       "Filter Freq",      20, 18000, 1200, true, Unit::Hz, nullptr },
        { "resonance",    "Filter Res",        0, 1, 0.2f, false, Unit::Percent, nullptr },
        { "key_tracking", "Filter Key",        0, 1, 0.5f, false, Unit::Percent, nullptr },
        { "env2_cutoff",  "Env 2 > Freq",     -6, 6, 2, false, Unit::Oct, nullptr },
        { "amp_attack",   "Env 1 Attack",  0.001f, 5, 0.005f, true, Unit::Seconds, nullptr },
        { "amp_decay",    "Env 1 Decay",   0.001f, 5, 0.3f, true, Unit::Seconds, nullptr },
        { "amp_sustain",  "Env 1 Sustain",     0, 1, 0.7f, false, Unit::Percent, nullptr },
        { "amp_release",  "Env 1 Release", 0.001f, 8, 0.4f, true, Unit::Seconds, nullptr },
        { "mod_attack",   "Env 2 Attack",  0.001f, 5, 0.001f, true, Unit::Seconds, nullptr },
        { "mod_decay",    "Env 2 Decay",   0.001f, 5, 0.4f, true, Unit::Seconds, nullptr },
        { "mod_sustain",  "Env 2 Sustain",     0, 1, 0, false, Unit::Percent, nullptr },
        { "mod_release",  "Env 2 Release", 0.001f, 8, 0.4f, true, Unit::Seconds, nullptr },
        { "env2_pitch",   "Env 2 > Pitch",   -24, 24, 0, false, Unit::Semitones, nullptr },
        { "lfo_rate",     "LFO Rate",       0.01f, 30, 4, true, Unit::Hz, nullptr },
        { "lfo_wave",     "LFO Shape",         0, 4, 0, false, Unit::Choice, "Sine|Triangle|Saw down|Square|Random" },
        { "lfo_pitch",    "LFO > Pitch",       0, 100, 0, false, Unit::Cents, nullptr },
        { "lfo_cutoff",   "LFO > Freq",        0, 4, 0, false, Unit::Oct, nullptr },
        { "lfo_shape",    "LFO > Shape",       0, 0.45f, 0, false, Unit::Percent, nullptr },
        { "drift",        "Drift",             0, 1, 0.3f, false, Unit::Percent, nullptr },
        { "vel_level",    "Vel > Vol",         0, 1, 0.5f, false, Unit::Percent, nullptr },
        { "volume",       "Volume",          -48, 6, -12, false, Unit::Db, nullptr },
        { "osc2_level",   "Osc 2 Level",       0, 1, 0.5f, false, Unit::Level, nullptr },
    } };
    return table;
}

struct Preset
{
    const char* name;
    std::array<float, numParams> values;
};

// The recipes of the course (06_course/sound-synthesis.course/02-wander), the same numbers as
// jsfx/wander-synth.presets.ini.
inline const std::array<Preset, 8>& presets()
{
    static const std::array<Preset, 8> table { {
        { "Init", { 2.0f, 0.0f, 0.5f, 2.0f, 0.0f, 7.0f, 0.5f, 0.0f, 0.0f, 1200.0f, 0.2f, 0.5f, 2.0f, 0.005f, 0.3f, 0.7f, 0.4f, 0.001f, 0.4f, 0.0f, 0.4f, 0.0f, 4.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.3f, 0.5f, -12.0f, 0.5f } },
        { "Bass", { 2.0f, 0.0f, 0.5f, 2.0f, -1.0f, 4.0f, 0.6f, 0.0f, 0.0f, 250.0f, 0.35f, 0.5f, 3.0f, 0.005f, 0.4f, 0.6f, 0.2f, 0.001f, 0.25f, 0.0f, 0.4f, 0.0f, 4.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.2f, 0.5f, -12.0f, 0.4f } },
        { "Pad", { 2.0f, 0.0f, 0.5f, 2.0f, 0.0f, 12.0f, 0.5f, 0.0f, 0.0f, 1400.0f, 0.15f, 0.5f, 0.5f, 1.2f, 1.0f, 0.9f, 2.5f, 1.5f, 2.0f, 0.6f, 0.4f, 0.0f, 0.2f, 0.0f, 0.0f, 0.5f, 0.0f, 0.7f, 0.5f, -14.0f, 0.5f } },
        { "Sonar ping", { 0.0f, 0.0f, 0.5f, 0.0f, 0.0f, 4.0f, 0.5f, 0.0f, 0.0f, 8000.0f, 0.0f, 0.5f, 0.0f, 0.001f, 1.6f, 0.0f, 1.6f, 0.001f, 0.4f, 0.0f, 0.4f, 0.0f, 4.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.1f, 0.5f, -12.0f, 0.5f } },
        { "Submarine engine", { 3.0f, 0.0f, 0.3f, 2.0f, -1.0f, -6.0f, 0.5f, 0.0f, 0.0f, 320.0f, 0.3f, 0.0f, 0.0f, 0.3f, 1.0f, 1.0f, 1.0f, 0.001f, 0.4f, 0.0f, 0.4f, 0.0f, 5.0f, 3.0f, 6.0f, 0.6f, 0.0f, 0.6f, 0.5f, -10.0f, 0.5f } },
        { "Sea", { 2.0f, 0.0f, 0.5f, 2.0f, 0.0f, 7.0f, 0.0f, 1.0f, 2.0f, 500.0f, 0.3f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 2.0f, 0.001f, 0.4f, 0.0f, 0.4f, 0.0f, 0.12f, 0.0f, 0.0f, 1.5f, 0.0f, 0.5f, 0.5f, -8.0f, 0.0f } },
        { "Steam", { 2.0f, 0.0f, 0.5f, 2.0f, 0.0f, 7.0f, 0.0f, 1.0f, 1.0f, 3500.0f, 0.2f, 0.0f, 1.0f, 0.03f, 0.6f, 0.0f, 0.3f, 0.001f, 0.5f, 0.0f, 0.4f, 0.0f, 4.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.3f, 0.5f, -10.0f, 0.0f } },
        { "Knock", { 0.0f, 0.0f, 0.5f, 0.0f, -1.0f, 7.0f, 0.5f, 0.0f, 0.0f, 600.0f, 0.0f, 0.0f, 0.0f, 0.001f, 0.08f, 0.0f, 0.08f, 0.001f, 0.06f, 0.0f, 0.4f, 12.0f, 4.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.5f, -6.0f, 0.5f } },
    } };
    return table;
}

// Plain values in table order -> the synth's settings.
inline Settings toSettings (const std::array<float, numParams>& v)
{
    Settings s;
    s.osc1Wave = (int) v[0];   s.osc1Oct = v[1];  s.pulseWidth = v[2];
    s.osc2Wave = (int) v[3];   s.osc2Oct = v[4];  s.detuneCents = v[5];
    s.osc1Level = v[6];        s.noise = v[7];
    s.filterType = (int) v[8]; s.cutoff = v[9];   s.resonance = v[10]; s.keyTracking = v[11]; s.env2ToCutoff = v[12];
    s.ampA = v[13]; s.ampD = v[14]; s.ampS = v[15]; s.ampR = v[16];
    s.modA = v[17]; s.modD = v[18]; s.modS = v[19]; s.modR = v[20]; s.env2ToPitch = v[21];
    s.lfoRate = v[22]; s.lfoWave = (int) v[23]; s.lfoToPitchCents = v[24]; s.lfoToCutoff = v[25]; s.lfoToPulseWidth = v[26];
    s.drift = v[27]; s.velocityToLevel = v[28]; s.volumeDb = v[29]; s.osc2Level = v[30];
    return s;
}
} // namespace wander
