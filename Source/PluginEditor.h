#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "CustomLookAndFeel.h"
#include "ShantyMascot.h"
#include "SpectrumAnalyzerComponent.h"
#include <array>
#include <memory>

class ShantyTailAndTeethEQEditor : public juce::AudioProcessorEditor,
                                     private juce::Timer
{
public:
    explicit ShantyTailAndTeethEQEditor(ShantyTailAndTeethEQProcessor&);
    ~ShantyTailAndTeethEQEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void selectBand(int bandIndex);
    void setupKnob(juce::Slider& s, juce::Label& l, const juce::String& text, const juce::String& role, bool bipolar = false);

    ShantyTailAndTeethEQProcessor& processor;
    ShantyEQ::ShantyLookAndFeel lnf;

    juce::Image adamskyLogo;
    ShantyEQ::SpectrumAnalyzerComponent spectrum;
    ShantyEQ::ShantyMascotComponent mascot;

    // Top Split Engine controls
    juce::ComboBox sourceCombo;
    juce::Label sourceLabel;
    juce::Slider sepSlider, smoothSlider, decaySlider;
    juce::Label sepLabel, smoothLabel, decayLabel;

    juce::TextButton bypassBtn { "BYPASS" };
    juce::TextButton msBtn     { "M/S" };
    juce::TextButton teethSoloBtn { "SOLO TEETH" };
    juce::TextButton tailSoloBtn  { "SOLO TAIL" };

    // Graph edit target buttons
    juce::TextButton editTeethBtn { "TEETH" };
    juce::TextButton editTailBtn  { "TAIL" };
    juce::TextButton editBothBtn  { "BOTH" };

    // Right column global bus knobs
    juce::Slider teethLevelSlider, tailLevelSlider, mixSlider, outputSlider;
    juce::Label teethLevelLabel, tailLevelLabel, mixLabel, outputLabel;

    // Bottom Band Inspector
    int currentBand = 0;
    std::array<juce::TextButton, ShantyEQ::numBands> bandButtons;

    juce::ToggleButton bandEnabledBtn { "Active" };
    juce::ComboBox bandTypeCombo;
    juce::Label bandTypeLabel;
    juce::Slider bandFreqSlider, bandQSlider;
    juce::Label bandFreqLabel, bandQLabel;

    // Teeth (Transient) per-band
    juce::Slider teethGainSlider, teethPanSlider, teethSatDriveSlider;
    juce::Label teethGainLabel, teethPanLabel, teethSatDriveLabel, teethSatTypeLabel;
    juce::ComboBox teethSatTypeCombo;

    // Tail (Tonal) per-band
    juce::Slider tailGainSlider, tailPanSlider, tailSatDriveSlider;
    juce::Label tailGainLabel, tailPanLabel, tailSatDriveLabel, tailSatTypeLabel;
    juce::ComboBox tailSatTypeCombo;

    // Global APVTS Attachments
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> sourceAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> sepAtt, smoothAtt, decayAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> bypassAtt, msAtt, teethSoloAtt, tailSoloAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> teethLevelAtt, tailLevelAtt, mixAtt, outputAtt;

    // Per-band dynamic APVTS Attachments
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> bandEnabledAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> bandTypeAtt, teethSatTypeAtt, tailSatTypeAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> bandFreqAtt, bandQAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> teethGainAtt, teethPanAtt, teethSatDriveAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> tailGainAtt, tailPanAtt, tailSatDriveAtt;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ShantyTailAndTeethEQEditor)
};
