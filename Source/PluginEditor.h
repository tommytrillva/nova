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

    // Preset navigation
    juce::TextButton prevPresetBtn{"<"};
    juce::TextButton nextPresetBtn{">"};
    juce::TextButton menuBtn{"MENU"};
    juce::Label presetNameLabel;

    // Macro knobs
    juce::Slider macro1Knob, macro2Knob, macro3Knob, macro4Knob;
    juce::Label macro1Label, macro2Label, macro3Label, macro4Label;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> macro1Attach, macro2Attach, macro3Attach, macro4Attach;

    // Osc A controls
    juce::ComboBox oscAWaveformBox;
    juce::Slider oscALevelKnob, oscAUnisonKnob;
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

    void setupKnob(juce::Slider& knob);
    void setupLabel(juce::Label& label, const juce::String& text);
    void showPresetMenu();
    void updatePresetDisplay();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VoidSynthAudioProcessorEditor)
};
