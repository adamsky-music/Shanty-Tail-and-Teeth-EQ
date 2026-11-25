#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "CustomLookAndFeel.h"
#include <array>
#include <functional>
#include <cmath>

namespace ShantyEQ
{

enum class EditTarget : int
{
    Teeth = 0,
    Tail,
    Both
};

class SpectrumAnalyzerComponent : public juce::Component
{
public:
    explicit SpectrumAnalyzerComponent(ShantyTailAndTeethEQProcessor& p)
        : processor(p)
    {
    }

    std::function<void(int)> onBandSelected;
    std::function<void()> onBandSoloChanged;

    void setSelectedBand(int idx)
    {
        selectedBand = juce::jlimit(0, numBands - 1, idx);
        repaint();
    }

    int getSelectedBand() const noexcept { return selectedBand; }

    void setEditTarget(EditTarget t)
    {
        editTarget = t;
        repaint();
    }

    EditTarget getEditTarget() const noexcept { return editTarget; }

    void updateSpectra()
    {
        processor.copyDisplaySpectra(teethSpec, tailSpec);
        repaint();
    }

    void paint(juce::Graphics& g) override
    {
        const auto bounds = getLocalBounds().toFloat();
        g.setColour(Palette::bgCanvas);
        g.fillRoundedRectangle(bounds, 8.0f);

        const auto plot = bounds.reduced(10.0f, 10.0f);
        drawGrid(g, plot);
        drawSpectra(g, plot);
        drawEQCurves(g, plot);
        drawBandNodes(g, plot);

        g.setColour(Palette::borderSubtle);
        g.drawRoundedRectangle(bounds, 8.0f, 1.2f);
    }

    void mouseDown(const juce::MouseEvent& e) override
    {
        const auto plot = getLocalBounds().toFloat().reduced(10.0f, 10.0f);
        int hit = findNearestNode(e.position, plot, 22.0f);
        if (hit >= 0)
        {
            selectedBand = hit;
            if (onBandSelected)
                onBandSelected(selectedBand);

            if (e.mods.isAltDown())
            {
                const int curSolo = processor.soloBandIndex.load(std::memory_order_relaxed);
                processor.soloBandIndex.store(curSolo == hit ? -1 : hit, std::memory_order_relaxed);
                if (onBandSoloChanged)
                    onBandSoloChanged();
                repaint();
                return;
            }

            draggingBand = hit;

            if (auto* fp = processor.apvts.getParameter(ShantyTailAndTeethEQProcessor::bandParamID(hit, "freq")))
                fp->beginChangeGesture();

            if (editTarget == EditTarget::Teeth || editTarget == EditTarget::Both)
                if (auto* gp = processor.apvts.getParameter(ShantyTailAndTeethEQProcessor::bandParamID(hit, "teethGain")))
                    gp->beginChangeGesture();

            if (editTarget == EditTarget::Tail || editTarget == EditTarget::Both)
                if (auto* gp = processor.apvts.getParameter(ShantyTailAndTeethEQProcessor::bandParamID(hit, "tailGain")))
                    gp->beginChangeGesture();

            repaint();
        }
    }

    void mouseDrag(const juce::MouseEvent& e) override
    {
        if (draggingBand < 0)
            return;

        const auto plot = getLocalBounds().toFloat().reduced(10.0f, 10.0f);
        const float freq = xToFreq(juce::jlimit(plot.getX(), plot.getRight(), e.position.x), plot);
        const float gain = yToGain(juce::jlimit(plot.getY(), plot.getBottom(), e.position.y), plot);

        if (auto* fp = processor.apvts.getParameter(ShantyTailAndTeethEQProcessor::bandParamID(draggingBand, "freq")))
            fp->setValueNotifyingHost(fp->convertTo0to1(freq));

        if (editTarget == EditTarget::Teeth || editTarget == EditTarget::Both)
            if (auto* gp = processor.apvts.getParameter(ShantyTailAndTeethEQProcessor::bandParamID(draggingBand, "teethGain")))
                gp->setValueNotifyingHost(gp->convertTo0to1(gain));

        if (editTarget == EditTarget::Tail || editTarget == EditTarget::Both)
            if (auto* gp = processor.apvts.getParameter(ShantyTailAndTeethEQProcessor::bandParamID(draggingBand, "tailGain")))
                gp->setValueNotifyingHost(gp->convertTo0to1(gain));

        repaint();
    }

    void mouseUp(const juce::MouseEvent&) override
    {
        if (draggingBand >= 0)
        {
            if (auto* fp = processor.apvts.getParameter(ShantyTailAndTeethEQProcessor::bandParamID(draggingBand, "freq")))
                fp->endChangeGesture();

            if (editTarget == EditTarget::Teeth || editTarget == EditTarget::Both)
                if (auto* gp = processor.apvts.getParameter(ShantyTailAndTeethEQProcessor::bandParamID(draggingBand, "teethGain")))
                    gp->endChangeGesture();

            if (editTarget == EditTarget::Tail || editTarget == EditTarget::Both)
                if (auto* gp = processor.apvts.getParameter(ShantyTailAndTeethEQProcessor::bandParamID(draggingBand, "tailGain")))
                    gp->endChangeGesture();

            draggingBand = -1;
        }
    }

    void mouseDoubleClick(const juce::MouseEvent& e) override
    {
        const auto plot = getLocalBounds().toFloat().reduced(10.0f, 10.0f);
        const int hit = findNearestNode(e.position, plot, 22.0f);
        if (hit >= 0)
        {
            processor.resetBand(hit);
            selectedBand = hit;
            if (onBandSelected)
                onBandSelected(selectedBand);
            repaint();
        }
    }

    void mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel) override
    {
        const auto plot = getLocalBounds().toFloat().reduced(10.0f, 10.0f);
        int target = findNearestNode(e.position, plot, 28.0f);
        if (target < 0)
            target = selectedBand;

        if (auto* qp = processor.apvts.getParameter(ShantyTailAndTeethEQProcessor::bandParamID(target, "q")))
        {
            const float cur = qp->convertFrom0to1(qp->getValue());
            const float next = juce::jlimit(0.1f, 12.0f, cur * (1.0f + wheel.deltaY * 0.25f));
            qp->setValueNotifyingHost(qp->convertTo0to1(next));
        }
    }

private:
    ShantyTailAndTeethEQProcessor& processor;
    std::array<float, StructuralSplitDSP::numBins> teethSpec {};
    std::array<float, StructuralSplitDSP::numBins> tailSpec {};
    int selectedBand = 0;
    int draggingBand = -1;
    EditTarget editTarget = EditTarget::Teeth;

    static float freqToX(float freq, const juce::Rectangle<float>& plot) noexcept
    {
        const float minF = std::log10(20.0f);
        const float maxF = std::log10(20000.0f);
        const float norm = (std::log10(juce::jlimit(20.0f, 20000.0f, freq)) - minF) / (maxF - minF);
        return plot.getX() + norm * plot.getWidth();
    }

    static float xToFreq(float x, const juce::Rectangle<float>& plot) noexcept
    {
        const float norm = juce::jlimit(0.0f, 1.0f, (x - plot.getX()) / plot.getWidth());
        const float minF = std::log10(20.0f);
        const float maxF = std::log10(20000.0f);
        return std::pow(10.0f, minF + norm * (maxF - minF));
    }

    static float gainToY(float gainDb, const juce::Rectangle<float>& plot) noexcept
    {
        const float norm = (juce::jlimit(-24.0f, 24.0f, gainDb) + 24.0f) / 48.0f;
        return plot.getBottom() - norm * plot.getHeight();
    }

    static float yToGain(float y, const juce::Rectangle<float>& plot) noexcept
    {
        const float norm = 1.0f - juce::jlimit(0.0f, 1.0f, (y - plot.getY()) / plot.getHeight());
        return norm * 48.0f - 24.0f;
    }

    int findNearestNode(juce::Point<float> pos, const juce::Rectangle<float>& plot, float maxDist) const
    {
        int bestIdx = -1;
        float bestD = maxDist;
        for (int i = 0; i < numBands; ++i)
        {
            const auto bp = processor.getBandParams(i);
            const float nx = freqToX(bp.freq, plot);
            const float gVal = (editTarget == EditTarget::Tail) ? bp.tailGainDb : bp.teethGainDb;
            const float ny = gainToY(gVal, plot);
            const float d = pos.getDistanceFrom({ nx, ny });
            if (d < bestD)
            {
                bestD = d;
                bestIdx = i;
            }
        }
        return bestIdx;
    }

    void drawGrid(juce::Graphics& g, const juce::Rectangle<float>& plot) const
    {
        const float freqs[] = { 30, 50, 100, 200, 500, 1000, 2000, 5000, 10000, 15000 };
        const char* labels[] = { "30", "50", "100", "200", "500", "1k", "2k", "5k", "10k", "15k" };

        g.setFont(10.5f);
        for (int i = 0; i < 10; ++i)
        {
            const float x = freqToX(freqs[i], plot);
            g.setColour(Palette::borderSubtle.withAlpha(0.55f));
            g.drawVerticalLine(static_cast<int>(x), plot.getY(), plot.getBottom());
            g.setColour(Palette::textSecondary.withAlpha(0.7f));
            g.drawText(labels[i], static_cast<int>(x - 14), static_cast<int>(plot.getBottom() - 14), 28, 13, juce::Justification::centred);
        }

        const float dbs[] = { -18.0f, -12.0f, -6.0f, 0.0f, 6.0f, 12.0f, 18.0f };
        for (float db : dbs)
        {
            const float y = gainToY(db, plot);
            g.setColour(db == 0.0f ? Palette::borderSubtle.brighter(0.25f) : Palette::borderSubtle.withAlpha(0.45f));
            g.drawHorizontalLine(static_cast<int>(y), plot.getX(), plot.getRight());
        }
    }

    void drawSpectra(juce::Graphics& g, const juce::Rectangle<float>& plot) const
    {
        const double sr = processor.getCurrentSampleRate();
        constexpr int numBins = StructuralSplitDSP::numBins;
        constexpr int fftSize = StructuralSplitDSP::fftSize;

        auto buildSpecPath = [&](const std::array<float, numBins>& spec) -> juce::Path
        {
            juce::Path p;
            p.startNewSubPath(plot.getX(), plot.getBottom());
            const int steps = 160;
            for (int i = 0; i <= steps; ++i)
            {
                const float norm = static_cast<float>(i) / static_cast<float>(steps);
                const float freq = xToFreq(plot.getX() + norm * plot.getWidth(), plot);
                const float binF = static_cast<float>(freq * fftSize / sr);
                const int b0 = juce::jlimit(1, numBins - 2, static_cast<int>(binF));
                const float frac = binF - static_cast<float>(b0);
                const float mag = spec[static_cast<size_t>(b0)] * (1.0f - frac) + spec[static_cast<size_t>(b0 + 1)] * frac;
                const float db = juce::Decibels::gainToDecibels(mag + 1.0e-5f, -80.0f);
                const float yNorm = juce::jlimit(0.0f, 1.0f, (db + 72.0f) / 72.0f);
                const float y = plot.getBottom() - yNorm * plot.getHeight();
                p.lineTo(plot.getX() + norm * plot.getWidth(), y);
            }
            p.lineTo(plot.getRight(), plot.getBottom());
            p.closeSubPath();
            return p;
        };

        auto tailPath = buildSpecPath(tailSpec);
        g.setColour(Palette::tailCyan.withAlpha(0.16f));
        g.fillPath(tailPath);
        g.setColour(Palette::tailCyan.withAlpha(0.35f));
        g.strokePath(tailPath, juce::PathStrokeType(1.0f));

        auto teethPath = buildSpecPath(teethSpec);
        g.setColour(Palette::teethRed.withAlpha(0.18f));
        g.fillPath(teethPath);
        g.setColour(Palette::teethRed.withAlpha(0.45f));
        g.strokePath(teethPath, juce::PathStrokeType(1.0f));
    }

    void drawEQCurves(juce::Graphics& g, const juce::Rectangle<float>& plot) const
    {
        const double sr = processor.getCurrentSampleRate();
        std::array<BandParams, numBands> bands {};
        std::array<BiquadCoeffs, numBands> teethC {};
        std::array<BiquadCoeffs, numBands> tailC {};

        for (int b = 0; b < numBands; ++b)
        {
            bands[static_cast<size_t>(b)] = processor.getBandParams(b);
            const auto& bp = bands[static_cast<size_t>(b)];
            teethC[static_cast<size_t>(b)] = bp.enabled ? BiquadCoeffs::makeFilter(sr, bp.type, bp.freq, bp.q, bp.teethGainDb) : BiquadCoeffs::makeIdentity();
            tailC[static_cast<size_t>(b)]  = bp.enabled ? BiquadCoeffs::makeFilter(sr, bp.type, bp.freq, bp.q, bp.tailGainDb)  : BiquadCoeffs::makeIdentity();
        }

        juce::Path teethCurve, tailCurve;
        const int steps = 200;
        for (int i = 0; i <= steps; ++i)
        {
            const float x = plot.getX() + (static_cast<float>(i) / static_cast<float>(steps)) * plot.getWidth();
            const double freq = static_cast<double>(xToFreq(x, plot));

            double magTeeth = 1.0, magTail = 1.0;
            for (int b = 0; b < numBands; ++b)
            {
                const auto& bp = bands[static_cast<size_t>(b)];
                const bool isCut = (bp.type == FilterType::LowCut || bp.type == FilterType::HighCut);
                const int stages = (bp.enabled && isCut) ? bp.slopeStages : 1;
                double mTr = teethC[static_cast<size_t>(b)].getMagnitude(sr, freq);
                double mTn = tailC[static_cast<size_t>(b)].getMagnitude(sr, freq);
                if (stages > 1)
                {
                    mTr = std::pow(mTr, static_cast<double>(stages));
                    mTn = std::pow(mTn, static_cast<double>(stages));
                }
                magTeeth *= mTr;
                magTail  *= mTn;
            }

            const float yTeeth = gainToY(static_cast<float>(juce::Decibels::gainToDecibels(magTeeth, -48.0)), plot);
            const float yTail  = gainToY(static_cast<float>(juce::Decibels::gainToDecibels(magTail, -48.0)), plot);

            if (i == 0)
            {
                teethCurve.startNewSubPath(x, yTeeth);
                tailCurve.startNewSubPath(x, yTail);
            }
            else
            {
                teethCurve.lineTo(x, yTeeth);
                tailCurve.lineTo(x, yTail);
            }
        }

        g.setColour(Palette::tailCyan);
        g.strokePath(tailCurve, juce::PathStrokeType(2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        g.setColour(Palette::teethRed);
        g.strokePath(teethCurve, juce::PathStrokeType(2.2f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    void drawBandNodes(juce::Graphics& g, const juce::Rectangle<float>& plot) const
    {
        for (int i = 0; i < numBands; ++i)
        {
            const auto bp = processor.getBandParams(i);
            const float x = freqToX(bp.freq, plot);
            const float gVal = (editTarget == EditTarget::Tail) ? bp.tailGainDb : bp.teethGainDb;
            const float y = gainToY(gVal, plot);

            const bool isSel = (i == selectedBand);
            const bool isSolo = (processor.soloBandIndex.load(std::memory_order_relaxed) == i);
            const bool hasSat = (bp.teethSatType != SatType::Off && bp.teethSatDrive > 0.01f)
                             || (bp.tailSatType != SatType::Off && bp.tailSatDrive > 0.01f);

            const float r = isSel ? 11.0f : 8.5f;

            if (isSolo)
            {
                g.setColour(Palette::textPrimary.withAlpha(0.35f));
                g.drawEllipse(x - r - 5.0f, y - r - 5.0f, (r + 5.0f) * 2.0f, (r + 5.0f) * 2.0f, 1.8f);
            }

            if (hasSat && bp.enabled)
            {
                g.setColour(Palette::satAmber.withAlpha(0.38f));
                g.fillEllipse(x - r - 4.0f, y - r - 4.0f, (r + 4.0f) * 2.0f, (r + 4.0f) * 2.0f);
                g.setColour(Palette::satAmber);
                g.drawEllipse(x - r - 2.5f, y - r - 2.5f, (r + 2.5f) * 2.0f, (r + 2.5f) * 2.0f, 1.4f);
            }

            juce::Colour nodeCol = (editTarget == EditTarget::Tail) ? Palette::tailCyan : Palette::teethRed;
            if (!bp.enabled)
                nodeCol = Palette::textSecondary.withAlpha(0.45f);

            g.setColour(isSel ? nodeCol : nodeCol.withAlpha(0.75f));
            g.fillEllipse(x - r, y - r, r * 2.0f, r * 2.0f);

            g.setColour(isSel ? Palette::textPrimary : Palette::bgDark);
            g.drawEllipse(x - r, y - r, r * 2.0f, r * 2.0f, isSel ? 2.0f : 1.2f);

            g.setColour(Palette::textPrimary);
            g.setFont(juce::Font(11.0f, juce::Font::bold));
            g.drawText(juce::String(i + 1), static_cast<int>(x - r), static_cast<int>(y - r),
                       static_cast<int>(r * 2.0f), static_cast<int>(r * 2.0f), juce::Justification::centred);
        }
    }
};

} // namespace ShantyEQ
