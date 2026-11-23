#pragma once

#include <JuceHeader.h>
#include <array>
#include <vector>
#include <cmath>
#include <atomic>

namespace ShantyEQ
{

enum class SourceType : int
{
    Default = 0,
    Drums,
    Bass,
    Vocal,
    Guitar,
    Keys,
    Mix
};

struct SplitParams
{
    SourceType source = SourceType::Default;
    float separation = 0.50f;   // 0..1 (sensitivity / contrast between Teeth & Tail)
    float smoothness = 0.35f;   // 0..1 (frequency-axis mask smoothing)
    float decay      = 0.30f;   // 0..1 (transient mask release time)
};

class StructuralSplitDSP
{
public:
    static constexpr int fftOrder = 11;
    static constexpr int fftSize  = 1 << fftOrder; // 2048
    static constexpr int hopSize  = fftSize / 4;   // 512 (75% overlap)
    static constexpr int numBins  = fftSize / 2 + 1;

    StructuralSplitDSP()
        : fft(fftOrder)
    {
        for (int i = 0; i < fftSize; ++i)
        {
            const float phase = juce::MathConstants<float>::pi * (static_cast<float>(i) + 0.5f) / static_cast<float>(fftSize);
            window[static_cast<size_t>(i)] = std::sin(phase);
        }
        reset();
    }

    void prepare(double newSampleRate)
    {
        sampleRate = newSampleRate > 0.0 ? newSampleRate : 44100.0;
        reset();
    }

    void reset()
    {
        for (int ch = 0; ch < 2; ++ch)
        {
            std::fill(inputRing[ch].begin(), inputRing[ch].end(), 0.0f);
            std::fill(transOutRing[ch].begin(), transOutRing[ch].end(), 0.0f);
            std::fill(tonalOutRing[ch].begin(), tonalOutRing[ch].end(), 0.0f);
            std::fill(fftWork[ch].begin(), fftWork[ch].end(), 0.0f);
            std::fill(fftTrans[ch].begin(), fftTrans[ch].end(), 0.0f);
            std::fill(fftTonal[ch].begin(), fftTonal[ch].end(), 0.0f);
        }

        std::fill(tonalFloor.begin(), tonalFloor.end(), 0.0f);
        std::fill(prevTransMask.begin(), prevTransMask.end(), 0.0f);
        std::fill(rawMask.begin(), rawMask.end(), 0.0f);
        std::fill(smoothMask.begin(), smoothMask.end(), 0.0f);

        for (int i = 0; i < numBins; ++i)
        {
            dispTransMag[static_cast<size_t>(i)].store(0.0f, std::memory_order_relaxed);
            dispTonalMag[static_cast<size_t>(i)].store(0.0f, std::memory_order_relaxed);
        }

        writePos = 0;
        readPos = 0;
        hopCounter = 0;
    }

    static constexpr int getLatencySamples() noexcept
    {
        return fftSize;
    }

    // Processes stereo input (inL, inR) and writes separated stereo buses:
    // outTransL/R ("Teeth") and outTonalL/R ("Tail").
    // Supports in-place if needed by reading input before writing output.
    void processBlock(const float* inL, const float* inR,
                      float* outTransL, float* outTransR,
                      float* outTonalL, float* outTonalR,
                      int numSamples,
                      const SplitParams& params)
    {
        for (int i = 0; i < numSamples; ++i)
        {
            const float xL = inL[i];
            const float xR = (inR != nullptr) ? inR[i] : xL;

            inputRing[0][static_cast<size_t>(writePos)] = xL;
            inputRing[1][static_cast<size_t>(writePos)] = xR;

            outTransL[i] = transOutRing[0][static_cast<size_t>(readPos)];
            outTransR[i] = transOutRing[1][static_cast<size_t>(readPos)];
            outTonalL[i] = tonalOutRing[0][static_cast<size_t>(readPos)];
            outTonalR[i] = tonalOutRing[1][static_cast<size_t>(readPos)];

            // Clear consumed output slot so future overlap-add accumulates cleanly
            transOutRing[0][static_cast<size_t>(readPos)] = 0.0f;
            transOutRing[1][static_cast<size_t>(readPos)] = 0.0f;
            tonalOutRing[0][static_cast<size_t>(readPos)] = 0.0f;
            tonalOutRing[1][static_cast<size_t>(readPos)] = 0.0f;

            writePos = (writePos + 1) & (fftSize - 1);
            readPos  = (readPos + 1) & (fftSize - 1);
            ++hopCounter;

            if (hopCounter >= hopSize)
            {
                hopCounter = 0;
                processFrame(params);
            }
        }
    }

    void copyDisplaySpectra(std::array<float, numBins>& transOut,
                            std::array<float, numBins>& tonalOut) const
    {
        for (int i = 0; i < numBins; ++i)
        {
            transOut[static_cast<size_t>(i)] = dispTransMag[static_cast<size_t>(i)].load(std::memory_order_relaxed);
            tonalOut[static_cast<size_t>(i)] = dispTonalMag[static_cast<size_t>(i)].load(std::memory_order_relaxed);
        }
    }

private:
    void getSourceConstants(const SplitParams& params,
                            float& floorAttackCoeff,
                            float& floorReleaseCoeff,
                            float& fluxBias,
                            float& hfWeight) const
    {
        // Frame rate = sampleRate / hopSize (~86 Hz at 44.1kHz)
        const float frameRate = static_cast<float>(sampleRate / static_cast<double>(hopSize));
        auto tauToCoeff = [frameRate](float tauSec) -> float
        {
            return std::exp(-1.0f / juce::jmax(0.1f, tauSec * frameRate));
        };

        switch (params.source)
        {
            case SourceType::Drums:
                floorAttackCoeff  = tauToCoeff(0.280f);
                floorReleaseCoeff = tauToCoeff(0.060f);
                fluxBias = 1.00f;
                hfWeight = 1.15f;
                break;
            case SourceType::Bass:
                floorAttackCoeff  = tauToCoeff(0.320f);
                floorReleaseCoeff = tauToCoeff(0.080f);
                fluxBias = 1.00f;
                hfWeight = 0.90f;
                break;
            case SourceType::Vocal:
                floorAttackCoeff  = tauToCoeff(0.220f);
                floorReleaseCoeff = tauToCoeff(0.050f);
                fluxBias = 1.00f;
                hfWeight = 1.20f;
                break;
            case SourceType::Guitar:
                floorAttackCoeff  = tauToCoeff(0.250f);
                floorReleaseCoeff = tauToCoeff(0.065f);
                fluxBias = 1.00f;
                hfWeight = 1.10f;
                break;
            case SourceType::Keys:
                floorAttackCoeff  = tauToCoeff(0.300f);
                floorReleaseCoeff = tauToCoeff(0.075f);
                fluxBias = 1.00f;
                hfWeight = 1.00f;
                break;
            case SourceType::Mix:
                floorAttackCoeff  = tauToCoeff(0.260f);
                floorReleaseCoeff = tauToCoeff(0.065f);
                fluxBias = 1.00f;
                hfWeight = 1.08f;
                break;
            case SourceType::Default:
            default:
                floorAttackCoeff  = tauToCoeff(0.260f);
                floorReleaseCoeff = tauToCoeff(0.065f);
                fluxBias = 1.00f;
                hfWeight = 1.05f;
                break;
        }
    }

    void processFrame(const SplitParams& params)
    {
        // 1. Copy windowed input into FFT work buffers
        for (int ch = 0; ch < 2; ++ch)
        {
            std::fill(fftWork[ch].begin(), fftWork[ch].end(), 0.0f);
            for (int n = 0; n < fftSize; ++n)
            {
                const int idx = (writePos + n) & (fftSize - 1);
                fftWork[ch][static_cast<size_t>(n)] = inputRing[ch][static_cast<size_t>(idx)] * window[static_cast<size_t>(n)];
            }
            fft.performRealOnlyForwardTransform(fftWork[ch].data(), true);
        }

        // 2. Compute stereo-linked magnitude spectrum and transient mask
        float floorAtk = 0.85f, floorRel = 0.70f, fluxBias = 1.08f, hfWeight = 1.0f;
        getSourceConstants(params, floorAtk, floorRel, fluxBias, hfWeight);

        // Separation maps 0..1 to contrast/slope & threshold (0 -> strict peaks only, 1 -> sensitive)
        const float sep = juce::jlimit(0.0f, 1.0f, params.separation);
        const float threshold = juce::jmap(sep, 0.0f, 1.0f, 0.55f, 0.06f);
        const float contrast  = juce::jmap(sep, 0.0f, 1.0f, 2.20f, 0.75f);
        const float decayCoeff = juce::jmap(juce::jlimit(0.0f, 1.0f, params.decay), 0.0f, 1.0f, 0.05f, 0.85f);

        constexpr float eps = 1.0e-7f;

        for (int k = 0; k < numBins; ++k)
        {
            const float reL = fftWork[0][static_cast<size_t>(2 * k)];
            const float imL = (k == 0 || k == numBins - 1) ? 0.0f : fftWork[0][static_cast<size_t>(2 * k + 1)];
            const float reR = fftWork[1][static_cast<size_t>(2 * k)];
            const float imR = (k == 0 || k == numBins - 1) ? 0.0f : fftWork[1][static_cast<size_t>(2 * k + 1)];

            const float magSq = 0.5f * (reL * reL + imL * imL + reR * reR + imR * imR);
            const float mag = std::sqrt(magSq);

            // Track steady-state tonal floor (slow attack on upward jumps, faster release)
            float& floorVal = tonalFloor[static_cast<size_t>(k)];
            if (mag > floorVal)
                floorVal = floorAtk * floorVal + (1.0f - floorAtk) * mag;
            else
                floorVal = floorRel * floorVal + (1.0f - floorRel) * mag;

            // Slight high-frequency transient sensitivity weighting
            const float normBin = static_cast<float>(k) / static_cast<float>(numBins - 1);
            const float binWeight = 1.0f + (hfWeight - 1.0f) * normBin;

            const float excess = juce::jmax(0.0f, mag - floorVal);
            const float ratio = juce::jlimit(0.0f, 1.0f, (excess / (mag + eps)) * binWeight);

            // Zero-leakage thresholded mapping: steady tones (ratio <= threshold) stay 100% in Tail
            float m = 0.0f;
            if (ratio > threshold)
            {
                const float norm = (ratio - threshold) / juce::jmax(0.05f, 1.0f - threshold);
                m = std::pow(juce::jlimit(0.0f, 1.0f, norm), contrast);
            }
            rawMask[static_cast<size_t>(k)] = m;
        }

        // 3. Frequency-axis smoothing to prevent spectral chatter
        const int radius = static_cast<int>(std::round(juce::jmap(juce::jlimit(0.0f, 1.0f, params.smoothness), 0.0f, 1.0f, 0.0f, 6.0f)));
        if (radius > 0)
        {
            for (int k = 0; k < numBins; ++k)
            {
                const int k0 = juce::jmax(0, k - radius);
                const int k1 = juce::jmin(numBins - 1, k + radius);
                float sum = 0.0f;
                float wSum = 0.0f;
                for (int j = k0; j <= k1; ++j)
                {
                    const float w = 1.0f - static_cast<float>(std::abs(j - k)) / static_cast<float>(radius + 1);
                    sum += rawMask[static_cast<size_t>(j)] * w;
                    wSum += w;
                }
                smoothMask[static_cast<size_t>(k)] = sum / juce::jmax(1.0e-5f, wSum);
            }
        }
        else
        {
            smoothMask = rawMask;
        }

        // 4. Temporal decay smoothing & complementary masking + display spectrum update
        for (int k = 0; k < numBins; ++k)
        {
            float m = smoothMask[static_cast<size_t>(k)];
            float& prevM = prevTransMask[static_cast<size_t>(k)];
            if (m < prevM)
                m = decayCoeff * prevM + (1.0f - decayCoeff) * m;
            prevM = juce::jlimit(0.0f, 1.0f, m);

            const float wTrans = prevM;
            const float wTonal = 1.0f - wTrans;

            for (int ch = 0; ch < 2; ++ch)
            {
                const float re = fftWork[ch][static_cast<size_t>(2 * k)];
                const float im = (k == 0 || k == numBins - 1) ? 0.0f : fftWork[ch][static_cast<size_t>(2 * k + 1)];

                fftTrans[ch][static_cast<size_t>(2 * k)] = re * wTrans;
                fftTonal[ch][static_cast<size_t>(2 * k)] = re * wTonal;
                if (k > 0 && k < numBins - 1)
                {
                    fftTrans[ch][static_cast<size_t>(2 * k + 1)] = im * wTrans;
                    fftTonal[ch][static_cast<size_t>(2 * k + 1)] = im * wTonal;
                }
            }

            // Store smoothed display magnitudes (normalized by fftSize)
            const float reL = fftWork[0][static_cast<size_t>(2 * k)];
            const float imL = (k == 0 || k == numBins - 1) ? 0.0f : fftWork[0][static_cast<size_t>(2 * k + 1)];
            const float reR = fftWork[1][static_cast<size_t>(2 * k)];
            const float imR = (k == 0 || k == numBins - 1) ? 0.0f : fftWork[1][static_cast<size_t>(2 * k + 1)];
            const float magNorm = std::sqrt(0.5f * (reL * reL + imL * imL + reR * reR + imR * imR)) / static_cast<float>(fftSize * 0.5f);

            const float prevDispTr = dispTransMag[static_cast<size_t>(k)].load(std::memory_order_relaxed);
            const float prevDispTn = dispTonalMag[static_cast<size_t>(k)].load(std::memory_order_relaxed);
            dispTransMag[static_cast<size_t>(k)].store(0.60f * prevDispTr + 0.40f * (magNorm * wTrans), std::memory_order_relaxed);
            dispTonalMag[static_cast<size_t>(k)].store(0.60f * prevDispTn + 0.40f * (magNorm * wTonal), std::memory_order_relaxed);
        }

        // 5. Inverse FFT and WOLA synthesis
        // With sqrt-Hann window and 75% overlap (hop = fftSize/4), sum(w^2) = 2.0
        constexpr float overlapNorm = 0.5f;

        for (int ch = 0; ch < 2; ++ch)
        {
            fft.performRealOnlyInverseTransform(fftTrans[ch].data());
            fft.performRealOnlyInverseTransform(fftTonal[ch].data());

            for (int n = 0; n < fftSize; ++n)
            {
                const int outIdx = (readPos + n) & (fftSize - 1);
                const float w = window[static_cast<size_t>(n)] * overlapNorm;
                transOutRing[ch][static_cast<size_t>(outIdx)] += fftTrans[ch][static_cast<size_t>(n)] * w;
                tonalOutRing[ch][static_cast<size_t>(outIdx)] += fftTonal[ch][static_cast<size_t>(n)] * w;
            }
        }
    }

    juce::dsp::FFT fft;
    double sampleRate = 44100.0;

    std::array<float, fftSize> window {};
    std::array<std::array<float, fftSize>, 2> inputRing {};
    std::array<std::array<float, fftSize>, 2> transOutRing {};
    std::array<std::array<float, fftSize>, 2> tonalOutRing {};

    // Real-only FFT in JUCE requires 2 * fftSize floats
    std::array<std::array<float, fftSize * 2>, 2> fftWork {};
    std::array<std::array<float, fftSize * 2>, 2> fftTrans {};
    std::array<std::array<float, fftSize * 2>, 2> fftTonal {};

    std::array<float, numBins> tonalFloor {};
    std::array<float, numBins> rawMask {};
    std::array<float, numBins> smoothMask {};
    std::array<float, numBins> prevTransMask {};

    std::array<std::atomic<float>, numBins> dispTransMag {};
    std::array<std::atomic<float>, numBins> dispTonalMag {};

    int writePos = 0;
    int readPos = 0;
    int hopCounter = 0;
};

} // namespace ShantyEQ
