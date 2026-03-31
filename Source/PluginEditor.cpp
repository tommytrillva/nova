#include "PluginEditor.h"

// Color palette
namespace Colors
{
    const juce::Colour background   { 0xff0d0d14 };
    const juce::Colour surface      { 0xff161622 };
    const juce::Colour surfaceLight { 0xff1e1e30 };
    const juce::Colour accentRed    { 0xffe94560 };
    const juce::Colour accentCyan   { 0xff00d4ff };
    const juce::Colour accentPurple { 0xff8b5cf6 };
    const juce::Colour accentGold   { 0xfff5c518 };
    const juce::Colour textPrimary  { 0xffeaeaea };
    const juce::Colour textSecondary{ 0xff667788 };
    const juce::Colour meterGreen   { 0xff00e676 };
}

VoidSynthAudioProcessorEditor::VoidSynthAudioProcessorEditor(VoidSynthAudioProcessor& p)
    : AudioProcessorEditor(&p), processor(p)
{
    setSize(660, 520);
    setLookAndFeel(&lookAndFeel);

    // Preset navigation
    addAndMakeVisible(prevPresetBtn);
    addAndMakeVisible(nextPresetBtn);
    addAndMakeVisible(menuBtn);
    addAndMakeVisible(presetNameLabel);
    presetNameLabel.setJustificationType(juce::Justification::centred);
    presetNameLabel.setColour(juce::Label::textColourId, Colors::textPrimary);
    presetNameLabel.setFont(juce::Font(16.0f, juce::Font::bold));

    prevPresetBtn.onClick = [this] { processor.presetManager.loadPrevPreset(); updatePresetDisplay(); };
    nextPresetBtn.onClick = [this] { processor.presetManager.loadNextPreset(); updatePresetDisplay(); };
    menuBtn.onClick = [this] { showPresetMenu(); };

    // Macro knobs
    auto setupMacroKnob = [this](juce::Slider& knob, juce::Label& label, const juce::String& paramId,
                                  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>& attach,
                                  const juce::String& labelText)
    {
        setupKnob(knob);
        knob.setColour(juce::Slider::rotarySliderFillColourId, Colors::accentGold);
        addAndMakeVisible(knob);
        attach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor.apvts, paramId, knob);
        setupLabel(label, labelText);
        addAndMakeVisible(label);
    };

    setupMacroKnob(macro1Knob, macro1Label, "macro1", macro1Attach, "MACRO 1");
    setupMacroKnob(macro2Knob, macro2Label, "macro2", macro2Attach, "MACRO 2");
    setupMacroKnob(macro3Knob, macro3Label, "macro3", macro3Attach, "MACRO 3");
    setupMacroKnob(macro4Knob, macro4Label, "macro4", macro4Attach, "MACRO 4");

    // Osc A
    oscAWaveformBox.addItemList({"Saw", "Square", "Triangle", "Sine", "Wavetable"}, 1);
    addAndMakeVisible(oscAWaveformBox);
    oscAWaveformAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(processor.apvts, "oscAWaveform", oscAWaveformBox);
    setupKnob(oscALevelKnob);
    addAndMakeVisible(oscALevelKnob);
    oscALevelAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor.apvts, "oscALevel", oscALevelKnob);

    // Osc B
    oscBWaveformBox.addItemList({"Saw", "Square", "Triangle", "Sine", "Wavetable"}, 1);
    addAndMakeVisible(oscBWaveformBox);
    oscBWaveformAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(processor.apvts, "oscBWaveform", oscBWaveformBox);
    setupKnob(oscBLevelKnob);
    addAndMakeVisible(oscBLevelKnob);
    oscBLevelAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor.apvts, "oscBLevel", oscBLevelKnob);

    // Granular
    setupKnob(granLevelKnob);
    granLevelKnob.setColour(juce::Slider::rotarySliderFillColourId, Colors::accentCyan);
    addAndMakeVisible(granLevelKnob);
    granLevelAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor.apvts, "granLevel", granLevelKnob);
    setupKnob(granDensityKnob);
    granDensityKnob.setColour(juce::Slider::rotarySliderFillColourId, Colors::accentCyan);
    addAndMakeVisible(granDensityKnob);
    granDensityAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor.apvts, "granDensity", granDensityKnob);

    // Sub
    setupKnob(subLevelKnob);
    addAndMakeVisible(subLevelKnob);
    subLevelAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor.apvts, "subLevel", subLevelKnob);

    // Filter
    filterTypeBox.addItemList({"LP 12", "LP 24", "HP 12", "BP", "Notch"}, 1);
    addAndMakeVisible(filterTypeBox);
    filterTypeAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(processor.apvts, "filterType", filterTypeBox);

    auto setupFilterKnob = [this](juce::Slider& knob, const juce::String& paramId,
                                   std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>& attach)
    {
        setupKnob(knob);
        knob.setColour(juce::Slider::rotarySliderFillColourId, Colors::accentPurple);
        addAndMakeVisible(knob);
        attach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor.apvts, paramId, knob);
    };
    setupFilterKnob(filterCutoffKnob, "filterCutoff", filterCutoffAttach);
    setupFilterKnob(filterResKnob, "filterRes", filterResAttach);
    setupFilterKnob(filterEnvKnob, "filterEnvAmt", filterEnvAttach);
    setupFilterKnob(filterDriveKnob, "filterDrive", filterDriveAttach);

    // Distortion
    distModeBox.addItemList({"Soft Clip", "Hard Clip", "Wavefold", "Bitcrush", "Rectify"}, 1);
    addAndMakeVisible(distModeBox);
    distModeAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(processor.apvts, "distMode", distModeBox);
    setupKnob(distDriveKnob);
    distDriveKnob.setColour(juce::Slider::rotarySliderFillColourId, Colors::accentRed);
    addAndMakeVisible(distDriveKnob);
    distDriveAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor.apvts, "distDrive", distDriveKnob);
    setupKnob(distMixKnob);
    distMixKnob.setColour(juce::Slider::rotarySliderFillColourId, Colors::accentRed);
    addAndMakeVisible(distMixKnob);
    distMixAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor.apvts, "distMix", distMixKnob);

    // Amp Envelope
    auto setupEnvKnob = [this](juce::Slider& knob, const juce::String& paramId,
                                std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>& attach)
    {
        setupKnob(knob);
        addAndMakeVisible(knob);
        attach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor.apvts, paramId, knob);
    };
    setupEnvKnob(ampAKnob, "ampA", ampAAttach);
    setupEnvKnob(ampDKnob, "ampD", ampDAttach);
    setupEnvKnob(ampSKnob, "ampS", ampSAttach);
    setupEnvKnob(ampRKnob, "ampR", ampRAttach);

    // FX Knobs
    auto setupFxKnob = [this](juce::Slider& knob, const juce::String& paramId,
                               std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>& attach)
    {
        setupKnob(knob);
        addAndMakeVisible(knob);
        attach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor.apvts, paramId, knob);
    };
    setupFxKnob(chorusRateKnob, "chorusRate", chorusRateAttach);
    setupFxKnob(chorusDepthKnob, "chorusDepth", chorusDepthAttach);
    setupFxKnob(chorusMixKnob, "chorusMix", chorusMixAttach);
    setupFxKnob(delayTimeKnob, "delayTime", delayTimeAttach);
    setupFxKnob(delayFBKnob, "delayFeedback", delayFBAttach);
    setupFxKnob(delayMixKnob, "delayMix", delayMixAttach);
    setupFxKnob(reverbSizeKnob, "reverbSize", reverbSizeAttach);
    setupFxKnob(reverbDampKnob, "reverbDamp", reverbDampAttach);
    setupFxKnob(reverbMixKnob, "reverbMix", reverbMixAttach);

    // Footer
    voiceModeBox.addItemList({"Poly", "Mono", "Legato"}, 1);
    addAndMakeVisible(voiceModeBox);
    voiceModeAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(processor.apvts, "voiceMode", voiceModeBox);
    setupKnob(glideKnob);
    addAndMakeVisible(glideKnob);
    glideAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor.apvts, "glideTime", glideKnob);
    setupKnob(masterVolKnob);
    addAndMakeVisible(masterVolKnob);
    masterVolAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor.apvts, "masterVol", masterVolKnob);

    updatePresetDisplay();
    startTimerHz(30);
}

VoidSynthAudioProcessorEditor::~VoidSynthAudioProcessorEditor()
{
    setLookAndFeel(nullptr);
}

void VoidSynthAudioProcessorEditor::setupKnob(juce::Slider& knob)
{
    knob.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    knob.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 14);
}

void VoidSynthAudioProcessorEditor::setupLabel(juce::Label& label, const juce::String& text)
{
    label.setText(text, juce::dontSendNotification);
    label.setJustificationType(juce::Justification::centred);
    label.setColour(juce::Label::textColourId, Colors::textSecondary);
    label.setFont(juce::Font(11.0f, juce::Font::bold));
}

void VoidSynthAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(Colors::background);

    // Header bar
    g.setColour(Colors::surface);
    g.fillRect(0, 0, getWidth(), 40);

    // Title
    g.setColour(Colors::textPrimary);
    g.setFont(juce::Font(18.0f, juce::Font::bold));
    g.drawText("VOIDSYNTH", 10, 0, 120, 40, juce::Justification::centredLeft);

    // Section backgrounds
    g.setColour(Colors::surface);
    g.fillRoundedRectangle(10.0f, 45.0f, 640.0f, 145.0f, 6.0f); // Macro area
    g.fillRoundedRectangle(10.0f, 200.0f, 400.0f, 120.0f, 6.0f); // Engine strip left
    g.fillRoundedRectangle(420.0f, 200.0f, 230.0f, 120.0f, 6.0f); // Filter area
    g.fillRoundedRectangle(10.0f, 330.0f, 230.0f, 130.0f, 6.0f); // Dist + Env
    g.fillRoundedRectangle(250.0f, 330.0f, 400.0f, 130.0f, 6.0f); // FX area

    // Section labels
    g.setColour(Colors::textSecondary);
    g.setFont(juce::Font(10.0f, juce::Font::bold));
    g.drawText("OSC A", 20, 200, 80, 16, juce::Justification::centredLeft);
    g.drawText("OSC B", 120, 200, 80, 16, juce::Justification::centredLeft);
    g.drawText("GRAIN", 220, 200, 80, 16, juce::Justification::centredLeft);
    g.drawText("SUB", 320, 200, 80, 16, juce::Justification::centredLeft);
    g.drawText("FILTER", 430, 200, 80, 16, juce::Justification::centredLeft);
    g.drawText("DISTORTION", 20, 330, 100, 16, juce::Justification::centredLeft);
    g.drawText("AMP ENV", 130, 330, 100, 16, juce::Justification::centredLeft);
    g.drawText("CHR", 260, 330, 40, 16, juce::Justification::centredLeft);
    g.drawText("DLY", 260, 370, 40, 16, juce::Justification::centredLeft);
    g.drawText("REV", 260, 410, 40, 16, juce::Justification::centredLeft);
}

void VoidSynthAudioProcessorEditor::resized()
{
    // Header
    prevPresetBtn.setBounds(200, 8, 30, 24);
    presetNameLabel.setBounds(235, 8, 200, 24);
    nextPresetBtn.setBounds(440, 8, 30, 24);
    menuBtn.setBounds(590, 8, 55, 24);

    // Macro row — 4 large knobs
    int macroY = 60;
    int macroKnobSize = 65;
    int macroSpacing = 150;
    int macroStartX = 40;

    auto placeMacro = [&](juce::Slider& knob, juce::Label& label, int index)
    {
        int x = macroStartX + index * macroSpacing;
        label.setBounds(x, macroY - 10, macroKnobSize, 16);
        knob.setBounds(x, macroY + 8, macroKnobSize, macroKnobSize);
    };
    placeMacro(macro1Knob, macro1Label, 0);
    placeMacro(macro2Knob, macro2Label, 1);
    placeMacro(macro3Knob, macro3Label, 2);
    placeMacro(macro4Knob, macro4Label, 3);

    // Engine strip
    int engY = 218;
    int knobS = 36;

    // Osc A
    oscAWaveformBox.setBounds(20, engY, 75, 20);
    oscALevelKnob.setBounds(20, engY + 24, knobS, knobS + 14);

    // Osc B
    oscBWaveformBox.setBounds(120, engY, 75, 20);
    oscBLevelKnob.setBounds(120, engY + 24, knobS, knobS + 14);

    // Granular
    granLevelKnob.setBounds(220, engY + 24, knobS, knobS + 14);
    granDensityKnob.setBounds(265, engY + 24, knobS, knobS + 14);

    // Sub
    subLevelKnob.setBounds(340, engY + 24, knobS, knobS + 14);

    // Filter
    filterTypeBox.setBounds(430, engY, 90, 20);
    filterCutoffKnob.setBounds(430, engY + 24, knobS, knobS + 14);
    filterResKnob.setBounds(475, engY + 24, knobS, knobS + 14);
    filterEnvKnob.setBounds(520, engY + 24, knobS, knobS + 14);
    filterDriveKnob.setBounds(565, engY + 24, knobS, knobS + 14);

    // Bottom strip - distortion + envelope
    int botY = 348;
    distModeBox.setBounds(20, botY, 90, 20);
    distDriveKnob.setBounds(20, botY + 24, knobS, knobS + 14);
    distMixKnob.setBounds(65, botY + 24, knobS, knobS + 14);

    ampAKnob.setBounds(130, botY + 24, knobS, knobS + 14);
    ampDKnob.setBounds(168, botY + 24, knobS, knobS + 14);
    ampSKnob.setBounds(130, botY + 68, knobS, knobS + 14);
    ampRKnob.setBounds(168, botY + 68, knobS, knobS + 14);

    // FX area
    int fxX = 300;
    int fxKS = knobS;
    chorusRateKnob.setBounds(fxX, botY, fxKS, fxKS + 14);
    chorusDepthKnob.setBounds(fxX + 45, botY, fxKS, fxKS + 14);
    chorusMixKnob.setBounds(fxX + 90, botY, fxKS, fxKS + 14);

    delayTimeKnob.setBounds(fxX, botY + 42, fxKS, fxKS + 14);
    delayFBKnob.setBounds(fxX + 45, botY + 42, fxKS, fxKS + 14);
    delayMixKnob.setBounds(fxX + 90, botY + 42, fxKS, fxKS + 14);

    reverbSizeKnob.setBounds(fxX, botY + 84, fxKS, fxKS + 14);
    reverbDampKnob.setBounds(fxX + 45, botY + 84, fxKS, fxKS + 14);
    reverbMixKnob.setBounds(fxX + 90, botY + 84, fxKS, fxKS + 14);

    // Footer
    voiceModeBox.setBounds(15, 475, 80, 24);
    glideKnob.setBounds(110, 470, 50, 40);
    masterVolKnob.setBounds(550, 470, 50, 40);
}

void VoidSynthAudioProcessorEditor::timerCallback()
{
    updatePresetDisplay();
}

void VoidSynthAudioProcessorEditor::updatePresetDisplay()
{
    auto name = processor.presetManager.getCurrentPresetName();
    presetNameLabel.setText(name.isEmpty() ? "INIT" : name, juce::dontSendNotification);

    // Update macro labels from current preset
    auto& macroNames = processor.macroSystem.getMacroNames();
    macro1Label.setText(macroNames.size() > 0 ? macroNames[0] : "MACRO 1", juce::dontSendNotification);
    macro2Label.setText(macroNames.size() > 1 ? macroNames[1] : "MACRO 2", juce::dontSendNotification);
    macro3Label.setText(macroNames.size() > 2 ? macroNames[2] : "MACRO 3", juce::dontSendNotification);
    macro4Label.setText(macroNames.size() > 3 ? macroNames[3] : "MACRO 4", juce::dontSendNotification);
}

void VoidSynthAudioProcessorEditor::showPresetMenu()
{
    juce::PopupMenu menu;
    auto& presets = processor.presetManager.getAllPresets();

    juce::PopupMenu rageMenu, granMenu, padMenu;
    for (int i = 0; i < (int)presets.size(); ++i)
    {
        auto& p = presets[i];
        if (p.category == "Rage")
            rageMenu.addItem(i + 1, p.name);
        else if (p.category == "Granular")
            granMenu.addItem(i + 1, p.name);
        else if (p.category == "Pads")
            padMenu.addItem(i + 1, p.name);
    }

    menu.addSubMenu("Rage", rageMenu);
    menu.addSubMenu("Granular", granMenu);
    menu.addSubMenu("Pads", padMenu);

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
