#include "LookAndFeel.h"

namespace LFColors
{
    const juce::Colour background   { 0xff0d0d14 };
    const juce::Colour surface      { 0xff161622 };
    const juce::Colour surfaceLight { 0xff1e1e30 };
    const juce::Colour accentGold   { 0xfff5c518 };
    const juce::Colour textPrimary  { 0xffeaeaea };
    const juce::Colour textSecondary{ 0xff667788 };
}

VoidSynthLookAndFeel::VoidSynthLookAndFeel()
{
    setColour(juce::Slider::textBoxTextColourId, LFColors::textSecondary);
    setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour(juce::ComboBox::backgroundColourId, LFColors::surfaceLight);
    setColour(juce::ComboBox::textColourId, LFColors::textPrimary);
    setColour(juce::ComboBox::outlineColourId, LFColors::surface);
    setColour(juce::ComboBox::arrowColourId, LFColors::textSecondary);
    setColour(juce::PopupMenu::backgroundColourId, LFColors::surface);
    setColour(juce::PopupMenu::textColourId, LFColors::textPrimary);
    setColour(juce::PopupMenu::highlightedBackgroundColourId, LFColors::surfaceLight);
    setColour(juce::PopupMenu::highlightedTextColourId, LFColors::accentGold);
    setColour(juce::TextButton::buttonColourId, LFColors::surfaceLight);
    setColour(juce::TextButton::textColourOnId, LFColors::textPrimary);
    setColour(juce::TextButton::textColourOffId, LFColors::textPrimary);
    setColour(juce::Label::textColourId, LFColors::textPrimary);
}

void VoidSynthLookAndFeel::drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                                             float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                                             juce::Slider& slider)
{
    auto bounds = juce::Rectangle<float>((float)x, (float)y, (float)width, (float)height);
    auto radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) * 0.4f;
    auto centreX = bounds.getCentreX();
    auto centreY = bounds.getCentreY() - 4.0f;
    auto arcRadius = radius;

    // Background arc
    juce::Path bgArc;
    bgArc.addCentredArc(centreX, centreY, arcRadius, arcRadius, 0.0f,
                          rotaryStartAngle, rotaryEndAngle, true);
    g.setColour(LFColors::surface);
    g.strokePath(bgArc, juce::PathStrokeType(3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // Value arc
    auto toAngle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);
    juce::Path valueArc;
    valueArc.addCentredArc(centreX, centreY, arcRadius, arcRadius, 0.0f,
                            rotaryStartAngle, toAngle, true);

    auto fillColour = slider.findColour(juce::Slider::rotarySliderFillColourId);
    if (fillColour == juce::Colour()) fillColour = LFColors::textPrimary;
    g.setColour(fillColour);
    g.strokePath(valueArc, juce::PathStrokeType(3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // Knob dot
    auto dotRadius = 2.5f;
    auto dotX = centreX + (arcRadius - 6.0f) * std::cos(toAngle - juce::MathConstants<float>::halfPi);
    auto dotY = centreY + (arcRadius - 6.0f) * std::sin(toAngle - juce::MathConstants<float>::halfPi);
    g.setColour(LFColors::textPrimary);
    g.fillEllipse(dotX - dotRadius, dotY - dotRadius, dotRadius * 2.0f, dotRadius * 2.0f);
}

void VoidSynthLookAndFeel::drawButtonBackground(juce::Graphics& g, juce::Button& button,
                                                  const juce::Colour&,
                                                  bool isHighlighted, bool isDown)
{
    auto bounds = button.getLocalBounds().toFloat().reduced(1.0f);
    auto baseColour = isDown ? LFColors::surfaceLight.brighter(0.1f)
                             : isHighlighted ? LFColors::surfaceLight
                                             : LFColors::surface;
    g.setColour(baseColour);
    g.fillRoundedRectangle(bounds, 4.0f);
}

void VoidSynthLookAndFeel::drawComboBox(juce::Graphics& g, int width, int height, bool,
                                         int, int, int, int, juce::ComboBox& box)
{
    auto bounds = juce::Rectangle<float>(0, 0, (float)width, (float)height);
    g.setColour(LFColors::surfaceLight);
    g.fillRoundedRectangle(bounds, 3.0f);

    // Arrow
    g.setColour(LFColors::textSecondary);
    auto arrowZone = juce::Rectangle<float>((float)width - 16.0f, 0, 14.0f, (float)height);
    juce::Path arrow;
    arrow.addTriangle(arrowZone.getCentreX() - 3.0f, arrowZone.getCentreY() - 2.0f,
                      arrowZone.getCentreX() + 3.0f, arrowZone.getCentreY() - 2.0f,
                      arrowZone.getCentreX(), arrowZone.getCentreY() + 3.0f);
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
        g.setColour(LFColors::surface.brighter(0.1f));
        g.fillRect(area.reduced(5, 0).withHeight(1));
        return;
    }

    if (isHighlighted)
    {
        g.setColour(LFColors::surfaceLight);
        g.fillRect(area);
    }

    g.setColour(isHighlighted ? LFColors::accentGold : (isActive ? LFColors::textPrimary : LFColors::textSecondary));
    g.setFont(juce::Font(13.0f));
    g.drawText(text, area.reduced(8, 0), juce::Justification::centredLeft);

    if (isTicked)
    {
        g.setColour(LFColors::accentGold);
        g.fillEllipse((float)(area.getRight() - 14), (float)(area.getCentreY() - 3), 6.0f, 6.0f);
    }

    if (hasSubMenu)
    {
        g.setColour(LFColors::textSecondary);
        g.drawText(">", area.reduced(4, 0), juce::Justification::centredRight);
    }
}
