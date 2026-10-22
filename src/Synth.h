// Wander's sound, without JUCE: the same algorithm as jsfx/wander-synth.jsfx, line for line where it can be.
//   2 oscillators + noise -> state-variable filter -> amp, an amp and a mod envelope, one LFO,
//   and drift: every voice wanders a little in pitch and cutoff on its own.
#pragma once
#include <array>
#include <cstdint>

namespace wander
{
constexpr int numVoices = 8;

// The parameters, in the units the GUI shows (seconds, Hz, dB, octaves). Order and ranges: Params.h.
struct Settings
{
    int osc1Wave = 2;   double osc1Oct = 0, pulseWidth = 0.5;
    int osc2Wave = 2;   double osc2Oct = 0, detuneCents = 7;
    double osc1Level = 0.5, noise = 0, osc2Level = 0.5;
    int filterType = 0; double cutoff = 1200, resonance = 0.2, keyTracking = 0.5, env2ToCutoff = 2;
    double ampA = 0.005, ampD = 0.3, ampS = 0.7, ampR = 0.4;
    double modA = 0.001, modD = 0.4, modS = 0, modR = 0.4, env2ToPitch = 0;
    double lfoRate = 4; int lfoWave = 0; double lfoToPitchCents = 0, lfoToCutoff = 0, lfoToPulseWidth = 0;
    double drift = 0.3, velocityToLevel = 0.5, volumeDb = -12;
};

class Synth
{
public:
    void prepare (double sampleRate);
    void set (const Settings& s);
    void noteOn (int note, float velocity);   // velocity 0..1
    void noteOff (int note);
    void allNotesOff();
    float process();                           // one sample, mono

    bool voiceSounding (int v) const { return voices[(size_t) v].ampStage > 0; }
    double lfoValue() const { return lfo; }

private:
    struct Voice
    {
        int note = 0; double velocity = 0; bool gate = false; std::uint64_t age = 0;
        double ampEnv = 0; int ampStage = 0;   // 0 off, 1 attack, 2 decay/sustain, 4 release
        double modEnv = 0; int modStage = 0;
        double phase1 = 0, phase2 = 0, ic1 = 0, ic2 = 0, driftPitch = 0, driftCutoff = 0;
    };

    double random();                           // -1 .. 1
    double coef (double seconds) const;
    void update();

    double sr = 48000;
    Settings p;
    std::array<Voice, numVoices> voices {};
    double ampAttack = 0, ampDecay = 0, ampRelease = 0, modAttack = 0, modDecay = 0, modRelease = 0, k = 2, volume = 0.25;
    double lfoPhase = 0, lfoRandom = 0, lfo = 0;
    std::uint64_t rng = 0x9E3779B97F4A7C15ull;
};
} // namespace wander
