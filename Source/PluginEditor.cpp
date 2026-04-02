#include "PluginEditor.h"

// Color palette
namespace Colors
{
    const juce::Colour background    { 0xff08080f };
    const juce::Colour surface       { 0xff111120 };
    const juce::Colour surfaceLight  { 0xff1a1a2e };
    const juce::Colour surfaceGlow   { 0xff222240 };
    const juce::Colour panelBorder   { 0xff2a2a45 };
    const juce::Colour accentRed     { 0xffe94560 };
    const juce::Colour accentCyan    { 0xff00d4ff };
    const juce::Colour accentPurple  { 0xff8b5cf6 };
    const juce::Colour accentGold    { 0xfff5c518 };
    const juce::Colour accentGreen   { 0xff00e676 };
    const juce::Colour textPrimary   { 0xfff0f0f0 };
    const juce::Colour textSecondary { 0xff556677 };
    const juce::Colour textDim       { 0xff334455 };
}

VoidSynthAudioProcessorEditor::VoidSynthAudioProcessorEditor(VoidSynthAudioProcessor& p)
    : AudioProcessorEditor(&p), processor(p)
{
    setSize(750, 580);
    setLookAndFeel(&lookAndFeel);

    // Preset navigation
    addAndMakeVisible(prevPresetBtn);
    addAndMakeVisible(nextPresetBtn);
    addAndMakeVisible(menuBtn);
    addAndMakeVisible(presetNameLabel);
    addAndMakeVisible(categoryLabel);

    presetNameLabel.setJustificationType(juce::Justification::centred);
    presetNameLabel.setColour(juce::Label::textColourId, Colors::textPrimary);
    presetNameLabel.setFont(juce::Font(15.0f, juce::Font::bold));

    categoryLabel.setJustificationType(juce::Justification::centred);
    categoryLabel.setFont(juce::Font(9.0f, juce::Font::bold));

    prevPresetBtn.onClick = [this] { processor.presetManager.loadPrevPreset(); updatePresetDisplay(); };
    nextPresetBtn.onClick = [this] { processor.presetManager.loadNextPreset(); updatePresetDisplay(); };
    menuBtn.onClick = [this] { showPresetMenu(); };

    // Macro knobs — hero elements, big and gold
    auto setupMacroKnob = [this](juce::Slider& knob, juce::Label& nameLabel, juce::Label& valLabel,
                                  const juce::String& paramId,
                                  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>& attach,
                                  const juce::String& labelText)
    {
        knob.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        knob.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        knob.setColour(juce::Slider::rotarySliderFillColourId, Colors::accentGold);
        addAndMakeVisible(knob);
        attach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor.apvts, paramId, knob);

        setupLabel(nameLabel, labelText, 10.0f);
        nameLabel.setColour(juce::Label::textColourId, Colors::accentGold.withAlpha(0.8f));
        nameLabel.setFont(juce::Font(10.0f, juce::Font::bold));
        addAndMakeVisible(nameLabel);

        setupLabel(valLabel, "", 9.0f);
        valLabel.setColour(juce::Label::textColourId, Colors::textSecondary);
        addAndMakeVisible(valLabel);
    };

    setupMacroKnob(macro1Knob, macro1Label, macro1ValLabel, "macro1", macro1Attach, "MACRO 1");
    setupMacroKnob(macro2Knob, macro2Label, macro2ValLabel, "macro2", macro2Attach, "MACRO 2");
    setupMacroKnob(macro3Knob, macro3Label, macro3ValLabel, "macro3", macro3Attach, "MACRO 3");
    setupMacroKnob(macro4Knob, macro4Label, macro4ValLabel, "macro4", macro4Attach, "MACRO 4");

    // Osc A
    oscAWaveformBox.addItemList({"SAW", "SQR", "TRI", "SIN", "WT"}, 1);
    addAndMakeVisible(oscAWaveformBox);
    oscAWaveformAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(processor.apvts, "oscAWaveform", oscAWaveformBox);
    setupSmallKnob(oscALevelKnob, Colors::textPrimary);
    oscALevelAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor.apvts, "oscALevel", oscALevelKnob);

    // Osc B
    oscBWaveformBox.addItemList({"SAW", "SQR", "TRI", "SIN", "WT"}, 1);
    addAndMakeVisible(oscBWaveformBox);
    oscBWaveformAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(processor.apvts, "oscBWaveform", oscBWaveformBox);
    setupSmallKnob(oscBLevelKnob, Colors::textPrimary);
    oscBLevelAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor.apvts, "oscBLevel", oscBLevelKnob);

    // Granular
    setupSmallKnob(granLevelKnob, Colors::accentCyan);
    granLevelAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor.apvts, "granLevel", granLevelKnob);
    setupSmallKnob(granDensityKnob, Colors::accentCyan);
    granDensityAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor.apvts, "granDensity", granDensityKnob);

    // Sub
    setupSmallKnob(subLevelKnob, Colors::textPrimary);
    subLevelAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor.apvts, "subLevel", subLevelKnob);

    // Filter
    filterTypeBox.addItemList({"LP12", "LP24", "HP12", "BP", "NOTCH"}, 1);
    addAndMakeVisible(filterTypeBox);
    filterTypeAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(processor.apvts, "filterType", filterTypeBox);
    setupSmallKnob(filterCutoffKnob, Colors::accentPurple);
    filterCutoffAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor.apvts, "filterCutoff", filterCutoffKnob);
    setupSmallKnob(filterResKnob, Colors::accentPurple);
    filterResAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor.apvts, "filterRes", filterResKnob);
    setupSmallKnob(filterEnvKnob, Colors::accentPurple);
    filterEnvAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor.apvts, "filterEnvAmt", filterEnvKnob);
    setupSmallKnob(filterDriveKnob, Colors::accentPurple);
    filterDriveAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor.apvts, "filterDrive", filterDriveKnob);

    // Distortion
    distModeBox.addItemList({"SOFT", "HARD", "FOLD", "CRUSH", "RECT"}, 1);
    addAndMakeVisible(distModeBox);
    distModeAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(processor.apvts, "distMode", distModeBox);
    setupSmallKnob(distDriveKnob, Colors::accentRed);
    distDriveAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor.apvts, "distDrive", distDriveKnob);
    setupSmallKnob(distMixKnob, Colors::accentRed);
    distMixAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor.apvts, "distMix", distMixKnob);

    // Amp Envelope
    setupSmallKnob(ampAKnob, Colors::accentGreen);
    ampAAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor.apvts, "ampA", ampAKnob);
    setupSmallKnob(ampDKnob, Colors::accentGreen);
    ampDAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor.apvts, "ampD", ampDKnob);
    setupSmallKnob(ampSKnob, Colors::accentGreen);
    ampSAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor.apvts, "ampS", ampSKnob);
    setupSmallKnob(ampRKnob, Colors::accentGreen);
    ampRAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor.apvts, "ampR", ampRKnob);

    // FX Knobs
    setupSmallKnob(chorusRateKnob, Colors::accentCyan);
    chorusRateAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor.apvts, "chorusRate", chorusRateKnob);
    setupSmallKnob(chorusDepthKnob, Colors::accentCyan);
    chorusDepthAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor.apvts, "chorusDepth", chorusDepthKnob);
    setupSmallKnob(chorusMixKnob, Colors::accentCyan);
    chorusMixAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor.apvts, "chorusMix", chorusMixKnob);

    setupSmallKnob(delayTimeKnob, Colors::accentCyan);
    delayTimeAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor.apvts, "delayTime", delayTimeKnob);
    setupSmallKnob(delayFBKnob, Colors::accentCyan);
    delayFBAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor.apvts, "delayFeedback", delayFBKnob);
    setupSmallKnob(delayMixKnob, Colors::accentCyan);
    delayMixAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor.apvts, "delayMix", delayMixKnob);

    setupSmallKnob(reverbSizeKnob, Colors::accentPurple);
    reverbSizeAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor.apvts, "reverbSize", reverbSizeKnob);
    setupSmallKnob(reverbDampKnob, Colors::accentPurple);
    reverbDampAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor.apvts, "reverbDamp", reverbDampKnob);
    setupSmallKnob(reverbMixKnob, Colors::accentPurple);
    reverbMixAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor.apvts, "reverbMix", reverbMixKnob);

    // Footer
    voiceModeBox.addItemList({"POLY", "MONO", "LEGATO"}, 1);
    addAndMakeVisible(voiceModeBox);
    voiceModeAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(processor.apvts, "voiceMode", voiceModeBox);
    setupSmallKnob(glideKnob, Colors::textPrimary);
    glideAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor.apvts, "glideTime", glideKnob);
    setupSmallKnob(masterVolKnob, Colors::accentGold);
    masterVolAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor.apvts, "masterVol", masterVolKnob);

    updatePresetDisplay();
    startTimerHz(30);
}

VoidSynthAudioProcessorEditor::~VoidSynthAudioProcessorEditor()
{
    setLookAndFeel(nullptr);
}

void VoidSynthAudioProcessorEditor::setupKnob(juce::Slider& knob, juce::Colour accentColour)
{
    knob.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    knob.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    knob.setColour(juce::Slider::rotarySliderFillColourId, accentColour);
}

void VoidSynthAudioProcessorEditor::setupSmallKnob(juce::Slider& knob, juce::Colour accentColour)
{
    setupKnob(knob, accentColour);
    addAndMakeVisible(knob);
}

void VoidSynthAudioProcessorEditor::setupLabel(juce::Label& label, const juce::String& text, float fontSize)
{
    label.setText(text, juce::dontSendNotification);
    label.setJustificationType(juce::Justification::centred);
    label.setColour(juce::Label::textColourId, Colors::textSecondary);
    label.setFont(juce::Font(fontSize, juce::Font::bold));
}

void VoidSynthAudioProcessorEditor::drawSection(juce::Graphics& g, juce::Rectangle<float> bounds,
                                                  const juce::String& title, juce::Colour accent)
{
    // Panel background with gradient
    juce::ColourGradient panelGrad(Colors::surface.brighter(0.05f), bounds.getCentreX(), bounds.getY(),
                                    Colors::surface, bounds.getCentreX(), bounds.getBottom(), false);
    g.setGradientFill(panelGrad);
    g.fillRoundedRectangle(bounds, 8.0f);

    // Border with accent-colored top edge
    g.setColour(Colors::panelBorder);
    g.drawRoundedRectangle(bounds.reduced(0.5f), 8.0f, 1.0f);

    // Accent top bar (thin glow line)
    juce::Path topBar;
    topBar.addRoundedRectangle(bounds.getX() + 12.0f, bounds.getY(), bounds.getWidth() - 24.0f, 2.0f, 1.0f);
    g.setColour(accent.withAlpha(0.6f));
    g.fillPath(topBar);

    // Glow behind top bar
    g.setColour(accent.withAlpha(0.1f));
    g.fillRoundedRectangle(bounds.getX() + 8.0f, bounds.getY() - 2.0f, bounds.getWidth() - 16.0f, 8.0f, 3.0f);

    // Title
    g.setColour(accent.withAlpha(0.7f));
    g.setFont(juce::Font(9.0f, juce::Font::bold));
    g.drawText(title, bounds.getX() + 10.0f, bounds.getY() + 4.0f, bounds.getWidth() - 20.0f, 14.0f,
               juce::Justification::centredLeft);
}

void VoidSynthAudioProcessorEditor::drawHeaderBar(juce::Graphics& g)
{
    // Header background with gradient
    juce::ColourGradient headerGrad(Colors::surfaceLight, 0, 0,
                                     Colors::surface, 0, 48.0f, false);
    g.setGradientFill(headerGrad);
    g.fillRect(0, 0, getWidth(), 48);

    // Bottom border glow
    g.setColour(categoryColour.withAlpha(0.4f));
    g.fillRect(0, 46, getWidth(), 2);
    g.setColour(categoryColour.withAlpha(0.08f));
    g.fillRect(0, 40, getWidth(), 8);

    // Logo
    g.setColour(Colors::textPrimary);
    g.setFont(juce::Font(20.0f, juce::Font::bold));
    g.drawText("VOID", 14, 4, 55, 22, juce::Justification::centredLeft);
    g.setColour(categoryColour);
    g.drawText("SYNTH", 62, 4, 65, 22, juce::Justification::centredLeft);

    // Subtle tagline
    g.setColour(Colors::textDim);
    g.setFont(juce::Font(8.0f));
    g.drawText("BY TOMMY TRILL AI", 14, 26, 120, 12, juce::Justification::centredLeft);
}

void VoidSynthAudioProcessorEditor::drawMacroBackground(juce::Graphics& g)
{
    auto macroArea = juce::Rectangle<float>(10.0f, 54.0f, (float)getWidth() - 20.0f, 150.0f);

    // Dark background with subtle texture feel
    juce::ColourGradient bgGrad(Colors::surface.brighter(0.03f), macroArea.getCentreX(), macroArea.getY(),
                                 Colors::background, macroArea.getCentreX(), macroArea.getBottom(), false);
    g.setGradientFill(bgGrad);
    g.fillRoundedRectangle(macroArea, 10.0f);

    // Gold accent border
    g.setColour(Colors::accentGold.withAlpha(0.15f));
    g.drawRoundedRectangle(macroArea.reduced(0.5f), 10.0f, 1.0f);

    // Top gold glow line
    g.setColour(Colors::accentGold.withAlpha(0.3f));
    g.fillRoundedRectangle(macroArea.getX() + 20.0f, macroArea.getY(), macroArea.getWidth() - 40.0f, 2.0f, 1.0f);

    // "MACROS" label
    g.setColour(Colors::accentGold.withAlpha(0.4f));
    g.setFont(juce::Font(8.0f, juce::Font::bold));
    g.drawText("PERFORMANCE MACROS", macroArea.getX() + 10.0f, macroArea.getY() + 4.0f,
               150.0f, 12.0f, juce::Justification::centredLeft);
}

void VoidSynthAudioProcessorEditor::paint(juce::Graphics& g)
{
    // Deep dark background
    g.fillAll(Colors::background);

    // Subtle noise/texture (diagonal lines pattern)
    g.setColour(juce::Colour(0x03ffffff));
    for (int i = -getHeight(); i < getWidth() + getHeight(); i += 4)
    {
        g.drawLine((float)i, 0.0f, (float)(i + getHeight()), (float)getHeight(), 0.5f);
    }

    drawHeaderBar(g);
    drawMacroBackground(g);

    // Section panels
    float row2Y = 212.0f;
    float row2H = 115.0f;
    float row3Y = 335.0f;
    float row3H = 115.0f;

    // Engine strip sections
    drawSection(g, { 10, row2Y, 95, row2H }, "OSC A", Colors::textSecondary);
    drawSection(g, { 112, row2Y, 95, row2H }, "OSC B", Colors::textSecondary);
    drawSection(g, { 214, row2Y, 115, row2H }, "GRAIN", Colors::accentCyan);
    drawSection(g, { 336, row2Y, 65, row2H }, "SUB", Colors::textSecondary);
    drawSection(g, { 408, row2Y, 330, row2H }, "FILTER", Colors::accentPurple);

    // Bottom strip sections
    drawSection(g, { 10, row3Y, 148, row3H }, "DISTORTION", Colors::accentRed);
    drawSection(g, { 165, row3Y, 148, row3H }, "AMP ENV", Colors::accentGreen);
    drawSection(g, { 320, row3Y, 200, row3H }, "CHORUS / DELAY", Colors::accentCyan);
    drawSection(g, { 527, row3Y, 211, row3H }, "REVERB", Colors::accentPurple);

    // Footer bar
    g.setColour(Colors::surface);
    g.fillRect(0, 458, getWidth(), 122);
    g.setColour(Colors::panelBorder);
    g.drawLine(0, 458, (float)getWidth(), 458, 1.0f);

    // Knob labels for sections
    g.setFont(juce::Font(8.0f));
    g.setColour(Colors::textDim);

    // Osc A labels
    g.drawText("LVL", 30, 290, 40, 10, juce::Justification::centred);

    // Osc B labels
    g.drawText("LVL", 132, 290, 40, 10, juce::Justification::centred);

    // Granular labels
    g.drawText("LVL", 225, 290, 40, 10, juce::Justification::centred);
    g.drawText("DENS", 270, 290, 40, 10, juce::Justification::centred);

    // Sub labels
    g.drawText("LVL", 347, 290, 40, 10, juce::Justification::centred);

    // Filter labels
    g.drawText("CUT", 430, 290, 40, 10, juce::Justification::centred);
    g.drawText("RES", 485, 290, 40, 10, juce::Justification::centred);
    g.drawText("ENV", 540, 290, 40, 10, juce::Justification::centred);
    g.drawText("DRV", 595, 290, 40, 10, juce::Justification::centred);

    // Distortion labels
    g.drawText("DRV", 30, 413, 40, 10, juce::Justification::centred);
    g.drawText("MIX", 85, 413, 40, 10, juce::Justification::centred);

    // Amp env labels
    g.drawText("A", 178, 413, 25, 10, juce::Justification::centred);
    g.drawText("D", 213, 413, 25, 10, juce::Justification::centred);
    g.drawText("S", 248, 413, 25, 10, juce::Justification::centred);
    g.drawText("R", 283, 413, 25, 10, juce::Justification::centred);

    // FX labels
    g.drawText("RATE", 332, 413, 36, 10, juce::Justification::centred);
    g.drawText("DPTH", 372, 413, 36, 10, juce::Justification::centred);
    g.drawText("MIX", 412, 413, 36, 10, juce::Justification::centred);
    g.drawText("TIME", 452, 413, 36, 10, juce::Justification::centred);
    g.drawText("FB", 492, 413, 36, 10, juce::Justification::centred);

    // Reverb labels
    g.drawText("SIZE", 547, 413, 40, 10, juce::Justification::centred);
    g.drawText("DAMP", 597, 413, 40, 10, juce::Justification::centred);
    g.drawText("MIX", 647, 413, 40, 10, juce::Justification::centred);

    // Footer labels
    g.setColour(Colors::textSecondary);
    g.setFont(juce::Font(9.0f, juce::Font::bold));
    g.drawText("VOICE", 15, 462, 60, 14, juce::Justification::centredLeft);
    g.drawText("GLIDE", 130, 462, 60, 14, juce::Justification::centredLeft);
    g.drawText("MASTER", 630, 462, 70, 14, juce::Justification::centredLeft);
}

void VoidSynthAudioProcessorEditor::resized()
{
    // Header
    prevPresetBtn.setBounds(260, 10, 32, 28);
    presetNameLabel.setBounds(296, 6, 200, 22);
    categoryLabel.setBounds(296, 26, 200, 14);
    nextPresetBtn.setBounds(500, 10, 32, 28);
    menuBtn.setBounds(660, 10, 75, 28);

    // Macro knobs — big, centered
    int macroKnobSize = 72;
    int macroStartX = 30;
    int macroSpacing = 175;
    int macroY = 80;

    auto placeMacro = [&](juce::Slider& knob, juce::Label& label, juce::Label& valLabel, int index)
    {
        int x = macroStartX + index * macroSpacing;
        label.setBounds(x, macroY - 16, macroKnobSize, 14);
        knob.setBounds(x, macroY, macroKnobSize, macroKnobSize);
        valLabel.setBounds(x, macroY + macroKnobSize - 2, macroKnobSize, 14);
    };
    placeMacro(macro1Knob, macro1Label, macro1ValLabel, 0);
    placeMacro(macro2Knob, macro2Label, macro2ValLabel, 1);
    placeMacro(macro3Knob, macro3Label, macro3ValLabel, 2);
    placeMacro(macro4Knob, macro4Label, macro4ValLabel, 3);

    // Engine strip
    int engY = 230;
    int ks = 40; // small knob size

    // Osc A
    oscAWaveformBox.setBounds(18, engY, 78, 20);
    oscALevelKnob.setBounds(30, engY + 24, ks, ks);

    // Osc B
    oscBWaveformBox.setBounds(120, engY, 78, 20);
    oscBLevelKnob.setBounds(132, engY + 24, ks, ks);

    // Granular
    granLevelKnob.setBounds(225, engY + 24, ks, ks);
    granDensityKnob.setBounds(270, engY + 24, ks, ks);

    // Sub
    subLevelKnob.setBounds(347, engY + 24, ks, ks);

    // Filter
    filterTypeBox.setBounds(418, engY, 85, 20);
    filterCutoffKnob.setBounds(430, engY + 24, ks, ks);
    filterResKnob.setBounds(485, engY + 24, ks, ks);
    filterEnvKnob.setBounds(540, engY + 24, ks, ks);
    filterDriveKnob.setBounds(595, engY + 24, ks, ks);

    // Bottom strip
    int botY = 352;

    // Distortion
    distModeBox.setBounds(18, botY, 85, 20);
    distDriveKnob.setBounds(30, botY + 24, ks, ks);
    distMixKnob.setBounds(85, botY + 24, ks, ks);

    // Amp Env
    ampAKnob.setBounds(178, botY + 24, 30, 30);
    ampDKnob.setBounds(213, botY + 24, 30, 30);
    ampSKnob.setBounds(248, botY + 24, 30, 30);
    ampRKnob.setBounds(283, botY + 24, 30, 30);

    // FX — chorus + delay
    int fxKS = 34;
    chorusRateKnob.setBounds(332, botY + 24, fxKS, fxKS);
    chorusDepthKnob.setBounds(372, botY + 24, fxKS, fxKS);
    chorusMixKnob.setBounds(412, botY + 24, fxKS, fxKS);
    delayTimeKnob.setBounds(452, botY + 24, fxKS, fxKS);
    delayFBKnob.setBounds(492, botY + 24, fxKS, fxKS);

    // Reverb
    reverbSizeKnob.setBounds(547, botY + 24, ks, ks);
    reverbDampKnob.setBounds(597, botY + 24, ks, ks);
    reverbMixKnob.setBounds(647, botY + 24, ks, ks);

    // Delay mix goes in reverb section area or we squeeze it
    delayMixKnob.setBounds(332, botY + 66, fxKS, fxKS);

    // Footer
    voiceModeBox.setBounds(15, 480, 90, 24);
    glideKnob.setBounds(130, 478, 50, 50);
    masterVolKnob.setBounds(650, 470, 65, 65);
}

void VoidSynthAudioProcessorEditor::timerCallback()
{
    updatePresetDisplay();

    // Update macro value labels
    auto formatVal = [](float v) { return juce::String(juce::roundToInt(v * 100)) + "%"; };
    macro1ValLabel.setText(formatVal(macro1Knob.getValue()), juce::dontSendNotification);
    macro2ValLabel.setText(formatVal(macro2Knob.getValue()), juce::dontSendNotification);
    macro3ValLabel.setText(formatVal(macro3Knob.getValue()), juce::dontSendNotification);
    macro4ValLabel.setText(formatVal(macro4Knob.getValue()), juce::dontSendNotification);
}

void VoidSynthAudioProcessorEditor::updateCategoryColour()
{
    auto& presets = processor.presetManager.getAllPresets();
    int idx = processor.presetManager.getCurrentPresetIndex();
    if (idx >= 0 && idx < (int)presets.size())
    {
        auto& cat = presets[idx].category;
        if (cat == "Rage") categoryColour = Colors::accentRed;
        else if (cat == "Granular") categoryColour = Colors::accentCyan;
        else if (cat == "Pads") categoryColour = Colors::accentPurple;
        else categoryColour = Colors::accentGold;
    }
    lookAndFeel.categoryAccent = categoryColour;
}

void VoidSynthAudioProcessorEditor::updatePresetDisplay()
{
    auto name = processor.presetManager.getCurrentPresetName();
    presetNameLabel.setText(name.isEmpty() ? "INIT" : name, juce::dontSendNotification);

    // Update category
    auto& presets = processor.presetManager.getAllPresets();
    int idx = processor.presetManager.getCurrentPresetIndex();
    if (idx >= 0 && idx < (int)presets.size())
    {
        categoryLabel.setText(presets[idx].category.toUpperCase(), juce::dontSendNotification);
    }
    else
    {
        categoryLabel.setText("", juce::dontSendNotification);
    }

    updateCategoryColour();
    categoryLabel.setColour(juce::Label::textColourId, categoryColour.withAlpha(0.7f));

    // Update macro labels
    auto& macroNames = processor.macroSystem.getMacroNames();
    macro1Label.setText(macroNames.size() > 0 ? macroNames[0] : "MACRO 1", juce::dontSendNotification);
    macro2Label.setText(macroNames.size() > 1 ? macroNames[1] : "MACRO 2", juce::dontSendNotification);
    macro3Label.setText(macroNames.size() > 2 ? macroNames[2] : "MACRO 3", juce::dontSendNotification);
    macro4Label.setText(macroNames.size() > 3 ? macroNames[3] : "MACRO 4", juce::dontSendNotification);

    repaint();
}

void VoidSynthAudioProcessorEditor::showPresetMenu()
{
    juce::PopupMenu menu;
    auto& presets = processor.presetManager.getAllPresets();

    juce::PopupMenu rageMenu, granMenu, padMenu;
    for (int i = 0; i < (int)presets.size(); ++i)
    {
        auto& p = presets[i];
        bool isCurrent = (i == processor.presetManager.getCurrentPresetIndex());
        if (p.category == "Rage")
            rageMenu.addItem(i + 1, p.name, true, isCurrent);
        else if (p.category == "Granular")
            granMenu.addItem(i + 1, p.name, true, isCurrent);
        else if (p.category == "Pads")
            padMenu.addItem(i + 1, p.name, true, isCurrent);
    }

    menu.addSubMenu("RAGE", rageMenu);
    menu.addSeparator();
    menu.addSubMenu("GRANULAR", granMenu);
    menu.addSeparator();
    menu.addSubMenu("PADS", padMenu);

    menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(&menuBtn),
        [this](int result)
        {
            if (result > 0)
            {
                processor.presetManager.loadPresetByIndex(result - 1);
                updatePresetDisplay();
            }
        });
}
