#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
    constexpr float defaultBandFreqs[ShantyEQ::numBands] = {
        45.0f, 110.0f, 260.0f, 650.0f, 1500.0f, 3400.0f, 7500.0f, 14000.0f
    };

    constexpr int defaultBandTypes[ShantyEQ::numBands] = {
        1, // LowShelf
        2, // Bell
        2, // Bell
        2, // Bell
        2, // Bell
        2, // Bell
        2, // Bell
        4  // HighShelf
    };
}

ShantyTailAndTeethEQProcessor::ShantyTailAndTeethEQProcessor()
    : AudioProcessor(BusesProperties()
                         .withInput("Input", juce::AudioChannelSet::stereo(), true)
                         .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "Parameters", createParameterLayout())
{
    splitSourcePtr = apvts.getRawParameterValue("splitSource");
    splitSepPtr    = apvts.getRawParameterValue("splitSeparation");
    splitSmoothPtr = apvts.getRawParameterValue("splitSmoothness");
    splitDecayPtr  = apvts.getRawParameterValue("splitDecay");

    teethLevelPtr  = apvts.getRawParameterValue("teethLevel");
    tailLevelPtr   = apvts.getRawParameterValue("tailLevel");
    teethSoloPtr   = apvts.getRawParameterValue("teethSolo");
    tailSoloPtr    = apvts.getRawParameterValue("tailSolo");
    midSidePtr     = apvts.getRawParameterValue("midSideMode");
    mixPtr         = apvts.getRawParameterValue("mix");
    outputGainPtr  = apvts.getRawParameterValue("outputGain");
    bypassPtr      = apvts.getRawParameterValue("bypass");

    for (int i = 0; i < ShantyEQ::numBands; ++i)
    {
        auto& bp = bandPtrs[static_cast<size_t>(i)];
        bp.enabled       = apvts.getRawParameterValue(bandParamID(i, "enabled"));
        bp.type          = apvts.getRawParameterValue(bandParamID(i, "type"));
        bp.freq          = apvts.getRawParameterValue(bandParamID(i, "freq"));
        bp.q             = apvts.getRawParameterValue(bandParamID(i, "q"));
        bp.teethGain     = apvts.getRawParameterValue(bandParamID(i, "teethGain"));
        bp.teethPan      = apvts.getRawParameterValue(bandParamID(i, "teethPan"));
        bp.teethSatType  = apvts.getRawParameterValue(bandParamID(i, "teethSatType"));
        bp.teethSatDrive = apvts.getRawParameterValue(bandParamID(i, "teethSatDrive"));
        bp.tailGain      = apvts.getRawParameterValue(bandParamID(i, "tailGain"));
        bp.tailPan       = apvts.getRawParameterValue(bandParamID(i, "tailPan"));
        bp.tailSatType   = apvts.getRawParameterValue(bandParamID(i, "tailSatType"));
        bp.tailSatDrive  = apvts.getRawParameterValue(bandParamID(i, "tailSatDrive"));
    }
}

juce::AudioProcessorValueTreeState::ParameterLayout ShantyTailAndTeethEQProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    // Split Engine
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        "splitSource", "Source",
        juce::StringArray { "Default", "Drums", "Bass", "Vocal", "Guitar", "Keys", "Mix" }, 0));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "splitSeparation", "Separation",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 50.0f,
        juce::AudioParameterFloatAttributes().withLabel("%")));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "splitSmoothness", "Smoothness",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 35.0f,
        juce::AudioParameterFloatAttributes().withLabel("%")));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "splitDecay", "Transient Decay",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 30.0f,
        juce::AudioParameterFloatAttributes().withLabel("%")));

    // Global Buses
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "teethLevel", "Teeth Level",
        juce::NormalisableRange<float>(-24.0f, 24.0f, 0.1f), 0.0f,
        juce::AudioParameterFloatAttributes().withLabel("dB")));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "tailLevel", "Tail Level",
        juce::NormalisableRange<float>(-24.0f, 24.0f, 0.1f), 0.0f,
        juce::AudioParameterFloatAttributes().withLabel("dB")));

    params.push_back(std::make_unique<juce::AudioParameterBool>("teethSolo", "Solo Teeth", false));
    params.push_back(std::make_unique<juce::AudioParameterBool>("tailSolo", "Solo Tail", false));
    params.push_back(std::make_unique<juce::AudioParameterBool>("midSideMode", "Mid/Side Mode", false));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "mix", "Mix",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 100.0f,
        juce::AudioParameterFloatAttributes().withLabel("%")));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "outputGain", "Output",
        juce::NormalisableRange<float>(-24.0f, 24.0f, 0.1f), 0.0f,
        juce::AudioParameterFloatAttributes().withLabel("dB")));

    params.push_back(std::make_unique<juce::AudioParameterBool>("bypass", "Bypass", false));

    // 8 Parametric Bands
    const juce::StringArray filterTypes { "Low Cut", "Low Shelf", "Bell", "Notch", "High Shelf", "High Cut" };
    const juce::StringArray satTypes    { "Off", "Tape", "Tube", "Soft Clip", "Hard Clip" };

    for (int i = 0; i < ShantyEQ::numBands; ++i)
    {
        const juce::String prefix = "Band " + juce::String(i + 1) + " ";

        params.push_back(std::make_unique<juce::AudioParameterBool>(
            bandParamID(i, "enabled"), prefix + "Enabled", true));

        params.push_back(std::make_unique<juce::AudioParameterChoice>(
            bandParamID(i, "type"), prefix + "Type", filterTypes, defaultBandTypes[i]));

        juce::NormalisableRange<float> freqRange(20.0f, 20000.0f, 0.1f);
        freqRange.setSkewForCentre(1000.0f);
        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            bandParamID(i, "freq"), prefix + "Freq", freqRange, defaultBandFreqs[i],
            juce::AudioParameterFloatAttributes().withLabel("Hz")));

        juce::NormalisableRange<float> qRange(0.1f, 12.0f, 0.01f);
        qRange.setSkewForCentre(1.0f);
        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            bandParamID(i, "q"), prefix + "Q", qRange, 1.0f));

        // Teeth (Transient)
        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            bandParamID(i, "teethGain"), prefix + "Teeth Gain",
            juce::NormalisableRange<float>(-24.0f, 24.0f, 0.1f), 0.0f,
            juce::AudioParameterFloatAttributes().withLabel("dB")));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            bandParamID(i, "teethPan"), prefix + "Teeth Pan",
            juce::NormalisableRange<float>(-100.0f, 100.0f, 1.0f), 0.0f,
            juce::AudioParameterFloatAttributes().withLabel("%")));

        params.push_back(std::make_unique<juce::AudioParameterChoice>(
            bandParamID(i, "teethSatType"), prefix + "Teeth Sat Type", satTypes, 0));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            bandParamID(i, "teethSatDrive"), prefix + "Teeth Sat Drive",
            juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 0.0f,
            juce::AudioParameterFloatAttributes().withLabel("%")));

        // Tail (Tonal)
        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            bandParamID(i, "tailGain"), prefix + "Tail Gain",
            juce::NormalisableRange<float>(-24.0f, 24.0f, 0.1f), 0.0f,
            juce::AudioParameterFloatAttributes().withLabel("dB")));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            bandParamID(i, "tailPan"), prefix + "Tail Pan",
            juce::NormalisableRange<float>(-100.0f, 100.0f, 1.0f), 0.0f,
            juce::AudioParameterFloatAttributes().withLabel("%")));

        params.push_back(std::make_unique<juce::AudioParameterChoice>(
            bandParamID(i, "tailSatType"), prefix + "Tail Sat Type", satTypes, 0));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            bandParamID(i, "tailSatDrive"), prefix + "Tail Sat Drive",
            juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 0.0f,
            juce::AudioParameterFloatAttributes().withLabel("%")));
    }

    return { params.begin(), params.end() };
}

ShantyEQ::BandParams ShantyTailAndTeethEQProcessor::getBandParams(int bandIndex) const
{
    ShantyEQ::BandParams p;
    if (bandIndex < 0 || bandIndex >= ShantyEQ::numBands)
        return p;

    const auto& bp = bandPtrs[static_cast<size_t>(bandIndex)];
    p.enabled       = bp.enabled->load(std::memory_order_relaxed) > 0.5f;
    p.type          = static_cast<ShantyEQ::FilterType>(static_cast<int>(bp.type->load(std::memory_order_relaxed)));
    p.freq          = bp.freq->load(std::memory_order_relaxed);
    p.q             = bp.q->load(std::memory_order_relaxed);
    p.teethGainDb   = bp.teethGain->load(std::memory_order_relaxed);
    p.teethPan      = bp.teethPan->load(std::memory_order_relaxed) * 0.01f;
    p.teethSatType  = static_cast<ShantyEQ::SatType>(static_cast<int>(bp.teethSatType->load(std::memory_order_relaxed)));
    p.teethSatDrive = bp.teethSatDrive->load(std::memory_order_relaxed) * 0.01f;
    p.tailGainDb    = bp.tailGain->load(std::memory_order_relaxed);
    p.tailPan       = bp.tailPan->load(std::memory_order_relaxed) * 0.01f;
    p.tailSatType   = static_cast<ShantyEQ::SatType>(static_cast<int>(bp.tailSatType->load(std::memory_order_relaxed)));
    p.tailSatDrive  = bp.tailSatDrive->load(std::memory_order_relaxed) * 0.01f;
    return p;
}

void ShantyTailAndTeethEQProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate > 0.0 ? sampleRate : 44100.0;
    splitDSP.prepare(currentSampleRate);

    for (int i = 0; i < ShantyEQ::numBands; ++i)
    {
        teethBands[static_cast<size_t>(i)].reset();
        tailBands[static_cast<size_t>(i)].reset();
    }

    const int bufSize = juce::jmax(512, samplesPerBlock);
    teethBuffer.setSize(2, bufSize);
    tailBuffer.setSize(2, bufSize);

    const int latency = ShantyEQ::StructuralSplitDSP::getLatencySamples();
    dryDelayBuffer.setSize(2, latency + 64);
    dryDelayBuffer.clear();
    dryDelayWritePos = 0;

    setLatencySamples(latency);
}

void ShantyTailAndTeethEQProcessor::releaseResources()
{
}

bool ShantyTailAndTeethEQProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    const auto& mainIn  = layouts.getMainInputChannelSet();
    const auto& mainOut = layouts.getMainOutputChannelSet();
    if (mainIn != mainOut)
        return false;
    return (mainIn == juce::AudioChannelSet::mono() || mainIn == juce::AudioChannelSet::stereo());
}

void ShantyTailAndTeethEQProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    lastProcessMs.store(juce::Time::getMillisecondCounter(), std::memory_order_relaxed);

    const int totalIn  = getTotalNumInputChannels();
    const int totalOut = getTotalNumOutputChannels();
    for (int ch = totalIn; ch < totalOut; ++ch)
        buffer.clear(ch, 0, buffer.getNumSamples());

    const int numSamples = buffer.getNumSamples();
    if (numSamples <= 0 || totalIn <= 0)
        return;

    if (teethBuffer.getNumSamples() < numSamples)
    {
        teethBuffer.setSize(2, numSamples, false, false, true);
        tailBuffer.setSize(2, numSamples, false, false, true);
    }

    const bool isBypassed = bypassPtr->load(std::memory_order_relaxed) > 0.5f;
    const int latency = ShantyEQ::StructuralSplitDSP::getLatencySamples();
    const int delayLen = dryDelayBuffer.getNumSamples();

    float* mainL = buffer.getWritePointer(0);
    float* mainR = (totalIn > 1) ? buffer.getWritePointer(1) : nullptr;

    // 1. Store dry signal into latency-compensated circular buffer
    float* dryL = dryDelayBuffer.getWritePointer(0);
    float* dryR = dryDelayBuffer.getWritePointer(1);

    // 2. Run Structural Split Engine (STFT)
    ShantyEQ::SplitParams sp;
    sp.source     = static_cast<ShantyEQ::SourceType>(static_cast<int>(splitSourcePtr->load(std::memory_order_relaxed)));
    sp.separation = splitSepPtr->load(std::memory_order_relaxed) * 0.01f;
    sp.smoothness = splitSmoothPtr->load(std::memory_order_relaxed) * 0.01f;
    sp.decay      = splitDecayPtr->load(std::memory_order_relaxed) * 0.01f;

    float* tL = teethBuffer.getWritePointer(0);
    float* tR = teethBuffer.getWritePointer(1);
    float* bL = tailBuffer.getWritePointer(0);
    float* bR = tailBuffer.getWritePointer(1);

    splitDSP.processBlock(mainL, mainR, tL, tR, bL, bR, numSamples, sp);

    // Update Band EQ & Saturation coefficients
    float maxTeethDrive = 0.0f;
    float maxTailDrive  = 0.0f;
    for (int i = 0; i < ShantyEQ::numBands; ++i)
    {
        const auto bp = getBandParams(i);
        teethBands[static_cast<size_t>(i)].update(currentSampleRate, bp);
        tailBands[static_cast<size_t>(i)].update(currentSampleRate, bp);
        if (bp.enabled && bp.teethSatType != ShantyEQ::SatType::Off)
            maxTeethDrive = juce::jmax(maxTeethDrive, bp.teethSatDrive);
        if (bp.enabled && bp.tailSatType != ShantyEQ::SatType::Off)
            maxTailDrive = juce::jmax(maxTailDrive, bp.tailSatDrive);
    }

    const bool midSide = midSidePtr->load(std::memory_order_relaxed) > 0.5f;
    const float teethBusGain = juce::Decibels::decibelsToGain(teethLevelPtr->load(std::memory_order_relaxed));
    const float tailBusGain  = juce::Decibels::decibelsToGain(tailLevelPtr->load(std::memory_order_relaxed));
    const bool soloTeeth     = teethSoloPtr->load(std::memory_order_relaxed) > 0.5f;
    const bool soloTail      = tailSoloPtr->load(std::memory_order_relaxed) > 0.5f;
    const float mixNorm      = mixPtr->load(std::memory_order_relaxed) * 0.01f;
    const float outGain      = juce::Decibels::decibelsToGain(outputGainPtr->load(std::memory_order_relaxed));

    float teethMute = 1.0f;
    float tailMute  = 1.0f;
    if (soloTeeth && !soloTail)
        tailMute = 0.0f;
    else if (soloTail && !soloTeeth)
        teethMute = 0.0f;

    float peakTeeth = 0.0f;
    float peakTail  = 0.0f;

    constexpr float invSqrt2 = 0.7071067811865475f;

    for (int n = 0; n < numSamples; ++n)
    {
        // Push dry into delay & read delayed dry
        const float inSampleL = mainL[n];
        const float inSampleR = (mainR != nullptr) ? mainR[n] : inSampleL;

        dryL[dryDelayWritePos] = inSampleL;
        dryR[dryDelayWritePos] = inSampleR;

        int readIdx = dryDelayWritePos - latency;
        if (readIdx < 0)
            readIdx += delayLen;

        const float delayedDryL = dryL[readIdx];
        const float delayedDryR = dryR[readIdx];
        dryDelayWritePos = (dryDelayWritePos + 1) % delayLen;

        if (isBypassed)
        {
            mainL[n] = delayedDryL;
            if (mainR != nullptr)
                mainR[n] = delayedDryR;
            continue;
        }

        float trL = tL[n];
        float trR = tR[n];
        float tnL = bL[n];
        float tnR = bR[n];

        // Optional Mid/Side encode before per-band EQ & Sat
        if (midSide)
        {
            const float mTr = (trL + trR) * invSqrt2;
            const float sTr = (trL - trR) * invSqrt2;
            trL = mTr;
            trR = sTr;

            const float mTn = (tnL + tnR) * invSqrt2;
            const float sTn = (tnL - tnR) * invSqrt2;
            tnL = mTn;
            tnR = sTn;
        }

        for (int b = 0; b < ShantyEQ::numBands; ++b)
        {
            teethBands[static_cast<size_t>(b)].processBranch(trL, trR, true);
            tailBands[static_cast<size_t>(b)].processBranch(tnL, tnR, false);
        }

        if (midSide)
        {
            const float lTr = (trL + trR) * invSqrt2;
            const float rTr = (trL - trR) * invSqrt2;
            trL = lTr;
            trR = rTr;

            const float lTn = (tnL + tnR) * invSqrt2;
            const float rTn = (tnL - tnR) * invSqrt2;
            tnL = lTn;
            tnR = rTn;
        }

        trL *= teethBusGain * teethMute;
        trR *= teethBusGain * teethMute;
        tnL *= tailBusGain * tailMute;
        tnR *= tailBusGain * tailMute;

        peakTeeth = juce::jmax(peakTeeth, juce::jmax(std::abs(trL), std::abs(trR)));
        peakTail  = juce::jmax(peakTail,  juce::jmax(std::abs(tnL), std::abs(tnR)));

        const float wetL = (trL + tnL) * outGain;
        const float wetR = (trR + tnR) * outGain;

        mainL[n] = delayedDryL * (1.0f - mixNorm) + wetL * mixNorm;
        if (mainR != nullptr)
            mainR[n] = delayedDryR * (1.0f - mixNorm) + wetR * mixNorm;
    }

    const float prevTr = teethMeter.load(std::memory_order_relaxed);
    const float prevTn = tailMeter.load(std::memory_order_relaxed);
    teethMeter.store(0.65f * prevTr + 0.35f * juce::jlimit(0.0f, 1.0f, peakTeeth), std::memory_order_relaxed);
    tailMeter.store(0.65f * prevTn  + 0.35f * juce::jlimit(0.0f, 1.0f, peakTail), std::memory_order_relaxed);
    teethSatMeter.store(juce::jlimit(0.0f, 1.0f, peakTeeth * maxTeethDrive), std::memory_order_relaxed);
    tailSatMeter.store(juce::jlimit(0.0f, 1.0f, peakTail * maxTailDrive), std::memory_order_relaxed);
}

juce::AudioProcessorEditor* ShantyTailAndTeethEQProcessor::createEditor()
{
    return new ShantyTailAndTeethEQEditor(*this);
}

void ShantyTailAndTeethEQProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void ShantyTailAndTeethEQProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState != nullptr && xmlState->hasTagName(apvts.state.getType()))
        apvts.replaceState(juce::ValueTree::fromXml(*xmlState));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ShantyTailAndTeethEQProcessor();
}
