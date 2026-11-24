#include "PluginEditor.h"
#include "BinaryData.h"

ShantyTailAndTeethEQEditor::ShantyTailAndTeethEQEditor(ShantyTailAndTeethEQProcessor& p)
    : AudioProcessorEditor(&p),
      processor(p),
      spectrum(p)
{
    setLookAndFeel(&lnf);
    setSize(1080, 660);

    adamskyLogo = juce::ImageCache::getFromMemory(BinaryData::adamsky_logo_png,
                                                  BinaryData::adamsky_logo_pngSize);

    addAndMakeVisible(spectrum);
    addAndMakeVisible(mascot);

    spectrum.onBandSelected = [this](int idx) { selectBand(idx); };

    // Top Split Engine
    sourceCombo.addItemList({ "Default", "Drums", "Bass", "Vocal", "Guitar", "Keys", "Mix" }, 1);
    addAndMakeVisible(sourceCombo);
    sourceLabel.setText("SOURCE", juce::dontSendNotification);
    sourceLabel.setJustificationType(juce::Justification::centred);
    sourceLabel.setFont(10.5f);
    sourceLabel.setColour(juce::Label::textColourId, ShantyEQ::Palette::textSecondary);
    addAndMakeVisible(sourceLabel);

    setupKnob(sepSlider, sepLabel, "SEPARATION", "neutral");
    setupKnob(smoothSlider, smoothLabel, "SMOOTH", "neutral");
    setupKnob(decaySlider, decayLabel, "DECAY", "neutral");

    auto setupToggleBtn = [this](juce::TextButton& btn, juce::Colour onCol)
    {
        btn.setClickingTogglesState(true);
        btn.setColour(juce::TextButton::buttonOnColourId, onCol.withAlpha(0.32f));
        btn.setColour(juce::TextButton::textColourOnId, ShantyEQ::Palette::textPrimary);
        addAndMakeVisible(btn);
    };

    setupToggleBtn(bypassBtn, ShantyEQ::Palette::satAmber);
    setupToggleBtn(msBtn, ShantyEQ::Palette::tailCyan);
    setupToggleBtn(teethSoloBtn, ShantyEQ::Palette::teethRed);
    setupToggleBtn(tailSoloBtn, ShantyEQ::Palette::tailCyan);

    manualBtn.setColour(juce::TextButton::buttonColourId, ShantyEQ::Palette::bgCanvas);
    manualBtn.setColour(juce::TextButton::textColourOffId, ShantyEQ::Palette::tailCyan);
    manualBtn.onClick = [this] {
        manualOverlay.setVisible(true);
        manualOverlay.toFront(true);
    };
    addAndMakeVisible(manualBtn);

    // Edit Target buttons
    auto setupEditBtn = [this](juce::TextButton& btn, ShantyEQ::EditTarget target, juce::Colour col)
    {
        btn.setRadioGroupId(101);
        btn.setClickingTogglesState(true);
        btn.setColour(juce::TextButton::buttonOnColourId, col.withAlpha(0.35f));
        btn.setColour(juce::TextButton::textColourOnId, ShantyEQ::Palette::textPrimary);
        btn.onClick = [this, target] { spectrum.setEditTarget(target); };
        addAndMakeVisible(btn);
    };
    setupEditBtn(editTeethBtn, ShantyEQ::EditTarget::Teeth, ShantyEQ::Palette::teethRed);
    setupEditBtn(editTailBtn,  ShantyEQ::EditTarget::Tail,  ShantyEQ::Palette::tailCyan);
    setupEditBtn(editBothBtn,  ShantyEQ::EditTarget::Both,  ShantyEQ::Palette::satAmber);
    editTeethBtn.setToggleState(true, juce::dontSendNotification);

    // Global Bus Knobs
    setupKnob(teethLevelSlider, teethLevelLabel, "TEETH LVL", "teeth", true);
    setupKnob(tailLevelSlider,  tailLevelLabel,  "TAIL LVL",  "tail",  true);
    setupKnob(mixSlider,        mixLabel,        "MIX",       "neutral");
    setupKnob(outputSlider,     outputLabel,     "OUTPUT",    "neutral", true);

    // Band Selector Buttons
    for (int i = 0; i < ShantyEQ::numBands; ++i)
    {
        auto& btn = bandButtons[static_cast<size_t>(i)];
        btn.setButtonText("BAND " + juce::String(i + 1));
        btn.setRadioGroupId(202);
        btn.setClickingTogglesState(true);
        btn.onClick = [this, i] { selectBand(i); };
        addAndMakeVisible(btn);
    }

    // Band Shape Controls
    bandEnabledBtn.setColour(juce::ToggleButton::textColourId, ShantyEQ::Palette::textPrimary);
    addAndMakeVisible(bandEnabledBtn);

    bandTypeCombo.addItemList({ "Low Cut", "Low Shelf", "Bell", "Notch", "High Shelf", "High Cut" }, 1);
    addAndMakeVisible(bandTypeCombo);
    bandTypeLabel.setText("TYPE", juce::dontSendNotification);
    bandTypeLabel.setFont(10.5f);
    bandTypeLabel.setColour(juce::Label::textColourId, ShantyEQ::Palette::textSecondary);
    addAndMakeVisible(bandTypeLabel);

    setupKnob(bandFreqSlider, bandFreqLabel, "FREQ", "neutral");
    setupKnob(bandQSlider,    bandQLabel,    "Q",    "neutral");

    // Teeth Controls
    setupKnob(teethGainSlider,     teethGainLabel,     "GAIN",  "teeth", true);
    setupKnob(teethPanSlider,      teethPanLabel,      "PAN",   "teeth", true);
    setupKnob(teethSatDriveSlider, teethSatDriveLabel, "DRIVE", "sat");

    teethSatTypeCombo.addItemList({ "Off", "Tape", "Tube", "Soft Clip", "Hard Clip" }, 1);
    addAndMakeVisible(teethSatTypeCombo);
    teethSatTypeLabel.setText("SATURATION", juce::dontSendNotification);
    teethSatTypeLabel.setFont(10.5f);
    teethSatTypeLabel.setColour(juce::Label::textColourId, ShantyEQ::Palette::satAmber);
    addAndMakeVisible(teethSatTypeLabel);

    // Tail Controls
    setupKnob(tailGainSlider,     tailGainLabel,     "GAIN",  "tail", true);
    setupKnob(tailPanSlider,      tailPanLabel,      "PAN",   "tail", true);
    setupKnob(tailSatDriveSlider, tailSatDriveLabel, "DRIVE", "sat");

    tailSatTypeCombo.addItemList({ "Off", "Tape", "Tube", "Soft Clip", "Hard Clip" }, 1);
    addAndMakeVisible(tailSatTypeCombo);
    tailSatTypeLabel.setText("SATURATION", juce::dontSendNotification);
    tailSatTypeLabel.setFont(10.5f);
    tailSatTypeLabel.setColour(juce::Label::textColourId, ShantyEQ::Palette::satAmber);
    addAndMakeVisible(tailSatTypeLabel);

    // Global Attachments
    sourceAtt     = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(processor.apvts, "splitSource", sourceCombo);
    sepAtt        = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor.apvts, "splitSeparation", sepSlider);
    smoothAtt     = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor.apvts, "splitSmoothness", smoothSlider);
    decayAtt      = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor.apvts, "splitDecay", decaySlider);

    bypassAtt     = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(processor.apvts, "bypass", bypassBtn);
    msAtt         = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(processor.apvts, "midSideMode", msBtn);
    teethSoloAtt  = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(processor.apvts, "teethSolo", teethSoloBtn);
    tailSoloAtt   = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(processor.apvts, "tailSolo", tailSoloBtn);

    teethLevelAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor.apvts, "teethLevel", teethLevelSlider);
    tailLevelAtt  = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor.apvts, "tailLevel", tailLevelSlider);
    mixAtt        = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor.apvts, "mix", mixSlider);
    outputAtt     = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor.apvts, "outputGain", outputSlider);

    selectBand(0);
    addChildComponent(manualOverlay);
    startTimerHz(60);
}

ShantyTailAndTeethEQEditor::~ShantyTailAndTeethEQEditor()
{
    stopTimer();
    setLookAndFeel(nullptr);
}

void ShantyTailAndTeethEQEditor::setupKnob(juce::Slider& s, juce::Label& l, const juce::String& text, const juce::String& role, bool bipolar)
{
    s.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    s.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 58, 16);
    s.setColour(juce::Slider::textBoxTextColourId, ShantyEQ::Palette::textPrimary);
    s.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    s.getProperties().set("role", role);
    s.getProperties().set("bipolar", bipolar);
    addAndMakeVisible(s);

    l.setText(text, juce::dontSendNotification);
    l.setJustificationType(juce::Justification::centred);
    l.setFont(juce::Font(10.5f, juce::Font::bold));
    l.setColour(juce::Label::textColourId, ShantyEQ::Palette::textSecondary);
    addAndMakeVisible(l);
}

void ShantyTailAndTeethEQEditor::selectBand(int bandIndex)
{
    currentBand = juce::jlimit(0, ShantyEQ::numBands - 1, bandIndex);
    bandButtons[static_cast<size_t>(currentBand)].setToggleState(true, juce::dontSendNotification);
    spectrum.setSelectedBand(currentBand);

    // Reset attachments before rebinding
    bandEnabledAtt.reset();
    bandTypeAtt.reset();
    bandFreqAtt.reset();
    bandQAtt.reset();
    teethGainAtt.reset();
    teethPanAtt.reset();
    teethSatTypeAtt.reset();
    teethSatDriveAtt.reset();
    tailGainAtt.reset();
    tailPanAtt.reset();
    tailSatTypeAtt.reset();
    tailSatDriveAtt.reset();

    auto id = [this](const char* s) { return ShantyTailAndTeethEQProcessor::bandParamID(currentBand, s); };

    bandEnabledAtt   = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(processor.apvts, id("enabled"), bandEnabledBtn);
    bandTypeAtt      = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(processor.apvts, id("type"), bandTypeCombo);
    bandFreqAtt      = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor.apvts, id("freq"), bandFreqSlider);
    bandQAtt         = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor.apvts, id("q"), bandQSlider);

    teethGainAtt     = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor.apvts, id("teethGain"), teethGainSlider);
    teethPanAtt      = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor.apvts, id("teethPan"), teethPanSlider);
    teethSatTypeAtt  = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(processor.apvts, id("teethSatType"), teethSatTypeCombo);
    teethSatDriveAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor.apvts, id("teethSatDrive"), teethSatDriveSlider);

    tailGainAtt      = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor.apvts, id("tailGain"), tailGainSlider);
    tailPanAtt       = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor.apvts, id("tailPan"), tailPanSlider);
    tailSatTypeAtt   = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(processor.apvts, id("tailSatType"), tailSatTypeCombo);
    tailSatDriveAtt  = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor.apvts, id("tailSatDrive"), tailSatDriveSlider);
}

void ShantyTailAndTeethEQEditor::timerCallback()
{
    spectrum.updateSpectra();
    const bool isSleeping = bypassBtn.getToggleState() || (mixSlider.getValue() < 0.5);
    mascot.updateAnimation(processor.teethMeter.load(std::memory_order_relaxed),
                           processor.tailMeter.load(std::memory_order_relaxed),
                           processor.teethSatMeter.load(std::memory_order_relaxed),
                           processor.tailSatMeter.load(std::memory_order_relaxed),
                           isSleeping);
}

void ShantyTailAndTeethEQEditor::paint(juce::Graphics& g)
{
    g.fillAll(ShantyEQ::Palette::bgDark);

    // Header bar background
    g.setColour(ShantyEQ::Palette::bgPanel);
    g.fillRoundedRectangle(12.0f, 10.0f, static_cast<float>(getWidth() - 24), 60.0f, 8.0f);
    g.setColour(ShantyEQ::Palette::borderSubtle);
    g.drawRoundedRectangle(12.0f, 10.0f, static_cast<float>(getWidth() - 24), 60.0f, 8.0f, 1.0f);

    // Logo & Title
    int titleX = 24;
    if (adamskyLogo.isValid())
    {
        g.drawImageWithin(adamskyLogo, 22, 18, 42, 42, juce::RectanglePlacement::centred);
        titleX = 72;
    }

    g.setColour(ShantyEQ::Palette::textPrimary);
    g.setFont(juce::Font(17.5f, juce::Font::bold));
    g.drawText("SHANTY TAIL & TEETH EQ", titleX, 17, 250, 24, juce::Justification::centredLeft);

    g.setColour(ShantyEQ::Palette::textSecondary);
    g.setFont(11.0f);
    g.drawText(juce::String::fromUTF8("ADAMSKY MUSIC  \xe2\x80\xa2  STRUCTURAL SPLIT & SATURATION"), titleX, 41, 300, 16, juce::Justification::centredLeft);

    // Right Shanty Panel background
    const auto rightPanel = juce::Rectangle<float>(static_cast<float>(getWidth() - 252), 80.0f, 240.0f, 356.0f);
    g.setColour(ShantyEQ::Palette::bgPanel);
    g.fillRoundedRectangle(rightPanel, 8.0f);
    g.setColour(ShantyEQ::Palette::borderSubtle);
    g.drawRoundedRectangle(rightPanel, 8.0f, 1.0f);

    // Bottom Inspector Panel background
    const auto bottomPanel = juce::Rectangle<float>(12.0f, 446.0f, static_cast<float>(getWidth() - 24), 202.0f);
    g.setColour(ShantyEQ::Palette::bgPanel);
    g.fillRoundedRectangle(bottomPanel, 8.0f);
    g.setColour(ShantyEQ::Palette::borderSubtle);
    g.drawRoundedRectangle(bottomPanel, 8.0f, 1.0f);

    // Inspector sub-cards: Band Shape | TEETH (Transient) | TAIL (Tonal)
    const float cardY = 486.0f;
    const float cardH = 150.0f;
    const float card1W = 260.0f;
    const float card2W = 365.0f;
    const float card3W = 365.0f;

    auto drawCard = [&](float x, float w, const juce::String& title, juce::Colour accent)
    {
        const juce::Rectangle<float> r(x, cardY, w, cardH);
        g.setColour(ShantyEQ::Palette::bgCanvas.withAlpha(0.75f));
        g.fillRoundedRectangle(r, 6.0f);
        g.setColour(accent.withAlpha(0.45f));
        g.drawRoundedRectangle(r, 6.0f, 1.0f);

        g.setColour(accent);
        g.setFont(juce::Font(11.5f, juce::Font::bold));
        g.drawText(title, static_cast<int>(x + 12), static_cast<int>(cardY + 6), static_cast<int>(w - 24), 18, juce::Justification::centredLeft);
    };

    drawCard(24.0f,  card1W, "BAND SHAPE", ShantyEQ::Palette::textSecondary);
    drawCard(296.0f, card2W, "TEETH  (TRANSIENT)", ShantyEQ::Palette::teethRed);
    drawCard(673.0f, card3W, "TAIL  (TONAL)", ShantyEQ::Palette::tailCyan);
}

void ShantyTailAndTeethEQEditor::resized()
{
    // Header Split Engine controls
    sourceLabel.setBounds(375, 14, 96, 14);
    sourceCombo.setBounds(375, 31, 96, 26);

    auto placeTopKnob = [](juce::Slider& s, juce::Label& l, int x)
    {
        l.setBounds(x, 11, 66, 13);
        s.setBounds(x, 22, 66, 46);
    };
    placeTopKnob(sepSlider, sepLabel, 482);
    placeTopKnob(smoothSlider, smoothLabel, 554);
    placeTopKnob(decaySlider, decayLabel, 626);

    // Header right buttons
    teethSoloBtn.setBounds(704, 24, 86, 30);
    tailSoloBtn.setBounds(795, 24, 80, 30);
    msBtn.setBounds(880, 24, 48, 30);
    bypassBtn.setBounds(933, 24, 82, 30);
    manualBtn.setBounds(1020, 24, 34, 30);

    // Spectrum Analyzer (left-center)
    spectrum.setBounds(12, 80, getWidth() - 274, 356);

    // Floating Edit Target pills inside top-left of Spectrum
    editTeethBtn.setBounds(24, 90, 62, 22);
    editTailBtn.setBounds(90, 90, 56, 22);
    editBothBtn.setBounds(150, 90, 58, 22);

    // Right Panel: Shanty Mascot + Global Bus Knobs
    const int rightX = getWidth() - 252;
    mascot.setBounds(rightX + 14, 86, 212, 186);

    auto placeGlobalKnob = [](juce::Slider& s, juce::Label& l, int x, int y)
    {
        l.setBounds(x, y, 100, 14);
        s.setBounds(x + 12, y + 14, 76, 62);
    };
    placeGlobalKnob(teethLevelSlider, teethLevelLabel, rightX + 14,  274);
    placeGlobalKnob(tailLevelSlider,  tailLevelLabel,  rightX + 126, 274);
    placeGlobalKnob(mixSlider,        mixLabel,        rightX + 14,  354);
    placeGlobalKnob(outputSlider,     outputLabel,     rightX + 126, 354);

    // Bottom Inspector: Band Buttons
    const int btnW = (getWidth() - 48 - 7 * 6) / ShantyEQ::numBands;
    for (int i = 0; i < ShantyEQ::numBands; ++i)
        bandButtons[static_cast<size_t>(i)].setBounds(24 + i * (btnW + 6), 454, btnW, 24);

    // Card 1: Band Shape (x=24..284, y=486..636)
    bandEnabledBtn.setBounds(185, 490, 90, 22);
    bandTypeLabel.setBounds(36, 518, 86, 14);
    bandTypeCombo.setBounds(36, 536, 94, 26);

    bandFreqLabel.setBounds(136, 514, 70, 14);
    bandFreqSlider.setBounds(136, 528, 70, 96);

    bandQLabel.setBounds(208, 514, 68, 14);
    bandQSlider.setBounds(208, 528, 68, 96);

    // Card 2: TEETH (x=296..661, y=486..636)
    teethGainLabel.setBounds(308, 514, 76, 14);
    teethGainSlider.setBounds(308, 528, 76, 96);

    teethPanLabel.setBounds(388, 514, 76, 14);
    teethPanSlider.setBounds(388, 528, 76, 96);

    teethSatTypeLabel.setBounds(474, 518, 96, 14);
    teethSatTypeCombo.setBounds(474, 536, 96, 26);

    teethSatDriveLabel.setBounds(576, 514, 76, 14);
    teethSatDriveSlider.setBounds(576, 528, 76, 96);

    // Card 3: TAIL (x=673..1038, y=486..636)
    tailGainLabel.setBounds(685, 514, 76, 14);
    tailGainSlider.setBounds(685, 528, 76, 96);

    tailPanLabel.setBounds(765, 514, 76, 14);
    tailPanSlider.setBounds(765, 528, 76, 96);

    tailSatTypeLabel.setBounds(851, 518, 96, 14);
    tailSatTypeCombo.setBounds(851, 536, 96, 26);

    tailSatDriveLabel.setBounds(953, 514, 76, 14);
    tailSatDriveSlider.setBounds(953, 528, 76, 96);

    manualOverlay.setBounds(getLocalBounds());
}
