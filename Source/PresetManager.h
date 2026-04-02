#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "MacroSystem.h"
#include <vector>

struct PresetInfo
{
    juce::String name;
    juce::String category;
    juce::StringArray tags;
    juce::String jsonData; // raw JSON for loading
};

class PresetManager
{
public:
    PresetManager(juce::AudioProcessorValueTreeState& apvts, MacroSystem& macroSystem);

    void loadFactoryPresets();
    void loadPresetByIndex(int index);
    void loadPresetByName(const juce::String& name);
    void loadNextPreset();
    void loadPrevPreset();

    juce::String getCurrentPresetName() const;
    int getCurrentPresetIndex() const { return currentPresetIndex; }
    const std::vector<PresetInfo>& getAllPresets() const { return presets; }

private:
    juce::AudioProcessorValueTreeState& apvts;
    MacroSystem& macroSystem;

    std::vector<PresetInfo> presets;
    int currentPresetIndex = -1;

    void applyPresetJSON(const juce::String& json);
    void parseAndApplyParam(const juce::var& obj, const juce::String& jsonKey, const juce::String& paramID);
};
