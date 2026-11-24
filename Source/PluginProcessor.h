#pragma once

#include <JuceHeader.h>
#include "StructuralSplitDSP.h"
#include "BandEQAndSatDSP.h"
#include <array>
#include <atomic>
#include <vector>

class ShantyTailAndTeethEQProcessor : public juce::AudioProcessor
{
public:
    ShantyTailAndTeethEQProcessor();
    ~ShantyTailAndTeethEQProcessor() override = default;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    juce::AudioProcessorParameter* getBypassParameter() const override { return apvts.getParameter("bypass"); }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.05; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;

    // For UI spectrum & curve rendering
    void copyDisplaySpectra(std::array<float, ShantyEQ::StructuralSplitDSP::numBins>& teethOut,
                            std::array<float, ShantyEQ::StructuralSplitDSP::numBins>& tailOut) const
    {
        splitDSP.copyDisplaySpectra(teethOut, tailOut);
    }

    ShantyEQ::BandParams getBandParams(int bandIndex) const;
    double getCurrentSampleRate() const noexcept { return currentSampleRate; }

    std::atomic<float> teethMeter { 0.0f };
    std::atomic<float> tailMeter  { 0.0f };
    std::atomic<float> teethSatMeter { 0.0f };
    std::atomic<float> tailSatMeter  { 0.0f };
    std::atomic<uint32_t> lastProcessMs { 0 };

    static juce::String bandParamID(int bandIndex, const char* suffix)
    {
        return "b" + juce::String(bandIndex + 1) + "_" + suffix;
    }

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    ShantyEQ::StructuralSplitDSP splitDSP;
    std::array<ShantyEQ::BandProcessor, ShantyEQ::numBands> teethBands;
    std::array<ShantyEQ::BandProcessor, ShantyEQ::numBands> tailBands;

    juce::AudioBuffer<float> teethBuffer;
    juce::AudioBuffer<float> tailBuffer;
    juce::AudioBuffer<float> dryDelayBuffer;
    int dryDelayWritePos = 0;

    double currentSampleRate = 44100.0;

    // Cached atomic parameter pointers
    struct BandParamPtrs
    {
        std::atomic<float>* enabled = nullptr;
        std::atomic<float>* type = nullptr;
        std::atomic<float>* freq = nullptr;
        std::atomic<float>* q = nullptr;
        std::atomic<float>* teethGain = nullptr;
        std::atomic<float>* teethPan = nullptr;
        std::atomic<float>* teethSatType = nullptr;
        std::atomic<float>* teethSatDrive = nullptr;
        std::atomic<float>* tailGain = nullptr;
        std::atomic<float>* tailPan = nullptr;
        std::atomic<float>* tailSatType = nullptr;
        std::atomic<float>* tailSatDrive = nullptr;
    };

    std::array<BandParamPtrs, ShantyEQ::numBands> bandPtrs;

    std::atomic<float>* splitSourcePtr = nullptr;
    std::atomic<float>* splitSepPtr = nullptr;
    std::atomic<float>* splitSmoothPtr = nullptr;
    std::atomic<float>* splitDecayPtr = nullptr;

    std::atomic<float>* teethLevelPtr = nullptr;
    std::atomic<float>* tailLevelPtr = nullptr;
    std::atomic<float>* teethSoloPtr = nullptr;
    std::atomic<float>* tailSoloPtr = nullptr;
    std::atomic<float>* midSidePtr = nullptr;
    std::atomic<float>* mixPtr = nullptr;
    std::atomic<float>* outputGainPtr = nullptr;
    std::atomic<float>* bypassPtr = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ShantyTailAndTeethEQProcessor)
};
