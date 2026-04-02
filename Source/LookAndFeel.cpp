#include "LookAndFeel.h"

namespace LFColors
{
    const juce::Colour background    { 0xff08080f };
    const juce::Colour surface       { 0xff111120 };
    const juce::Colour surfaceLight  { 0xff1a1a2e };
    const juce::Colour surfaceGlow   { 0xff222240 };
    const juce::Colour accentGold    { 0xfff5c518 };
    const juce::Colour accentRed     { 0xffe94560 };
    const juce::Colour accentCyan    { 0xff00d4ff };
    const juce::Colour accentPurple  { 0xff8b5cf6 };
    const juce::Colour textPrimary   { 0xfff0f0f0 };
    const juce::Colour textSecondary { 0xff556677 };
    const juce::Colour knobBg        { 0xff0c0c18 };
    const juce::Colour knobTrack     { 0xff1e1e35 };
}

VoidSynthLookAndFeel::VoidSynthLookAndFeel()
{
    setColour(juce::Slider::textBoxTextColourId, LFColors::textSecondary);
    setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour(juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
    setColour(juce::ComboBox::backgroundColourId, LFColors::surfaceLight);
    setColour(juce::ComboBox::textColourId, LFColors::textPrimary);
    setColour(juce::ComboBox::outlineColourId, juce::Colours::transparentBlack);
    setColour(juce::ComboBox::arrowColourId, LFColors::textSecondary);
    setColour(juce::PopupMenu::backgroundColourId, LFColors::surface);
    setColour(juce::PopupMenu::textColourId, LFColors::textPrimary);
    setColour(juce::PopupMenu::highlightedBackgroundColourId, LFColors::surfaceGlow);
    setColour(juce::PopupMenu::highlightedTextColourId, LFColors::accentGold);
    setColour(juce::TextButton::buttonColourId, LFColors::surfaceLight);
    setColour(juce::TextButton::textColourOnId, LFColors::textPrimary);
    setColour(juce::TextButton::textColourOffId, LFColors::textPrimary);
    setColour(juce::Label::textColourId, LFColors::textPrimary);

    setDefaultSansSerifTypeface(juce::Font(juce::Font::getDefaultSansSerifFontName(), 12.0f, juce::Font::plain).getTypefacePtr());
}

void VoidSynthLookAndFeel::drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                                             float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                                             juce::Slider& slider)
{
    auto bounds = juce::Rectangle<float>((float)x, (float)y, (float)width, (float)height);
    auto knobArea = bounds.reduced(4.0f);
    auto radius = juce::jmin(knobArea.getWidth(), knobArea.getHeight()) * 0.42f;
    auto centreX = knobArea.getCentreX();
    auto centreY = knobArea.getCentreY() - 2.0f;
    auto arcRadius = radius;
    auto toAngle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);

    auto fillColour = slider.findColour(juce::Slider::rotarySliderFillColourId);
    if (fillColour.isTransparent()) fillColour = LFColors::textPrimary;

    // Knob body - dark circle with subtle gradient
    {
        juce::ColourGradient bodyGrad(LFColors::surfaceLight, centreX, centreY - radius,
                                       LFColors::knobBg, centreX, centreY + radius, false);
        g.setGradientFill(bodyGrad);
        g.fillEllipse(centreX - radius * 0.65f, centreY - radius * 0.65f,
                       radius * 1.3f, radius * 1.3f);
    }

    // Background arc track
    {
        juce::Path bgArc;
        bgArc.addCentredArc(centreX, centreY, arcRadius, arcRadius, 0.0f,
                              rotaryStartAngle, rotaryEndAngle, true);
        g.setColour(LFColors::knobTrack);
        g.strokePath(bgArc, juce::PathStrokeType(3.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    // Neon glow behind value arc
    if (sliderPos > 0.01f)
    {
        juce::Path glowArc;
        glowArc.addCentredArc(centreX, centreY, arcRadius, arcRadius, 0.0f,
                                rotaryStartAngle, toAngle, true);

        // Outer glow (wider, more transparent)
        g.setColour(fillColour.withAlpha(0.15f));
        g.strokePath(glowArc, juce::PathStrokeType(10.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        // Mid glow
        g.setColour(fillColour.withAlpha(0.3f));
        g.strokePath(glowArc, juce::PathStrokeType(6.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        // Core value arc
        g.setColour(fillColour);
        g.strokePath(glowArc, juce::PathStrokeType(3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    // Pointer line from center
    {
        float lineLen = radius * 0.55f;
        float endX = centreX + lineLen * std::cos(toAngle - juce::MathConstants<float>::halfPi);
        float endY = centreY + lineLen * std::sin(toAngle - juce::MathConstants<float>::halfPi);
        float startX = centreX + radius * 0.2f * std::cos(toAngle - juce::MathConstants<float>::halfPi);
        float startY = centreY + radius * 0.2f * std::sin(toAngle - juce::MathConstants<float>::halfPi);

        g.setColour(fillColour.withAlpha(0.9f));
        g.drawLine(startX, startY, endX, endY, 2.0f);

        // Dot at the end
        g.setColour(fillColour);
        g.fillEllipse(endX - 2.5f, endY - 2.5f, 5.0f, 5.0f);

        // Dot glow
        g.setColour(fillColour.withAlpha(0.25f));
        g.fillEllipse(endX - 5.0f, endY - 5.0f, 10.0f, 10.0f);
    }
}

void VoidSynthLookAndFeel::drawButtonBackground(juce::Graphics& g, juce::Button& button,
                                                  const juce::Colour&,
                                                  bool isHighlighted, bool isDown)
{
    auto bounds = button.getLocalBounds().toFloat().reduced(1.0f);

    if (isDown)
    {
        juce::ColourGradient grad(LFColors::surfaceGlow, bounds.getCentreX(), bounds.getY(),
                                   LFColors::surfaceLight, bounds.getCentreX(), bounds.getBottom(), false);
        g.setGradientFill(grad);
    }
    else if (isHighlighted)
    {
        g.setColour(LFColors::surfaceGlow);
    }
    else
    {
        g.setColour(LFColors::surfaceLight);
    }
    g.fillRoundedRectangle(bounds, 5.0f);

    // Subtle border glow on hover
    if (isHighlighted || isDown)
    {
        g.setColour(categoryAccent.withAlpha(0.3f));
        g.drawRoundedRectangle(bounds, 5.0f, 1.0f);
    }
}

void VoidSynthLookAndFeel::drawComboBox(juce::Graphics& g, int width, int height, bool isDown,
                                         int, int, int, int, juce::ComboBox&)
{
    auto bounds = juce::Rectangle<float>(0, 0, (float)width, (float)height);

    // Dark fill with subtle gradient
    juce::ColourGradient grad(LFColors::surfaceGlow.withAlpha(0.5f), 0, 0,
                               LFColors::surfaceLight, 0, (float)height, false);
    g.setGradientFill(grad);
    g.fillRoundedRectangle(bounds, 4.0f);

    // Border
    g.setColour(isDown ? categoryAccent.withAlpha(0.5f) : LFColors::surfaceGlow);
    g.drawRoundedRectangle(bounds.reduced(0.5f), 4.0f, 1.0f);

    // Arrow
    float arrowX = (float)width - 14.0f;
    float arrowY = (float)height * 0.5f;
    juce::Path arrow;
    arrow.addTriangle(arrowX - 4.0f, arrowY - 2.5f,
                      arrowX + 4.0f, arrowY - 2.5f,
                      arrowX, arrowY + 3.0f);
    g.setColour(LFColors::textSecondary);
    g.fillPath(arrow);
}

void VoidSynthLookAndFeel::drawPopupMenuItem(juce::Graphics& g, const juce::Rectangle<int>& area,
                                              bool isSeparator, bool isActive, bool isHighlighted,
                                              bool isTicked, bool hasSubMenu,
                                              const juce::String& text, const juce::String&,
                                              const juce::Drawable*, const juce::Colour*)
{
    if (isSeparator)
    {
        auto sepArea = area.reduced(10, 0);
        g.setColour(LFColors::surfaceGlow);
        g.fillRect(sepArea.getX(), sepArea.getCentreY(), sepArea.getWidth(), 1);
        return;
    }

    if (isHighlighted)
    {
        juce::ColourGradient grad(categoryAccent.withAlpha(0.15f), (float)area.getX(), (float)area.getY(),
                                   LFColors::surfaceGlow, (float)area.getRight(), (float)area.getY(), false);
        g.setGradientFill(grad);
        g.fillRect(area);

        // Left accent bar
        g.setColour(categoryAccent);
        g.fillRect(area.getX(), area.getY(), 3, area.getHeight());
    }

    g.setColour(isHighlighted ? LFColors::textPrimary : (isActive ? LFColors::textPrimary : LFColors::textSecondary));
    g.setFont(juce::Font(13.0f));
    g.drawText(text, area.reduced(12, 0), juce::Justification::centredLeft);

    if (isTicked)
    {
        g.setColour(categoryAccent);
        g.fillEllipse((float)(area.getRight() - 16), (float)(area.getCentreY() - 3), 6.0f, 6.0f);
        g.setColour(categoryAccent.withAlpha(0.3f));
        g.fillEllipse((float)(area.getRight() - 19), (float)(area.getCentreY() - 6), 12.0f, 12.0f);
    }

    if (hasSubMenu)
    {
        float arrowX = (float)(area.getRight() - 10);
        float arrowY = (float)area.getCentreY();
        juce::Path arrow;
        arrow.addTriangle(arrowX - 3.0f, arrowY - 4.0f,
                          arrowX + 3.0f, arrowY,
                          arrowX - 3.0f, arrowY + 4.0f);
        g.setColour(LFColors::textSecondary);
        g.fillPath(arrow);
    }
}

void VoidSynthLookAndFeel::drawLabel(juce::Graphics& g, juce::Label& label)
{
    g.fillAll(label.findColour(juce::Label::backgroundColourId));

    auto textColour = label.findColour(juce::Label::textColourId);
    auto font = label.getFont();
    g.setColour(textColour);
    g.setFont(font);

    auto textArea = label.getBorderSize().subtractedFrom(label.getLocalBounds());
    g.drawText(label.getText(), textArea, label.getJustificationType(), true);
}
