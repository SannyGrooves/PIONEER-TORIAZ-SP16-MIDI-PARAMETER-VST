#pragma once

#include <juce_audio_utils/juce_audio_utils.h>

//==============================================================================
/** Colour palette used by the whole editor (dark charcoal + HISE-style orange accent). */
namespace sp16ui
{
    inline const juce::Colour bgTop      { 0xff3a3c40 };
    inline const juce::Colour bgBottom   { 0xff1b1c1f };
    inline const juce::Colour panelTop   { 0xff35373c };
    inline const juce::Colour panelBot   { 0xff222327 };
    inline const juce::Colour outline    { 0xff0d0e10 };
    inline const juce::Colour accent     { 0xffff9f35 };
    inline const juce::Colour accentDim  { 0xffc06a14 };
    inline const juce::Colour text       { 0xffdadbde };
    inline const juce::Colour textDim    { 0xff8e9198 };
    inline const juce::Colour ledGreen   { 0xff86e64e };
    inline const juce::Colour danger     { 0xffd9453b };
}

//==============================================================================
/** A LookAndFeel that mimics the glossy, skeuomorphic controls found in HISE
    instruments: arc-style rotary knobs, glossy buttons with LEDs, inset combo boxes
    and bevelled panels. */
class HiseLookAndFeel : public juce::LookAndFeel_V4
{
public:
    HiseLookAndFeel();

    // Knobs
    void drawRotarySlider (juce::Graphics&, int x, int y, int width, int height,
                           float sliderPosProportional, float rotaryStartAngle,
                           float rotaryEndAngle, juce::Slider&) override;

    juce::Label* createSliderTextBox (juce::Slider&) override;

    // Buttons
    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour& backgroundColour,
                               bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;
    void drawButtonText (juce::Graphics&, juce::TextButton&,
                         bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;
    juce::Font getTextButtonFont (juce::TextButton&, int buttonHeight) override;

    // Combo box
    void drawComboBox (juce::Graphics&, int width, int height, bool isButtonDown,
                       int buttonX, int buttonY, int buttonW, int buttonH, juce::ComboBox&) override;
    void positionComboBoxText (juce::ComboBox&, juce::Label&) override;
    juce::Font getComboBoxFont (juce::ComboBox&) override;
    juce::Font getPopupMenuFont() override;

    // Helpers used directly by the editor
    /** Bevelled panel with a title strip. */
    static void drawPanel (juce::Graphics&, juce::Rectangle<float> area, const juce::String& title);

    /** Round LED with an optional glow. level is 0..1. */
    static void drawLed (juce::Graphics&, juce::Rectangle<float> area, juce::Colour colour, float level);

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HiseLookAndFeel)
};
