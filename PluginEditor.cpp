#include "PluginEditor.h"

namespace
{
    constexpr int editorWidth  = 760;
    constexpr int editorHeight = 600;
    constexpr int outerMargin  = 14;
    constexpr int panelGap     = 10;
    constexpr int titleStrip   = 26;
}

//==============================================================================
SP16Editor::SP16Editor (SP16Processor& p)
    : juce::AudioProcessorEditor (&p),
      proc (p),
      keyboard (p.keyboardState, juce::MidiKeyboardComponent::horizontalKeyboard)
{
    setLookAndFeel (&lnf);

    // ARP toggle (LED button) and PANIC
    arpToggle.setButtonText ("ARP");
    arpToggle.getProperties().set ("led", true);
    addAndMakeVisible (arpToggle);

    panicButton.setButtonText ("PANIC  -  ALL NOTES OFF");
    panicButton.getProperties().set ("panic", true);
    panicButton.onClick = [this]
    {
        for (int c = 1; c <= 16; ++c)
            proc.keyboardState.allNotesOff (c);

        proc.requestPanic();
    };
    addAndMakeVisible (panicButton);

    // Chord buttons
    for (size_t i = 0; i < chordButtons.size(); ++i)
    {
        auto& b = chordButtons[i];
        b.setButtonText (ids::chordNames[(int) i]);
        setupChoiceButton (b);
        b.onClick = [this, i]
        {
            setChoice (ids::chord, (int) i);
            refreshButtons();
        };
        addAndMakeVisible (b);
    }

    // Arp mode buttons
    for (size_t i = 0; i < modeButtons.size(); ++i)
    {
        auto& b = modeButtons[i];
        b.setButtonText (ids::arpModeNames[(int) i]);
        setupChoiceButton (b);
        b.onClick = [this, i]
        {
            setChoice (ids::arpMode, (int) i);
            refreshButtons();
        };
        addAndMakeVisible (b);
    }

    // Knobs
    setupKnob (channelKnob,   "MIDI Ch");
    setupKnob (transposeKnob, "Pitch Shift");
    setupKnob (bendKnob,      "Pitch Bend");
    setupKnob (octKnob,       "Arp Octaves");
    setupKnob (gateKnob,      "Arp Gate");

    // Arp rate combo
    rateBox.addItemList (ids::arpRateNames, 1);
    addAndMakeVisible (rateBox);
    setupCaption (rateCaption, "Arp Rate");
    setupCaption (modeCaption, "Arp Mode");

    // Hint (sits in the keyboard panel's title strip)
    hintLabel.setText ("Route MIDI out to the SP-16  |  click the keys to audition",
                       juce::dontSendNotification);
    hintLabel.setFont (juce::Font (juce::FontOptions (11.0f)));
    hintLabel.setColour (juce::Label::textColourId, sp16ui::textDim);
    hintLabel.setJustificationType (juce::Justification::centredRight);
    hintLabel.setInterceptsMouseClicks (false, false);
    addAndMakeVisible (hintLabel);

    // Keyboard
    keyboard.setLowestVisibleKey (36);
    keyboard.setAvailableRange (24, 108);
    keyboard.setKeyWidth (24.0f);
    keyboard.setBlackNoteLengthProportion (0.62f);
    keyboard.setScrollButtonsVisible (false);
    keyboard.setColour (juce::MidiKeyboardComponent::whiteNoteColourId,          juce::Colour (0xffe9e6df));
    keyboard.setColour (juce::MidiKeyboardComponent::blackNoteColourId,          juce::Colour (0xff17181a));
    keyboard.setColour (juce::MidiKeyboardComponent::keySeparatorLineColourId,   juce::Colour (0xff5b5d62));
    keyboard.setColour (juce::MidiKeyboardComponent::mouseOverKeyOverlayColourId, sp16ui::accent.withAlpha (0.25f));
    keyboard.setColour (juce::MidiKeyboardComponent::keyDownOverlayColourId,     sp16ui::accent.withAlpha (0.85f));
    keyboard.setColour (juce::MidiKeyboardComponent::textLabelColourId,          juce::Colour (0xff55575c));
    keyboard.setColour (juce::MidiKeyboardComponent::shadowColourId,             juce::Colours::black.withAlpha (0.5f));
    addAndMakeVisible (keyboard);

    // Attachments (created after the components they attach to)
    arpAtt       = std::make_unique<ButtonAtt> (proc.apvts, ids::arpOn, arpToggle);
    channelAtt   = std::make_unique<SliderAtt> (proc.apvts, ids::channel, channelKnob.slider);
    transposeAtt = std::make_unique<SliderAtt> (proc.apvts, ids::transpose, transposeKnob.slider);
    bendAtt      = std::make_unique<SliderAtt> (proc.apvts, ids::bend, bendKnob.slider);
    octAtt       = std::make_unique<SliderAtt> (proc.apvts, ids::arpOct, octKnob.slider);
    gateAtt      = std::make_unique<SliderAtt> (proc.apvts, ids::arpGate, gateKnob.slider);
    rateAtt      = std::make_unique<ComboAtt>  (proc.apvts, ids::arpRate, rateBox);

    // Custom read-outs. These must be set after the attachments, which install
    // their own text functions when they are constructed.
    auto numberFrom = [] (const juce::String& t) { return t.retainCharacters ("-+0123456789.").getDoubleValue(); };

    channelKnob.slider.textFromValueFunction = [] (double v) { return "CH " + juce::String (juce::roundToInt (v)); };
    channelKnob.slider.valueFromTextFunction = numberFrom;

    transposeKnob.slider.textFromValueFunction = [] (double v)
    {
        const int s = juce::roundToInt (v);
        return (s > 0 ? "+" : "") + juce::String (s) + " st";
    };
    transposeKnob.slider.valueFromTextFunction = numberFrom;

    bendKnob.slider.textFromValueFunction = [] (double v)
    {
        return (v > 0.0005 ? "+" : "") + juce::String (v, 2);
    };
    bendKnob.slider.valueFromTextFunction = numberFrom;

    octKnob.slider.textFromValueFunction = [] (double v) { return juce::String (juce::roundToInt (v)) + " oct"; };
    octKnob.slider.valueFromTextFunction = numberFrom;

    gateKnob.slider.textFromValueFunction = [] (double v) { return juce::String (juce::roundToInt (v * 100.0)) + " %"; };
    gateKnob.slider.valueFromTextFunction = [numberFrom] (const juce::String& t)
    {
        const double v = numberFrom (t);
        return v > 1.0 ? v / 100.0 : v;
    };

    for (auto* k : { &channelKnob, &transposeKnob, &bendKnob, &octKnob, &gateKnob })
        k->slider.updateText();

    setSize (editorWidth, editorHeight);
    refreshButtons();
    startTimerHz (30);
}

SP16Editor::~SP16Editor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

//==============================================================================
void SP16Editor::paint (juce::Graphics& g)
{
    using namespace sp16ui;

    // Background
    g.setGradientFill (juce::ColourGradient::vertical (bgTop, 0.0f, bgBottom, (float) getHeight()));
    g.fillAll();

    // Fine pin-stripes for a bit of texture
    g.setColour (juce::Colours::white.withAlpha (0.014f));
    for (int y = 0; y < getHeight(); y += 4)
        g.drawHorizontalLine (y, 0.0f, (float) getWidth());

    // Header
    {
        auto h = headerArea.reduced (4, 0);

        g.setColour (juce::Colours::black.withAlpha (0.6f));
        g.setFont (juce::Font (juce::FontOptions (30.0f, juce::Font::bold)));
        g.drawText ("SP-16", h.withWidth (110).translated (1, 2), juce::Justification::centredLeft, false);
        g.setColour (juce::Colours::white);
        g.drawText ("SP-16", h.withWidth (110), juce::Justification::centredLeft, false);

        g.setColour (accent);
        g.setFont (juce::Font (juce::FontOptions (12.0f, juce::Font::bold)));
        g.drawText ("MIDI CONTROLLER", h.withTrimmedLeft (106).withHeight (h.getHeight() / 2 + 4),
                    juce::Justification::bottomLeft, false);

        g.setColour (textDim);
        g.setFont (juce::Font (juce::FontOptions (10.0f)));
        g.drawText ("PIONEER DJ TORAIZ SP-16  |  VST3", h.withTrimmedLeft (106).withTrimmedTop (h.getHeight() / 2 + 6),
                    juce::Justification::topLeft, false);

        // Accent rule under the header
        g.setGradientFill (juce::ColourGradient::horizontal (accent, (float) headerArea.getX(),
                                                             accent.withAlpha (0.0f), (float) headerArea.getRight()));
        g.fillRect (headerArea.getX(), headerArea.getBottom() - 2, headerArea.getWidth(), 2);

        // MIDI activity LED
        auto led = juce::Rectangle<float> (10.0f, 10.0f).withCentre (
            { (float) midiLedArea.getX() + 8.0f, (float) midiLedArea.getCentreY() });
        HiseLookAndFeel::drawLed (g, led, ledGreen, midiGlow);

        g.setColour (textDim);
        g.setFont (juce::Font (juce::FontOptions (11.0f, juce::Font::bold)));
        g.drawText ("MIDI", midiLedArea.withTrimmedLeft (20), juce::Justification::centredLeft, false);
    }

    // Panels
    HiseLookAndFeel::drawPanel (g, outputPanel.toFloat(), "Output");
    HiseLookAndFeel::drawPanel (g, chordPanel.toFloat(),  "Chord");
    HiseLookAndFeel::drawPanel (g, arpPanel.toFloat(),    "Arpeggiator");
    HiseLookAndFeel::drawPanel (g, keyPanel.toFloat(),    "Keyboard");

    // Inset frame around the keyboard
    {
        auto kb = keyboard.getBounds().toFloat().expanded (3.0f);
        g.setColour (juce::Colours::black.withAlpha (0.65f));
        g.fillRoundedRectangle (kb, 4.0f);
        g.setColour (juce::Colours::white.withAlpha (0.08f));
        g.drawRoundedRectangle (kb, 4.0f, 1.0f);
    }
}

void SP16Editor::resized()
{
    auto area = getLocalBounds().reduced (outerMargin);

    // Header
    headerArea = area.removeFromTop (54);
    area.removeFromTop (panelGap);
    {
        auto h = headerArea.reduced (4, 0).withTrimmedBottom (4);
        panicButton.setBounds (h.removeFromRight (190).withSizeKeepingCentre (190, 30));
        h.removeFromRight (22);
        midiLedArea = h.removeFromRight (56).withSizeKeepingCentre (56, 20);
    }

    // Row 1: output knobs + chord buttons
    auto row1 = area.removeFromTop (150);
    outputPanel = row1.removeFromLeft (330);
    row1.removeFromLeft (panelGap);
    chordPanel = row1;
    area.removeFromTop (panelGap);

    // Row 2: arpeggiator
    arpPanel = area.removeFromTop (150);
    area.removeFromTop (panelGap);

    // Row 3: keyboard
    keyPanel = area;

    //--------------------------------------------------------------------------
    // Output knobs
    {
        auto c = outputPanel.withTrimmedTop (titleStrip).reduced (10, 6);
        const int w = c.getWidth() / 3;
        layoutKnob (channelKnob,   c.removeFromLeft (w));
        layoutKnob (transposeKnob, c.removeFromLeft (w));
        layoutKnob (bendKnob,      c);
    }

    // Chord buttons (two rows: 6 + 5)
    {
        auto c = chordPanel.withTrimmedTop (titleStrip).reduced (10, 6);
        c = c.withSizeKeepingCentre (c.getWidth(), 34 * 2 + 10);

        layoutButtons (chordButtons.data(),     6, 6, c.removeFromTop (34));
        c.removeFromTop (10);
        layoutButtons (chordButtons.data() + 6, 5, 6, c.removeFromTop (34));
    }

    // Arpeggiator
    {
        arpToggle.setBounds (arpPanel.getRight() - 112, arpPanel.getY() + 2, 104, titleStrip - 4);

        auto c = arpPanel.withTrimmedTop (titleStrip).reduced (10, 6);
        auto left = c.removeFromLeft (380);
        c.removeFromLeft (12);

        left = left.withSizeKeepingCentre (left.getWidth(), 96);

        modeCaption.setBounds (left.removeFromTop (14));
        layoutButtons (modeButtons.data(), 5, 5, left.removeFromTop (32));
        left.removeFromTop (8);

        rateCaption.setBounds (left.removeFromTop (14));
        rateBox.setBounds (left.removeFromTop (28).removeFromLeft (200).reduced (2, 0));

        const int w = c.getWidth() / 2;
        layoutKnob (octKnob,  c.removeFromLeft (w));
        layoutKnob (gateKnob, c);
    }

    // Keyboard
    {
        hintLabel.setBounds (keyPanel.getX() + 120, keyPanel.getY(), keyPanel.getWidth() - 130, titleStrip);
        keyboard.setBounds (keyPanel.withTrimmedTop (titleStrip).reduced (12, 10));
    }
}

//==============================================================================
void SP16Editor::timerCallback()
{
    // MIDI activity LED: lights while any note is held (on-screen keyboard or host input)
    bool anyNote = false;

    for (int n = 0; n < 128 && ! anyNote; ++n)
        anyNote = proc.keyboardState.isNoteOnForChannels (0xffff, n);

    const float previous = midiGlow;
    midiGlow = anyNote ? 1.0f : midiGlow * 0.80f;

    if (midiGlow < 0.02f)
        midiGlow = 0.0f;

    if (midiGlow != previous)
        repaint (midiLedArea.expanded (10));

    refreshButtons();
}

float SP16Editor::rawValue (const char* id) const
{
    const float v = *proc.apvts.getRawParameterValue (id);
    return v;
}

void SP16Editor::setChoice (const char* id, int index)
{
    if (auto* param = proc.apvts.getParameter (id))
    {
        param->beginChangeGesture();
        param->setValueNotifyingHost (param->convertTo0to1 ((float) index));
        param->endChangeGesture();
    }
}

void SP16Editor::refreshButtons()
{
    const int chordIdx = juce::roundToInt (rawValue (ids::chord));
    const int modeIdx  = juce::roundToInt (rawValue (ids::arpMode));

    for (size_t i = 0; i < chordButtons.size(); ++i)
        chordButtons[i].setToggleState ((int) i == chordIdx, juce::dontSendNotification);

    for (size_t i = 0; i < modeButtons.size(); ++i)
        modeButtons[i].setToggleState ((int) i == modeIdx, juce::dontSendNotification);
}

void SP16Editor::setupChoiceButton (juce::TextButton& b)
{
    b.setClickingTogglesState (false);
}

void SP16Editor::setupCaption (juce::Label& l, const juce::String& text)
{
    l.setText (text.toUpperCase(), juce::dontSendNotification);
    l.setFont (juce::Font (juce::FontOptions (10.5f, juce::Font::bold)));
    l.setColour (juce::Label::textColourId, sp16ui::textDim);
    l.setJustificationType (juce::Justification::centredLeft);
    l.setInterceptsMouseClicks (false, false);
    addAndMakeVisible (l);
}

void SP16Editor::setupKnob (Knob& k, const juce::String& caption)
{
    auto& s = k.slider;
    s.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    s.setRotaryParameters (juce::MathConstants<float>::pi * 1.2f,
                           juce::MathConstants<float>::pi * 2.8f, true);
    s.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 74, 18);
    s.setMouseDragSensitivity (160);
    addAndMakeVisible (s);

    k.label.setText (caption.toUpperCase(), juce::dontSendNotification);
    k.label.setFont (juce::Font (juce::FontOptions (10.5f, juce::Font::bold)));
    k.label.setColour (juce::Label::textColourId, sp16ui::textDim);
    k.label.setJustificationType (juce::Justification::centred);
    k.label.setInterceptsMouseClicks (false, false);
    addAndMakeVisible (k.label);
}

void SP16Editor::layoutKnob (Knob& k, juce::Rectangle<int> r)
{
    k.label.setBounds (r.removeFromTop (16));
    k.slider.setBounds (r);
}

void SP16Editor::layoutButtons (juce::TextButton* buttons, int count, int slots, juce::Rectangle<int> r)
{
    const int w = r.getWidth() / slots;

    for (int i = 0; i < count; ++i)
        buttons[i].setBounds (r.removeFromLeft (w).reduced (3, 0));
}
