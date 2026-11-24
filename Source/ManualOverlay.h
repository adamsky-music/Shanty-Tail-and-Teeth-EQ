#pragma once

#include <JuceHeader.h>
#include "CustomLookAndFeel.h"

namespace ShantyEQ
{

class ManualOverlayComponent : public juce::Component
{
public:
    ManualOverlayComponent()
    {
        closeButton.setColour(juce::TextButton::buttonColourId, Palette::teethRed.withAlpha(0.28f));
        closeButton.onClick = [this]() { setVisible(false); };
        addAndMakeVisible(closeButton);

        langButton.setClickingTogglesState(true);
        langButton.onClick = [this]() {
            isEnglish = langButton.getToggleState();
            langButton.setButtonText(isEnglish ? "RU" : "EN");
            updateText();
            repaint();
        };
        addAndMakeVisible(langButton);

        textEditor.setMultiLine(true);
        textEditor.setReadOnly(true);
        textEditor.setScrollbarsShown(true);
        textEditor.setCaretVisible(false);
        textEditor.setColour(juce::TextEditor::backgroundColourId, Palette::bgCanvas);
        textEditor.setColour(juce::TextEditor::outlineColourId, Palette::borderSubtle);
        textEditor.setColour(juce::TextEditor::textColourId, Palette::textPrimary);
        textEditor.setFont(juce::FontOptions(13.0f));
        addAndMakeVisible(textEditor);

        updateText();
    }

    void paint(juce::Graphics& g) override
    {
        g.fillAll(Palette::bgDark.withAlpha(0.92f));

        const auto card = getLocalBounds().reduced(24).toFloat();
        g.setColour(Palette::bgPanel);
        g.fillRoundedRectangle(card, 10.0f);
        g.setColour(Palette::borderSubtle);
        g.drawRoundedRectangle(card, 10.0f, 1.2f);

        g.setColour(Palette::teethRed);
        g.fillEllipse(card.getX() + 16.0f, card.getY() + 17.0f, 9.0f, 9.0f);

        g.setColour(Palette::textPrimary);
        g.setFont(juce::FontOptions(15.5f, juce::Font::bold));
        g.drawText(isEnglish ? juce::String::fromUTF8("SHANTY TAIL & TEETH EQ — MANUAL")
                             : juce::String::fromUTF8("SHANTY TAIL & TEETH EQ — ИНСТРУКЦИЯ"),
                   static_cast<int>(card.getX() + 32), static_cast<int>(card.getY() + 10),
                   420, 24, juce::Justification::centredLeft);
    }

    void resized() override
    {
        auto card = getLocalBounds().reduced(24);
        auto top = card.removeFromTop(42).reduced(12, 8);
        closeButton.setBounds(top.removeFromRight(68));
        top.removeFromRight(8);
        langButton.setBounds(top.removeFromRight(42));

        textEditor.setBounds(card.reduced(12, 8));
    }

private:
    bool isEnglish = false;
    juce::TextButton closeButton { "CLOSE" };
    juce::TextButton langButton { "EN" };
    juce::TextEditor textEditor;

    void updateText()
    {
        if (isEnglish)
        {
            textEditor.setText(juce::String::fromUTF8(
                "SHANTY TAIL & TEETH EQ v1.0.0 — by Adamsky Music\n"
                "Structural Split Equalizer with independent per-band Saturation for Transients (TEETH) and Tonal Body (TAIL).\n\n"
                "1. STRUCTURAL SPLIT ENGINE (TOP BAR)\n"
                "• SOURCE — Tunes transient/tonal detection for your material (Default, Drums, Bass, Vocal, Guitar, Keys, Mix).\n"
                "• SEPARATION — Sensitivity of the split. Higher values catch more micro-transients in TEETH; lower values isolate only sharp peaks.\n"
                "• SMOOTH — Frequency-axis mask smoothing to prevent spectral chatter.\n"
                "• DECAY — How fast the transient mask releases back into the tonal TAIL bus.\n"
                "• SOLO TEETH / SOLO TAIL — Audition only the separated transients or only the tonal body.\n"
                "• M/S — Switches all 8 bands & saturation into Mid/Side processing.\n\n"
                "2. SPECTRUM & 8-BAND GRAPH\n"
                "• Red spectrum & curve = TEETH (Transients). Cyan spectrum & curve = TAIL (Tonal).\n"
                "• Use the TEETH / TAIL / BOTH pills at the top-left of the graph to choose which curve you drag.\n"
                "• Drag any node 1–8 to adjust Frequency & Gain. Use the mouse wheel over a node to adjust Q.\n"
                "• An amber halo around a node indicates active per-band saturation.\n\n"
                "3. PER-BAND EQ & INDEPENDENT SATURATION (BOTTOM INSPECTOR)\n"
                "• BAND SHAPE — Toggle band Active, choose Filter Type (Low Cut, Low Shelf, Bell, Notch, High Shelf, High Cut), Freq, and Q.\n"
                "• TEETH (TRANSIENT) — Independent Gain (-24..+24 dB), Pan, and 2x-oversampled Saturation (Off, Tape, Tube, Soft Clip, Hard Clip + Drive).\n"
                "• TAIL (TONAL) — Independent Gain, Pan, and Saturation strictly for the tonal body of that frequency band.\n\n"
                "4. SHANTY MASCOT & GLOBAL BUS\n"
                "• TEETH LVL / TAIL LVL — Global balance between transients and tonal body.\n"
                "• MIX / OUTPUT — Latency-compensated Dry/Wet blend and final output trim.\n"
                "• Shanti's tail wags with tonal TAIL energy, her collar glows with TEETH & saturation bite — and you can pet her!"
            ));
        }
        else
        {
            textEditor.setText(juce::String::fromUTF8(
                "SHANTY TAIL & TEETH EQ v1.0.0 — Adamsky Music\n"
                "Эквалайзер со структурным разделением сигнала (Structural Split) и независимой поканальной сатурацией для транзиентов (TEETH) и тонального тела (TAIL).\n\n"
                "1. ДВИЖОК РАЗДЕЛЕНИЯ (ВЕРХНЯЯ ПАНЕЛЬ)\n"
                "• SOURCE — Пресет детектора под тип сигнала (Default, Drums, Bass, Vocal, Guitar, Keys, Mix).\n"
                "• SEPARATION — Чувствительность разделения. Выше значение — в TEETH попадает больше мелких атак; ниже — только самые резкие пики.\n"
                "• SMOOTH — Сглаживание спектральной маски по частоте для чистого, натурального звучания.\n"
                "• DECAY — Время затухания транзиентной маски обратно в тональную шину TAIL.\n"
                "• SOLO TEETH / SOLO TAIL — Прослушать отдельно только атаки (Teeth) или только тело звука (Tail).\n"
                "• M/S — Переключает все 8 полос EQ и сатурации в режим Mid/Side.\n\n"
                "2. АНАЛИЗАТОР СПЕКТРА И КРИВЫЕ EQ\n"
                "• Красный спектр и кривая — TEETH (транзиенты). Голубой спектр и кривая — TAIL (тональная часть).\n"
                "• Переключатель TEETH / TAIL / BOTH в левом верхнем углу графика выбирает, какую кривую вы двигаете мышью.\n"
                "• Перетаскивайте узлы 1–8 для изменения частоты и гейна; колёсико мыши над узлом меняет добротность Q.\n"
                "• Янтарное свечение вокруг узла означает, что на этой полосе включена сатурация.\n\n"
                "3. ПОЛОСА И НЕЗАВИСИМАЯ САТУРАЦИЯ (НИЖНЯЯ ПАНЕЛЬ)\n"
                "• BAND SHAPE — Включение полосы (Active), тип фильтра (Low Cut, Low Shelf, Bell, Notch, High Shelf, High Cut), Freq и Q.\n"
                "• TEETH (TRANSIENT) — Независимые Gain (-24..+24 дБ), Pan и сатурация с 2x-оверсэмплингом (Off, Tape, Tube, Soft Clip, Hard Clip + Drive) только для транзиентов этой полосы.\n"
                "• TAIL (TONAL) — Независимые Gain, Pan и сатурация только для тонального тела этой же полосы.\n\n"
                "4. ШАНТИ И ГЛОБАЛЬНАЯ ШИНА\n"
                "• TEETH LVL / TAIL LVL — Общий баланс громкости между транзиентами и тональной частью.\n"
                "• MIX / OUTPUT — Подмешивание Dry/Wet с компенсацией задержки и выходной уровень.\n"
                "• Хвост Шанти виляет от энергии TAIL, ошейник светится от атак TEETH и сатурации — а ещё её можно погладить!"
            ));
        }
    }
};

} // namespace ShantyEQ
