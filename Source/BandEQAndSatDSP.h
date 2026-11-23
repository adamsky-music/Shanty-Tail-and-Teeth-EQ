#pragma once

#include <JuceHeader.h>
#include <array>
#include <cmath>
#include <complex>

namespace ShantyEQ
{

static constexpr int numBands = 8;

enum class FilterType : int
{
    LowCut = 0,
    LowShelf,
    Bell,
    Notch,
    HighShelf,
    HighCut
};

enum class SatType : int
{
    Off = 0,
    Tape,
    Tube,
    SoftClip,
    HardClip
};

struct BandParams
{
    bool enabled = true;
    FilterType type = FilterType::Bell;
    float freq = 1000.0f;
    float q = 1.0f;

    // Teeth (Transient)
    float teethGainDb = 0.0f;
    float teethPan = 0.0f; // -1..+1
    SatType teethSatType = SatType::Off;
    float teethSatDrive = 0.0f; // 0..1

    // Tail (Tonal)
    float tailGainDb = 0.0f;
    float tailPan = 0.0f; // -1..+1
    SatType tailSatType = SatType::Off;
    float tailSatDrive = 0.0f; // 0..1
};

struct BiquadCoeffs
{
    double b0 = 1.0, b1 = 0.0, b2 = 0.0;
    double a1 = 0.0, a2 = 0.0;

    static BiquadCoeffs makeIdentity() noexcept
    {
        return { 1.0, 0.0, 0.0, 0.0, 0.0 };
    }

    static BiquadCoeffs makeFilter(double sampleRate, FilterType type, double freq, double q, double gainDb) noexcept
    {
        if (sampleRate <= 0.0)
            return makeIdentity();

        const double clampedFreq = juce::jlimit(18.0, sampleRate * 0.48, freq);
        const double clampedQ    = juce::jlimit(0.1, 16.0, q);
        const double w0 = juce::MathConstants<double>::twoPi * clampedFreq / sampleRate;
        const double cosW0 = std::cos(w0);
        const double sinW0 = std::sin(w0);
        const double alpha = sinW0 / (2.0 * clampedQ);
        const double A = std::pow(10.0, gainDb / 40.0);

        double b0 = 1.0, b1 = 0.0, b2 = 0.0, a0 = 1.0, a1 = 0.0, a2 = 0.0;

        switch (type)
        {
            case FilterType::LowCut:
                b0 =  (1.0 + cosW0) * 0.5;
                b1 = -(1.0 + cosW0);
                b2 =  (1.0 + cosW0) * 0.5;
                a0 =   1.0 + alpha;
                a1 =  -2.0 * cosW0;
                a2 =   1.0 - alpha;
                break;

            case FilterType::HighCut:
                b0 =  (1.0 - cosW0) * 0.5;
                b1 =   1.0 - cosW0;
                b2 =  (1.0 - cosW0) * 0.5;
                a0 =   1.0 + alpha;
                a1 =  -2.0 * cosW0;
                a2 =   1.0 - alpha;
                break;

            case FilterType::LowShelf:
            {
                const double twoSqrtAAlpha = 2.0 * std::sqrt(A) * alpha;
                b0 =    A * ((A + 1.0) - (A - 1.0) * cosW0 + twoSqrtAAlpha);
                b1 = 2.0 * A * ((A - 1.0) - (A + 1.0) * cosW0);
                b2 =    A * ((A + 1.0) - (A - 1.0) * cosW0 - twoSqrtAAlpha);
                a0 =        (A + 1.0) + (A - 1.0) * cosW0 + twoSqrtAAlpha;
                a1 = -2.0 * ((A - 1.0) + (A + 1.0) * cosW0);
                a2 =        (A + 1.0) + (A - 1.0) * cosW0 - twoSqrtAAlpha;
                break;
            }

            case FilterType::HighShelf:
            {
                const double twoSqrtAAlpha = 2.0 * std::sqrt(A) * alpha;
                b0 =    A * ((A + 1.0) + (A - 1.0) * cosW0 + twoSqrtAAlpha);
                b1 = -2.0 * A * ((A - 1.0) + (A + 1.0) * cosW0);
                b2 =    A * ((A + 1.0) + (A - 1.0) * cosW0 - twoSqrtAAlpha);
                a0 =        (A + 1.0) - (A - 1.0) * cosW0 + twoSqrtAAlpha;
                a1 =  2.0 * ((A - 1.0) - (A + 1.0) * cosW0);
                a2 =        (A + 1.0) - (A - 1.0) * cosW0 - twoSqrtAAlpha;
                break;
            }

            case FilterType::Notch:
                b0 =  1.0;
                b1 = -2.0 * cosW0;
                b2 =  1.0;
                a0 =  1.0 + alpha;
                a1 = -2.0 * cosW0;
                a2 =  1.0 - alpha;
                break;

            case FilterType::Bell:
            default:
                if (std::abs(gainDb) < 1.0e-4)
                    return makeIdentity();
                b0 =  1.0 + alpha * A;
                b1 = -2.0 * cosW0;
                b2 =  1.0 - alpha * A;
                a0 =  1.0 + alpha / A;
                a1 = -2.0 * cosW0;
                a2 =  1.0 - alpha / A;
                break;
        }

        const double invA0 = 1.0 / a0;
        return { b0 * invA0, b1 * invA0, b2 * invA0, a1 * invA0, a2 * invA0 };
    }

    static BiquadCoeffs makeBandExtractor(double sampleRate, FilterType type, double freq, double q) noexcept
    {
        if (sampleRate <= 0.0)
            return makeIdentity();

        const double clampedFreq = juce::jlimit(20.0, sampleRate * 0.46, freq);
        const double clampedQ    = juce::jlimit(0.25, 12.0, q);
        const double w0 = juce::MathConstants<double>::twoPi * clampedFreq / sampleRate;
        const double cosW0 = std::cos(w0);
        const double sinW0 = std::sin(w0);
        const double alpha = sinW0 / (2.0 * clampedQ);

        double b0 = 1.0, b1 = 0.0, b2 = 0.0, a0 = 1.0, a1 = 0.0, a2 = 0.0;

        switch (type)
        {
            case FilterType::LowCut:
            case FilterType::LowShelf:
                // Lowpass extraction
                b0 = (1.0 - cosW0) * 0.5;
                b1 =  1.0 - cosW0;
                b2 = (1.0 - cosW0) * 0.5;
                a0 =  1.0 + alpha;
                a1 = -2.0 * cosW0;
                a2 =  1.0 - alpha;
                break;

            case FilterType::HighCut:
            case FilterType::HighShelf:
                // Highpass extraction
                b0 =  (1.0 + cosW0) * 0.5;
                b1 = -(1.0 + cosW0);
                b2 =  (1.0 + cosW0) * 0.5;
                a0 =   1.0 + alpha;
                a1 =  -2.0 * cosW0;
                a2 =   1.0 - alpha;
                break;

            case FilterType::Bell:
            case FilterType::Notch:
            default:
                // Constant 0 dB peak bandpass
                b0 =  alpha;
                b1 =  0.0;
                b2 = -alpha;
                a0 =  1.0 + alpha;
                a1 = -2.0 * cosW0;
                a2 =  1.0 - alpha;
                break;
        }

        const double invA0 = 1.0 / a0;
        return { b0 * invA0, b1 * invA0, b2 * invA0, a1 * invA0, a2 * invA0 };
    }

    double getMagnitude(double sampleRate, double freq) const noexcept
    {
        if (sampleRate <= 0.0)
            return 1.0;

        const double w = juce::MathConstants<double>::twoPi * freq / sampleRate;
        const std::complex<double> z1(std::cos(-w), std::sin(-w));
        const std::complex<double> z2 = z1 * z1;
        const std::complex<double> num = b0 + b1 * z1 + b2 * z2;
        const std::complex<double> den = 1.0 + a1 * z1 + a2 * z2;
        return std::abs(num / den);
    }
};

struct StereoBiquad
{
    double z1[2] = { 0.0, 0.0 };
    double z2[2] = { 0.0, 0.0 };

    void reset() noexcept
    {
        z1[0] = z1[1] = 0.0;
        z2[0] = z2[1] = 0.0;
    }

    inline float processSample(int ch, float x, const BiquadCoeffs& c) noexcept
    {
        const double in = static_cast<double>(x);
        const double out = c.b0 * in + z1[ch];
        z1[ch] = c.b1 * in - c.a1 * out + z2[ch];
        z2[ch] = c.b2 * in - c.a2 * out;
        return static_cast<float>(out);
    }
};

class BandProcessor
{
public:
    void reset() noexcept
    {
        eqFilter.reset();
        satExtractFilter.reset();
        satPostFilter.reset();
        prevSlice[0] = prevSlice[1] = 0.0f;
    }

    void update(double sampleRate, const BandParams& p) noexcept
    {
        params = p;
        teethCoeffs = p.enabled
            ? BiquadCoeffs::makeFilter(sampleRate, p.type, p.freq, p.q, p.teethGainDb)
            : BiquadCoeffs::makeIdentity();

        tailCoeffs = p.enabled
            ? BiquadCoeffs::makeFilter(sampleRate, p.type, p.freq, p.q, p.tailGainDb)
            : BiquadCoeffs::makeIdentity();

        extractCoeffs = BiquadCoeffs::makeBandExtractor(sampleRate, p.type, p.freq, p.q);
    }

    static inline float shapeSample(float x, SatType type, float driveNorm) noexcept
    {
        if (type == SatType::Off || driveNorm <= 0.001f)
            return x;

        // Map driveNorm (0..1) to musical gain (enough bite even on -18 dBFS band slices)
        const float driveGain = 1.0f + driveNorm * 22.0f;
        const float in = x * driveGain;
        float shaped = in;

        switch (type)
        {
            case SatType::Tape:
            {
                // Asymmetric warm tape saturation with subtle 2nd harmonic
                const float asym = in + 0.12f * in * in * juce::jlimit(0.0f, 1.0f, driveNorm);
                shaped = std::tanh(asym) / std::tanh(driveGain * 0.65f + 0.35f);
                // Remove DC bias induced by x^2 term softly
                shaped -= 0.04f * driveNorm * std::tanh(in * in);
                break;
            }
            case SatType::Tube:
            {
                // Triode-like asymmetric soft clip
                const float pos = (in >= 0.0f) ? std::tanh(in * 1.15f) : std::tanh(in * 0.78f) * 1.05f;
                shaped = pos / (0.6f + 0.4f * std::tanh(driveGain));
                break;
            }
            case SatType::SoftClip:
            {
                shaped = std::tanh(in) / (0.55f + 0.45f * std::tanh(driveGain));
                break;
            }
            case SatType::HardClip:
            {
                constexpr float thresh = 0.72f;
                const float clipped = juce::jlimit(-thresh, thresh, in * 0.85f);
                // Soft knee blend
                shaped = (clipped / thresh) * (1.0f / (0.65f + 0.35f * driveGain * 0.25f));
                break;
            }
            case SatType::Off:
            default:
                return x;
        }

        // Gain-compensated blend
        return x + (shaped - x) * juce::jlimit(0.0f, 1.0f, 0.35f + 0.65f * driveNorm);
    }

    inline void processBranch(float& l, float& r, bool isTeeth) noexcept
    {
        if (!params.enabled)
            return;

        const BiquadCoeffs& c = isTeeth ? teethCoeffs : tailCoeffs;
        l = eqFilter.processSample(0, l, c);
        r = eqFilter.processSample(1, r, c);

        const SatType satType   = isTeeth ? params.teethSatType : params.tailSatType;
        const float   satDrive  = isTeeth ? params.teethSatDrive : params.tailSatDrive;
        const float   pan       = isTeeth ? params.teethPan : params.tailPan;

        // Per-band frequency-focused saturation with 2x oversampled waveshaping
        if (satType != SatType::Off && satDrive > 0.001f)
        {
            float sliceL = satExtractFilter.processSample(0, l, extractCoeffs);
            float sliceR = satExtractFilter.processSample(1, r, extractCoeffs);

            // 2x linear oversampling for clean anti-aliased harmonics
            const float midL = 0.5f * (prevSlice[0] + sliceL);
            const float midR = 0.5f * (prevSlice[1] + sliceR);
            prevSlice[0] = sliceL;
            prevSlice[1] = sliceR;

            const float satL0 = shapeSample(midL, satType, satDrive);
            const float satL1 = shapeSample(sliceL, satType, satDrive);
            const float satR0 = shapeSample(midR, satType, satDrive);
            const float satR1 = shapeSample(sliceR, satType, satDrive);

            const float satSliceL = 0.5f * (satL0 + satL1);
            const float satSliceR = 0.5f * (satR0 + satR1);

            // Band-limit the newly generated harmonics slightly to avoid harshness
            const float harmL = satPostFilter.processSample(0, satSliceL - sliceL, extractCoeffs);
            const float harmR = satPostFilter.processSample(1, satSliceR - sliceR, extractCoeffs);

            l += harmL * 0.85f + (satSliceL - sliceL) * 0.35f;
            r += harmR * 0.85f + (satSliceR - sliceR) * 0.35f;
        }

        // Per-band pan (only applied to the band's energy or proportional pan)
        if (std::abs(pan) > 0.001f)
        {
            // Constant-power gentle stereo tilt for this band
            const float gainL = juce::jlimit(0.0f, 1.414f, 1.0f - pan * 0.45f);
            const float gainR = juce::jlimit(0.0f, 1.414f, 1.0f + pan * 0.45f);
            l *= gainL;
            r *= gainR;
        }
    }

    double getMagnitude(double sampleRate, double freq, bool isTeeth) const noexcept
    {
        if (!params.enabled)
            return 1.0;
        return (isTeeth ? teethCoeffs : tailCoeffs).getMagnitude(sampleRate, freq);
    }

private:
    BandParams params;
    BiquadCoeffs teethCoeffs;
    BiquadCoeffs tailCoeffs;
    BiquadCoeffs extractCoeffs;

    StereoBiquad eqFilter;
    StereoBiquad satExtractFilter;
    StereoBiquad satPostFilter;
    float prevSlice[2] = { 0.0f, 0.0f };
};

} // namespace ShantyEQ
