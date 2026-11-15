#include "HiseLookAndFeel.h"

#include <utility>

namespace
{
    juce::Font boldFont (float height)
    {
        return juce::Font (juce::FontOptions (height, juce::Font::bold));
    }

    bool hasFlag (const juce::Component& c, const char* name)
    {
        return (bool) c.getProperties().getWithDefault (name, false);
    }
}

//==============================================================================
HiseLookAndFeel::HiseLookAndFeel()
{
    using namespace sp16ui;

    // Slider read-outs
    setColour (juce::Slider::textBoxTextColourId,       accent);
    setColour (juce::Slider::textBoxBackgroundColourId, juce::Colour (0xff111214));
    setColour (juce::Slider::textBoxOutlineColourId,    juce::Colour (0xff3a3c42));
    setColour (juce::Slider::textBoxHighlightColourId,  accent.withAlpha (0.35f));
    setColour (juce::Slider::thumbColourId,             accent);
    setColour (juce::Slider::rotarySliderFillColourId,  accent);

    // Inline text editing
    setColour (juce::TextEditor::backgroundColourId,     juce::Colour (0xff111214));
    setColour (juce::TextEditor::textColourId,           juce::Colours::white);
    setColour (juce::TextEditor::highlightColourId,      accent.withAlpha (0.4f));
    setColour (juce::TextEditor::highlightedTextColourId, juce::Colours::white);
    setColour (juce::TextEditor::outlineColourId,        juce::Colour (0xff3a3c42));
    setColour (juce::TextEditor::focusedOutlineColourId, accent);
    setColour (juce::CaretComponent::caretColourId,      accent);

    // Combo box + popup menu
    setColour (juce::ComboBox::textColourId,       text);
    setColour (juce::ComboBox::arrowColourId,      accent);
    setColour (juce::PopupMenu::backgroundColourId,            juce::Colour (0xff1a1b1e));
    setColour (juce::PopupMenu::textColourId,                  text);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, accentDim);
    setColour (juce::PopupMenu::highlightedTextColourId,       juce::Colours::white);

    setColour (juce::Label::textColourId, text);
}

//==============================================================================
void HiseLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                                        float sliderPos, float startAngle, float endAngle,
                                        juce::Slider& slider)
{
    using namespace sp16ui;

    auto bounds = juce::Rectangle<float> ((float) x, (float) y, (float) width, (float) height).reduced (3.0f);
    const float dia = juce::jmin (bounds.getWidth(), bounds.getHeight());

    if (dia < 8.0f)
        return;

    const auto centre = bounds.getCentre();
    const float angle = startAngle + sliderPos * (endAngle - startAngle);
    const bool enabled = slider.isEnabled();
    const bool hot = slider.isMouseOverOrDragging() && enabled;

    const float arcW   = juce::jmax (3.0f, dia * 0.075f);
    const float arcR   = dia * 0.5f - arcW * 0.5f;
    const float bodyR  = dia * 0.5f - arcW - 4.0f;

    // --- Groove ---------------------------------------------------------------
    {
        juce::Path groove;
        groove.addCentredArc (centre.x, centre.y, arcR, arcR, 0.0f, startAngle, endAngle, true);

        g.setColour (juce::Colours::black.withAlpha (0.55f));
        g.strokePath (groove, juce::PathStrokeType (arcW + 2.0f, juce::PathStrokeType::curved,
                                                    juce::PathStrokeType::rounded));
        g.setColour (juce::Colour (0xff17181a));
        g.strokePath (groove, juce::PathStrokeType (arcW, juce::PathStrokeType::curved,
                                                    juce::PathStrokeType::rounded));
    }

    // --- Value arc (bipolar sliders fill from the centre) ---------------------
    {
        const bool bipolar = slider.getMinimum() < 0.0 && slider.getMaximum() > 0.0;
        const float zeroPos = bipolar ? (float) slider.valueToProportionOfLength (0.0) : 0.0f;

        float a0 = startAngle + zeroPos * (endAngle - startAngle);
        float a1 = angle;

        if (a1 < a0)
            std::swap (a0, a1);

        if (a1 - a0 > 0.001f)
        {
            juce::Path value;
            value.addCentredArc (centre.x, centre.y, arcR, arcR, 0.0f, a0, a1, true);

            const auto col = enabled ? (hot ? accent.brighter (0.25f) : accent) : textDim;

            g.setColour (col.withAlpha (0.22f));
            g.strokePath (value, juce::PathStrokeType (arcW + 5.0f, juce::PathStrokeType::curved,
                                                       juce::PathStrokeType::rounded));
            g.setColour (col);
            g.strokePath (value, juce::PathStrokeType (arcW, juce::PathStrokeType::curved,
                                                       juce::PathStrokeType::rounded));
        }
    }

    // --- Knob body ------------------------------------------------------------
    const auto body = juce::Rectangle<float> (bodyR * 2.0f, bodyR * 2.0f).withCentre (centre);

    {
        juce::Path p;
        p.addEllipse (body);
        juce::DropShadow (juce::Colours::black.withAlpha (0.7f), 7, { 0, 3 }).drawForPath (g, p);
    }

    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff62656c), body.getX(), body.getY(),
                                             juce::Colour (0xff1f2024), body.getRight(), body.getBottom(), false));
    g.fillEllipse (body);

    const auto face = body.reduced (bodyR * 0.16f);
    g.setGradientFill (juce::ColourGradient::vertical (juce::Colour (0xff2c2e33), face.getY(),
                                                       juce::Colour (0xff43464c), face.getBottom()));
    g.fillEllipse (face);

    g.setColour (juce::Colours::white.withAlpha (0.10f));
    g.drawEllipse (face, 1.0f);
    g.setColour (juce::Colours::black.withAlpha (0.85f));
    g.drawEllipse (body, 1.2f);

    // gloss highlight on the upper half of the face
    {
        auto gloss = face.reduced (2.0f).withTrimmedBottom (face.getHeight() * 0.5f);
        g.setGradientFill (juce::ColourGradient::vertical (juce::Colours::white.withAlpha (0.14f), gloss.getY(),
                                                           juce::Colours::white.withAlpha (0.0f), gloss.getBottom()));
        g.fillEllipse (gloss);
    }

    // --- Pointer --------------------------------------------------------------
    {
        juce::Path pointer;
        const float len = bodyR * 0.46f;
        pointer.addRoundedRectangle (-1.8f, -(bodyR - 3.5f), 3.6f, len, 1.8f);

        const auto xf = juce::AffineTransform::rotation (angle).translated (centre.x, centre.y);

        g.setColour ((enabled ? accent : textDim).withAlpha (0.35f));
        g.strokePath (pointer, juce::PathStrokeType (3.0f), xf);
        g.setColour (enabled ? accent.brighter (0.15f) : textDim);
        g.fillPath (pointer, xf);
    }
}

juce::Label* HiseLookAndFeel::createSliderTextBox (juce::Slider& slider)
{
    auto* l = LookAndFeel_V4::createSliderTextBox (slider);
    l->setJustificationType (juce::Justification::centred);
    l->setFont (boldFont (12.0f));
    return l;
}

//==============================================================================
void HiseLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& b, const juce::Colour&,
                                            bool highlighted, bool down)
{
    using namespace sp16ui;

    auto r = b.getLocalBounds().toFloat().reduced (1.0f);
    const float cr = 4.0f;
    const bool on    = b.getToggleState();
    const bool panic = hasFlag (b, "panic");
    const bool led   = hasFlag (b, "led");

    juce::Colour top, bottom;

    if (panic)       { top = danger.brighter (0.2f); bottom = danger.darker (0.55f); }
    else if (on)     { top = accent.brighter (0.2f); bottom = accentDim; }
    else             { top = juce::Colour (0xff4d5056); bottom = juce::Colour (0xff2b2c31); }

    if (highlighted) { top = top.brighter (0.12f); bottom = bottom.brighter (0.10f); }
    if (down)        { top = top.darker (0.35f);   bottom = bottom.darker (0.25f);   }

    // outer glow for lit buttons
    if (on || panic)
    {
        g.setColour ((panic ? danger : accent).withAlpha (down ? 0.10f : 0.20f));
        g.fillRoundedRectangle (r.expanded (2.0f), cr + 2.0f);
    }

    // drop shadow below the button
    if (! down)
    {
        g.setColour (juce::Colours::black.withAlpha (0.45f));
        g.fillRoundedRectangle (r.translated (0.0f, 1.5f), cr);
    }

    auto face = down ? r.translated (0.0f, 1.0f) : r;

    g.setGradientFill (juce::ColourGradient::vertical (top, face.getY(), bottom, face.getBottom()));
    g.fillRoundedRectangle (face, cr);

    // glossy upper half
    {
        auto gloss = face.reduced (1.0f).withTrimmedBottom (face.getHeight() * 0.5f);
        g.setGradientFill (juce::ColourGradient::vertical (juce::Colours::white.withAlpha (on || panic ? 0.30f : 0.14f),
                                                           gloss.getY(),
                                                           juce::Colours::white.withAlpha (0.0f), gloss.getBottom()));
        g.fillRoundedRectangle (gloss, cr);
    }

    g.setColour (outline);
    g.drawRoundedRectangle (face, cr, 1.2f);
    g.setColour (juce::Colours::white.withAlpha (0.10f));
    g.drawRoundedRectangle (face.reduced (1.0f), cr - 1.0f, 1.0f);

    // LED
    if (led)
    {
        auto ledArea = juce::Rectangle<float> (9.0f, 9.0f).withCentre ({ face.getX() + 14.0f, face.getCentreY() });
        drawLed (g, ledArea, ledGreen, on ? 1.0f : 0.0f);
    }
}

void HiseLookAndFeel::drawButtonText (juce::Graphics& g, juce::TextButton& b, bool, bool down)
{
    using namespace sp16ui;

    auto r = b.getLocalBounds().reduced (3, 1);

    if (hasFlag (b, "led"))
        r.removeFromLeft (20);

    const bool on    = b.getToggleState();
    const bool panic = hasFlag (b, "panic");

    juce::Colour c = panic ? juce::Colours::white
                           : (on ? juce::Colour (0xff1c1105) : text);

    g.setFont (getTextButtonFont (b, b.getHeight()));
    g.setColour (c.withMultipliedAlpha (b.isEnabled() ? 1.0f : 0.5f));
    g.drawText (b.getButtonText(), r.translated (0, down ? 1 : 0), juce::Justification::centred, false);
}

juce::Font HiseLookAndFeel::getTextButtonFont (juce::TextButton&, int)
{
    return boldFont (12.0f);
}

//==============================================================================
void HiseLookAndFeel::drawComboBox (juce::Graphics& g, int width, int height, bool,
                                    int, int, int, int, juce::ComboBox& box)
{
    using namespace sp16ui;

    auto r = juce::Rectangle<float> (0.0f, 0.0f, (float) width, (float) height).reduced (1.0f);
    const float cr = 4.0f;

    g.setGradientFill (juce::ColourGradient::vertical (juce::Colour (0xff0f1012), r.getY(),
                                                       juce::Colour (0xff222428), r.getBottom()));
    g.fillRoundedRectangle (r, cr);

    const bool hot = box.isMouseOver (true) || box.isPopupActive();
    g.setColour (hot ? accent.withAlpha (0.9f) : juce::Colour (0xff45474d));
    g.drawRoundedRectangle (r, cr, 1.2f);

    // arrow
    const float ax = (float) width - 16.0f;
    const float ay = (float) height * 0.5f;

    juce::Path arrow;
    arrow.addTriangle (ax - 4.5f, ay - 2.5f, ax + 4.5f, ay - 2.5f, ax, ay + 3.0f);
    g.setColour (box.isEnabled() ? accent : textDim);
    g.fillPath (arrow);
}

void HiseLookAndFeel::positionComboBoxText (juce::ComboBox& box, juce::Label& label)
{
    label.setBounds (8, 1, box.getWidth() - 30, box.getHeight() - 2);
    label.setFont (getComboBoxFont (box));
}

juce::Font HiseLookAndFeel::getComboBoxFont (juce::ComboBox&)
{
    return boldFont (13.0f);
}

juce::Font HiseLookAndFeel::getPopupMenuFont()
{
    return boldFont (13.0f);
}

//==============================================================================
void HiseLookAndFeel::drawPanel (juce::Graphics& g, juce::Rectangle<float> r, const juce::String& title)
{
    using namespace sp16ui;

    const float cr = 7.0f;

    {
        juce::Path p;
        p.addRoundedRectangle (r, cr);
        juce::DropShadow (juce::Colours::black.withAlpha (0.55f), 10, { 0, 4 }).drawForPath (g, p);
    }

    g.setGradientFill (juce::ColourGradient::vertical (panelTop, r.getY(), panelBot, r.getBottom()));
    g.fillRoundedRectangle (r, cr);

    // title strip
    auto strip = r.withHeight (26.0f);
    {
        juce::Path p;
        p.addRoundedRectangle (strip.getX(), strip.getY(), strip.getWidth(), strip.getHeight(),
                               cr, cr, true, true, false, false);
        g.setColour (juce::Colours::black.withAlpha (0.28f));
        g.fillPath (p);
    }

    g.setColour (juce::Colours::black.withAlpha (0.6f));
    g.drawHorizontalLine ((int) strip.getBottom(), r.getX() + 1.0f, r.getRight() - 1.0f);
    g.setColour (juce::Colours::white.withAlpha (0.06f));
    g.drawHorizontalLine ((int) strip.getBottom() + 1, r.getX() + 1.0f, r.getRight() - 1.0f);

    // accent tab + title
    g.setColour (accent);
    g.fillRoundedRectangle (strip.getX() + 10.0f, strip.getCentreY() - 5.0f, 3.0f, 10.0f, 1.5f);

    g.setColour (textDim.brighter (0.2f));
    g.setFont (boldFont (11.0f));
    g.drawText (title.toUpperCase(), strip.withTrimmedLeft (20.0f).toNearestInt(),
                juce::Justification::centredLeft, false);

    // frame
    g.setColour (outline);
    g.drawRoundedRectangle (r, cr, 1.2f);
    g.setColour (juce::Colours::white.withAlpha (0.07f));
    g.drawRoundedRectangle (r.reduced (1.0f), cr - 1.0f, 1.0f);
}

void HiseLookAndFeel::drawLed (juce::Graphics& g, juce::Rectangle<float> r, juce::Colour colour, float level)
{
    level = juce::jlimit (0.0f, 1.0f, level);

    if (level > 0.01f)
    {
        g.setColour (colour.withAlpha (0.30f * level));
        g.fillEllipse (r.expanded (4.0f));
        g.setColour (colour.withAlpha (0.25f * level));
        g.fillEllipse (r.expanded (2.0f));
    }

    const auto dark = juce::Colour (0xff1b1d1c);
    const auto lit  = dark.interpolatedWith (colour, level);

    g.setGradientFill (juce::ColourGradient (lit.brighter (0.5f * level), r.getX() + r.getWidth() * 0.3f,
                                             r.getY() + r.getHeight() * 0.25f,
                                             lit.darker (0.4f), r.getRight(), r.getBottom(), true));
    g.fillEllipse (r);

    g.setColour (juce::Colours::black.withAlpha (0.8f));
    g.drawEllipse (r, 1.0f);
}
