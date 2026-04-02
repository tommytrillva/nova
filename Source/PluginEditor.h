#pragma once

#include "PluginProcessor.h"
#include "LookAndFeel.h"

class VoidSynthAudioProcessorEditor : public juce::AudioProcessorEditor,
                                       private juce::Timer
{
public:
    explicit VoidSynthAudioProcessorEditor(VoidSynthAudioProcessor&);
    ~VoidSynthAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;
    void timerCallback() override;

private:
    VoidSynthAudioProcessor& processor;
    VoidSynthLookAndFeel lookAndFeel;

    // Current category color
    juce::Colour categoryColour { 0xffe94560 };

    // Preset navigation
    juce::TextButton prevPresetBtn{"<"};
    juce::TextButton nextPresetBtn{">"};
    juce::TextButton menuBtn{"BROWSE"};
    juce::Label presetNameLabel;
    juce::Label categoryLabel;

    // Macro knobs (hero elements)
    juce::Slider macro1Knob, macro2Knob, macro3Knob, macro4Knob;
    juce::Label macro1Label, macro2Label, macro3Label, macro4Label;
    juce::Label macro1ValLabel, macro2ValLabel, macro3ValLabel, macro4ValLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> macro1Attach, macro2Attach, macro3Attach, macro4Attach;

    // Osc A controls
    juce::ComboBox oscAWaveformBox;
    juce::Slider oscALevelKnob;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> oscALevelAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> oscAWaveformAttach;

    // Osc B controls
    juce::ComboBox oscBWaveformBox;
    juce::Slider oscBLevelKnob;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> oscBLevelAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> oscBWaveformAttach;

    // Granular controls
    juce::Slider granLevelKnob, granDensityKnob;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> granLevelAttach, granDensityAttach;

    // Sub controls
    juce::Slider subLevelKnob;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> subLevelAttach;

    // Filter controls
    juce::ComboBox filterTypeBox;
    juce::Slider filterCutoffKnob, filterResKnob, filterEnvKnob, filterDriveKnob;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> filterCutoffAttach, filterResAttach, filterEnvAttach, filterDriveAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> filterTypeAttach;

    // Distortion controls
    juce::ComboBox distModeBox;
    juce::Slider distDriveKnob, distMixKnob;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> distDriveAttach, distMixAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> distModeAttach;

    // Amp envelope
    juce::Slider ampAKnob, ampDKnob, ampSKnob, ampRKnob;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> ampAAttach, ampDAttach, ampSAttach, ampRAttach;

    // FX controls
    juce::Slider chorusRateKnob, chorusDepthKnob, chorusMixKnob;
    juce::Slider delayTimeKnob, delayFBKnob, delayMixKnob;
    juce::Slider reverbSizeKnob, reverbDampKnob, reverbMixKnob;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
        chorusRateAttach, chorusDepthAttach, chorusMixAttach,
        delayTimeAttach, delayFBAttach, delayMixAttach,
        reverbSizeAttach, reverbDampAttach, reverbMixAttach;

    // Footer controls
    juce::ComboBox voiceModeBox;
    juce::Slider glideKnob, masterVolKnob;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> glideAttach, masterVolAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> voiceModeAttach;

    void setupKnob(juce::Slider& knob, juce::Colour accentColour = juce::Colour(0xfff0f0f0));
    void setupSmallKnob(juce::Slider& knob, juce::Colour accentColour);
    void setupLabel(juce::Label& label, const juce::String& text, float fontSize = 10.0f);
    void showPresetMenu();
    void updatePresetDisplay();
    void updateCategoryColour();

    // Paint helpers
    void drawSection(juce::Graphics& g, juce::Rectangle<float> bounds, const juce::String& title,
                     juce::Colour accent = juce::Colour(0xff556677));
    void drawHeaderBar(juce::Graphics& g);
    void drawMacroBackground(juce::Graphics& g);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VoidSynthAudioProcessorEditor)
};
