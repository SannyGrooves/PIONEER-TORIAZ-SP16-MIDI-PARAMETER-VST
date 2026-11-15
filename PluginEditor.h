#pragma once

#include "HiseLookAndFeel.h"
#include "PluginProcessor.h"

#include <array>
#include <memory>

//==============================================================================
/** Editor window. Implementation lives in PluginEditor.cpp. */
class SP16Editor : public juce::AudioProcessorEditor,
                   private juce::Timer
{
public:
    explicit SP16Editor (SP16Processor& p);
    ~SP16Editor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    using SliderAtt = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAtt = juce::AudioProcessorValueTreeState::ButtonAttachment;
    using ComboAtt  = juce::AudioProcessorValueTreeState::ComboBoxAttachment;

    /** A rotary knob with a caption above it. */
    struct Knob
    {
        juce::Slider slider;
        juce::Label  label;
    };

    void timerCallback() override;

    float rawValue (const char* id) const;
    void setChoice (const char* id, int index);
    void refreshButtons();

    void setupKnob (Knob& k, const juce::String& caption);
    void setupChoiceButton (juce::TextButton& b);
    void setupCaption (juce::Label& l, const juce::String& text);

    static void layoutKnob (Knob& k, juce::Rectangle<int> r);
    static void layoutButtons (juce::TextButton* buttons, int count, int slots, juce::Rectangle<int> r);

    SP16Processor& proc;

    // Declared right after proc so it is destroyed after every component that uses it.
    HiseLookAndFeel lnf;

    juce::TextButton arpToggle;
    juce::TextButton panicButton;
    std::array<juce::TextButton, 11> chordButtons;
    std::array<juce::TextButton, 5> modeButtons;

    Knob channelKnob, transposeKnob, bendKnob, octKnob, gateKnob;

    juce::ComboBox rateBox;
    juce::Label rateCaption, modeCaption;
    juce::Label hintLabel;
    juce::MidiKeyboardComponent keyboard;

    // Layout areas, computed in resized() and used by paint().
    juce::Rectangle<int> headerArea, midiLedArea, outputPanel, chordPanel, arpPanel, keyPanel;

    float midiGlow = 0.0f;

    // Attachments last, so they are destroyed before the components they refer to.
    std::unique_ptr<ButtonAtt> arpAtt;
    std::unique_ptr<SliderAtt> channelAtt, transposeAtt, bendAtt, octAtt, gateAtt;
    std::unique_ptr<ComboAtt> rateAtt;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SP16Editor)
};
