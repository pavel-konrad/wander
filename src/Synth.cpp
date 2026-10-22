#include "Synth.h"
#include <algorithm>
#include <cmath>

namespace wander
{
namespace
{
constexpr double pi = 3.14159265358979323846;

// PolyBLEP: rounds off the jump in a saw or pulse so it does not alias (whistle) at high notes.
double blep (double t, double dt)
{
    if (t < dt) { t /= dt; return t + t - t * t - 1; }
    if (t > 1 - dt) { t = (t - 1) / dt; return t * t + t + t + 1; }
    return 0;
}

// One sample of a wave at phase ph (0..1), phase step dt, pulse width pw.
double oscillator (double ph, double dt, int wave, double pw)
{
    switch (wave)
    {
        case 0: return std::sin (2 * pi * ph);
        case 1: return 1 - 4 * std::abs (ph - 0.5);
        case 2: return 2 * ph - 1 - blep (ph, dt);
        default:
        {
            double v = ph < pw ? 1 : -1;
            v += blep (ph, dt);
            double p2 = ph - pw;
            if (p2 < 0) p2 += 1;
            return v - blep (p2, dt);
        }
    }
}
} // namespace

void Synth::prepare (double sampleRate)
{
    sr = sampleRate;
    allNotesOff();
    for (auto& v : voices) v = Voice {};
    update();
}

void Synth::set (const Settings& s)
{
    p = s;
    update();
}

double Synth::coef (double seconds) const
{
    // per-sample step of an exponential approach: 99 % of the way in `seconds`
    return 1 - std::exp (-4.6 / (std::max (seconds, 0.001) * sr));
}

void Synth::update()
{
    ampAttack = 1 / (std::max (p.ampA, 0.001) * sr); ampDecay = coef (p.ampD); ampRelease = coef (p.ampR);
    modAttack = 1 / (std::max (p.modA, 0.001) * sr); modDecay = coef (p.modD); modRelease = coef (p.modR);
    k = 2 - 1.96 * p.resonance;                 // filter damping: 2 = no resonance, near 0 = rings
    volume = std::pow (10.0, p.volumeDb / 20);
}

double Synth::random()
{
    rng ^= rng << 13; rng ^= rng >> 7; rng ^= rng << 17;
    return (double) (rng >> 11) / 4503599627370496.0 - 1;   // 2^52: -1 .. 1
}

void Synth::noteOn (int note, float velocity)
{
    // a free voice, or the one that has sounded longest
    int best = 0;
    std::uint64_t oldest = 0;
    bool free = false;
    for (int i = 0; i < numVoices; ++i)
    {
        if (voices[(size_t) i].ampStage == 0) { best = i; free = true; break; }
        if (voices[(size_t) i].age >= oldest) { oldest = voices[(size_t) i].age; best = i; }
    }
    (void) free;
    auto& v = voices[(size_t) best];
    v.note = note; v.velocity = velocity; v.gate = true; v.age = 0;
    v.ampStage = 1; v.modStage = 1; v.modEnv = 0;
}

void Synth::noteOff (int note)
{
    for (auto& v : voices)
        if (v.note == note && v.gate) { v.gate = false; v.ampStage = 4; v.modStage = 4; }
}

void Synth::allNotesOff()
{
    for (auto& v : voices)
        if (v.ampStage > 0) { v.gate = false; v.ampStage = 4; v.modStage = 4; }
}

float Synth::process()
{
    // LFO, one for all voices: a value from -1 to 1
    lfoPhase += p.lfoRate / sr;
    if (lfoPhase >= 1) { lfoPhase -= 1; lfoRandom = random(); }
    switch (p.lfoWave)
    {
        case 0: lfo = std::sin (2 * pi * lfoPhase); break;
        case 1: lfo = 1 - 4 * std::abs (lfoPhase - 0.5); break;
        case 2: lfo = 1 - 2 * lfoPhase; break;
        case 3: lfo = lfoPhase < 0.5 ? 1 : -1; break;
        default: lfo = lfoRandom; break;
    }

    double out = 0;
    for (auto& v : voices)
    {
        if (v.ampStage == 0) continue;
        ++v.age;

        // amp envelope: 1 attack (straight up), 2 decay to sustain, 4 release to 0
        if (v.ampStage == 1) { v.ampEnv += ampAttack; if (v.ampEnv >= 1) { v.ampEnv = 1; v.ampStage = 2; } }
        else if (v.ampStage == 2) v.ampEnv += (p.ampS - v.ampEnv) * ampDecay;
        else { v.ampEnv -= v.ampEnv * ampRelease; if (v.ampEnv < 0.00005) { v.ampEnv = 0; v.ampStage = 0; } }
        // mod envelope, same shape, its own times
        if (v.modStage == 1) { v.modEnv += modAttack; if (v.modEnv >= 1) { v.modEnv = 1; v.modStage = 2; } }
        else if (v.modStage == 2) v.modEnv += (p.modS - v.modEnv) * modDecay;
        else v.modEnv -= v.modEnv * modRelease;

        // drift: each voice's own slow random walk, pulled back towards 0
        v.driftPitch = v.driftPitch * 0.99995 + random() * 0.004;
        v.driftCutoff = v.driftCutoff * 0.99995 + random() * 0.004;
        const double dp = v.driftPitch * p.drift * 0.25;     // up to about a quarter of a semitone
        const double dc = v.driftCutoff * p.drift * 0.5;     // up to about half an octave of cutoff

        // pitch in semitones from A4, then frequency
        const double semis = v.note - 69 + v.modEnv * p.env2ToPitch + lfo * p.lfoToPitchCents / 100 + dp;
        const double f1 = 440 * std::pow (2.0, (semis + 12 * p.osc1Oct) / 12);
        const double f2 = 440 * std::pow (2.0, (semis + 12 * p.osc2Oct + p.detuneCents / 100) / 12);
        const double dt1 = std::min (f1 / sr, 0.45), dt2 = std::min (f2 / sr, 0.45);
        const double pw = std::clamp (p.pulseWidth + lfo * p.lfoToPulseWidth, 0.05, 0.95);
        const double o1 = oscillator (v.phase1, dt1, p.osc1Wave, pw);
        const double o2 = oscillator (v.phase2, dt2, p.osc2Wave, pw);
        v.phase1 += dt1; if (v.phase1 >= 1) v.phase1 -= 1;
        v.phase2 += dt2; if (v.phase2 >= 1) v.phase2 -= 1;
        const double x = o1 * p.osc1Level + o2 * p.osc2Level + random() * p.noise;   // the mixer

        // filter: a state-variable filter (Zavalishin's TPT form), stable at any setting
        double fc = p.cutoff * std::pow (2.0, v.modEnv * p.env2ToCutoff + lfo * p.lfoToCutoff + dc
                                                + p.keyTracking * (v.note - 60) / 12);
        fc = std::clamp (fc, 20.0, sr * 0.45);
        const double g = std::tan (pi * fc / sr);
        const double bp = (v.ic1 + g * (x - v.ic2)) / (1 + g * (g + k));
        const double lp = v.ic2 + g * bp;
        v.ic1 = 2 * bp - v.ic1;
        v.ic2 = 2 * lp - v.ic2;
        const double hp = x - k * bp - lp;
        const double y = p.filterType == 0 ? lp : p.filterType == 1 ? hp : bp;

        out += y * v.ampEnv * (1 - p.velocityToLevel + p.velocityToLevel * v.velocity);
    }
    out *= volume;
    return (float) (out / (1 + std::abs (out)));   // soft limit: never clips hard
}
} // namespace wander
