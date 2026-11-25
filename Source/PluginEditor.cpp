#include "PluginEditor.h"
#include "BinaryData.h"

ShantyTailAndTeethEQEditor::ShantyTailAndTeethEQEditor(ShantyTailAndTeethEQProcessor& p)
    : AudioProcessorEditor(&p),
      processor(p),
      spectrum(p)
{
    setLookAndFeel(&lnf);
    setResizable(true, true);
    setResizeLimits(880, 560, 1920, 1200);
    setSize(1080, 660);

    // Load Adamsky Music heart logo and key out pure-black background
    {
        auto rawLogo = juce::ImageCache::getFromMemory(BinaryData::adamsky_logo_png,
                                                       BinaryData::adamsky_logo_pngSize);
        if (rawLogo.isValid())
        {
            const int lw = rawLogo.getWidth();
            const int lh = rawLogo.getHeight();
            adamskyLogo = juce::Image(juce::Image::ARGB, lw, lh, true);
            juce::Image::BitmapData src(rawLogo, juce::Image::BitmapData::readOnly);
            juce::Image::BitmapData dst(adamskyLogo, juce::Image::BitmapData::writeOnly);

            for (int y = 0; y < lh; ++y)
            {
                for (int x = 0; x < lw; ++x)
                {
                    const auto c = src.getPixelColour(x, y);
                    const int maxC = juce::jmax(static_cast<int>(c.getRed()),
                                                juce::jmax(static_cast<int>(c.getGreen()),
                                                           static_cast<int>(c.getBlue())));
                    if (maxC < 12)
                        dst.setPixelColour(x, y, juce::Colours::transparentBlack);
                    else if (maxC < 36)
                        dst.setPixelColour(x, y, c.withAlpha(static_cast<float>(maxC - 12) / 24.0f));
                    else
                        dst.setPixelColour(x, y, c);
                }
            }
        }
    }

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
    const uint32_t nowMs  = juce::Time::getMillisecondCounter();
    const uint32_t lastMs = processor.lastProcessMs.load(std::memory_order_relaxed);
    const bool dawBypassed = (lastMs != 0 && (nowMs - lastMs) > 350);
    const bool isSleeping  = bypassBtn.getToggleState() || (mixSlider.getValue() < 0.5) || dawBypassed;
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

    const float midH = static_cast<float>(juce::jmax(220, getHeight() - 304));
    const float bottomY = 80.0f + midH + 10.0f;

    // Right Shanty Panel background
    const auto rightPanel = juce::Rectangle<float>(static_cast<float>(getWidth() - 252), 80.0f, 240.0f, midH);
    g.setColour(ShantyEQ::Palette::bgPanel);
    g.fillRoundedRectangle(rightPanel, 8.0f);
    g.setColour(ShantyEQ::Palette::borderSubtle);
    g.drawRoundedRectangle(rightPanel, 8.0f, 1.0f);

    // Bottom Inspector Panel background
    const auto bottomPanel = juce::Rectangle<float>(12.0f, bottomY, static_cast<float>(getWidth() - 24), 202.0f);
    g.setColour(ShantyEQ::Palette::bgPanel);
    g.fillRoundedRectangle(bottomPanel, 8.0f);
    g.setColour(ShantyEQ::Palette::borderSubtle);
    g.drawRoundedRectangle(bottomPanel, 8.0f, 1.0f);

    // Inspector sub-cards: Band Shape | TEETH (Transient) | TAIL (Tonal)
    const float cardY = bottomY + 40.0f;
    const float cardH = 150.0f;
    const float totalCardsW = static_cast<float>(getWidth() - 48 - 24);
    const float card1W = totalCardsW * 0.26f;
    const float card2W = totalCardsW * 0.37f;
    const float card3W = totalCardsW * 0.37f;
    const float c1X = 24.0f;
    const float c2X = c1X + card1W + 12.0f;
    const float c3X = c2X + card2W + 12.0f;

    auto drawCard = [&](float x, float w, const juce::String& title, juce::Colour accent)
    {
        const juce::Rectangle<float> r(x, cardY, w, cardH);
        g.setColour(ShantyEQ::Palette::bgCanvas.withAlpha(0.75f));
        g.fillRoundedRectangle(r, 6.0f);
        g.setColour(accent.withAlpha(0.45f));
        g.drawRoundedRectangle(r, 6.0f, 1.0f);

        g.setColour(accent);
        g.setFont(juce::FontOptions(11.5f, juce::Font::bold));
        g.drawText(title, static_cast<int>(x + 12), static_cast<int>(cardY + 6), static_cast<int>(w - 24), 18, juce::Justification::centredLeft);
    };

    drawCard(c1X, card1W, "BAND SHAPE", ShantyEQ::Palette::textSecondary);
    drawCard(c2X, card2W, "TEETH  (TRANSIENT)", ShantyEQ::Palette::teethRed);
    drawCard(c3X, card3W, "TAIL  (TONAL)", ShantyEQ::Palette::tailCyan);
}

void ShantyTailAndTeethEQEditor::resized()
{
    const int w = getWidth();
    const int midH = juce::jmax(220, getHeight() - 304);
    const int bottomY = 80 + midH + 10;

    // Header Split Engine controls
    const int headerRightX = w - 376;
    sourceLabel.setBounds(juce::jmin(375, headerRightX - 330), 14, 96, 14);
    sourceCombo.setBounds(juce::jmin(375, headerRightX - 330), 31, 96, 26);

    auto placeTopKnob = [](juce::Slider& s, juce::Label& l, int x)
    {
        l.setBounds(x, 11, 64, 13);
        s.setBounds(x, 22, 64, 46);
    };
    const int knobStartX = sourceCombo.getRight() + 10;
    placeTopKnob(sepSlider, sepLabel, knobStartX);
    placeTopKnob(smoothSlider, smoothLabel, knobStartX + 68);
    placeTopKnob(decaySlider, decayLabel, knobStartX + 136);

    // Header right buttons (anchored to right edge)
    manualBtn.setBounds(w - 54, 24, 34, 30);
    bypassBtn.setBounds(w - 140, 24, 80, 30);
    msBtn.setBounds(w - 192, 24, 46, 30);
    tailSoloBtn.setBounds(w - 276, 24, 78, 30);
    teethSoloBtn.setBounds(w - 366, 24, 84, 30);

    // Spectrum Analyzer (left-center, stretches with window!)
    spectrum.setBounds(12, 80, w - 274, midH);

    // Floating Edit Target pills inside top-left of Spectrum
    editTeethBtn.setBounds(24, 90, 62, 22);
    editTailBtn.setBounds(90, 90, 56, 22);
    editBothBtn.setBounds(150, 90, 58, 22);

    // Right Panel: Shanty Mascot + Global Bus Knobs
    const int rightX = w - 252;
    const int mascotH = juce::jmax(110, midH - 170);
    mascot.setBounds(rightX + 14, 86, 212, mascotH);

    const int globalY = 86 + mascotH + 4;
    auto placeGlobalKnob = [](juce::Slider& s, juce::Label& l, int x, int y)
    {
        l.setBounds(x, y, 100, 14);
        s.setBounds(x + 12, y + 14, 76, 60);
    };
    placeGlobalKnob(teethLevelSlider, teethLevelLabel, rightX + 14,  globalY);
    placeGlobalKnob(tailLevelSlider,  tailLevelLabel,  rightX + 126, globalY);
    placeGlobalKnob(mixSlider,        mixLabel,        rightX + 14,  globalY + 78);
    placeGlobalKnob(outputSlider,     outputLabel,     rightX + 126, globalY + 78);

    // Bottom Inspector: Band Buttons
    const int btnW = (w - 48 - 7 * 6) / ShantyEQ::numBands;
    for (int i = 0; i < ShantyEQ::numBands; ++i)
        bandButtons[static_cast<size_t>(i)].setBounds(24 + i * (btnW + 6), bottomY + 8, btnW, 24);

    // Bottom Inspector Cards (proportional)
    const int cardY = bottomY + 40;
    const int totalCardsW = w - 48 - 24;
    const int card1W = static_cast<int>(totalCardsW * 0.26f);
    const int card2W = static_cast<int>(totalCardsW * 0.37f);
    const int card3W = static_cast<int>(totalCardsW * 0.37f);
    const int c1X = 24;
    const int c2X = c1X + card1W + 12;
    const int c3X = c2X + card2W + 12;

    // Card 1: Band Shape
    bandEnabledBtn.setBounds(c1X + card1W - 86, cardY + 4, 80, 22);
    bandTypeLabel.setBounds(c1X + 10, cardY + 32, 86, 14);
    bandTypeCombo.setBounds(c1X + 10, cardY + 50, juce::jmax(76, card1W / 3), 26);

    const int c1KnobW = juce::jmin(72, (card1W - 110) / 2);
    bandFreqLabel.setBounds(c1X + card1W - c1KnobW * 2 - 14, cardY + 28, c1KnobW, 14);
    bandFreqSlider.setBounds(c1X + card1W - c1KnobW * 2 - 14, cardY + 42, c1KnobW, 96);
    bandQLabel.setBounds(c1X + card1W - c1KnobW - 8, cardY + 28, c1KnobW, 14);
    bandQSlider.setBounds(c1X + card1W - c1KnobW - 8, cardY + 42, c1KnobW, 96);

    auto layoutBranchCard = [&](int cx, int cw,
                                juce::Slider& gS, juce::Label& gL,
                                juce::Slider& pS, juce::Label& pL,
                                juce::ComboBox& satC, juce::Label& satCL,
                                juce::Slider& dS, juce::Label& dL)
    {
        const int kw = juce::jmin(76, (cw - 126) / 3);
        gL.setBounds(cx + 10, cardY + 28, kw, 14);
        gS.setBounds(cx + 10, cardY + 42, kw, 96);

        pL.setBounds(cx + 16 + kw, cardY + 28, kw, 14);
        pS.setBounds(cx + 16 + kw, cardY + 42, kw, 96);

        const int satX = cx + 24 + kw * 2;
        const int comboW = juce::jmax(80, cw - kw * 3 - 42);
        satCL.setBounds(satX, cardY + 32, comboW, 14);
        satC.setBounds(satX, cardY + 50, comboW, 26);

        dL.setBounds(cx + cw - kw - 10, cardY + 28, kw, 14);
        dS.setBounds(cx + cw - kw - 10, cardY + 42, kw, 96);
    };

    layoutBranchCard(c2X, card2W, teethGainSlider, teethGainLabel, teethPanSlider, teethPanLabel, teethSatTypeCombo, teethSatTypeLabel, teethSatDriveSlider, teethSatDriveLabel);
    layoutBranchCard(c3X, card3W, tailGainSlider, tailGainLabel, tailPanSlider, tailPanLabel, tailSatTypeCombo, tailSatTypeLabel, tailSatDriveSlider, tailSatDriveLabel);

    manualOverlay.setBounds(getLocalBounds());
}
