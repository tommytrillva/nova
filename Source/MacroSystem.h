#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <vector>
#include <array>

struct MacroMapping
{
    juce::String targetParamID;
    float amount = 0.0f;        // -1 to +1 bipolar
    float curveExponent = 1.0f; // 1.0 = linear
};

struct Macro
{
    juce::String displayName;
    std::vector<MacroMapping> mappings;
};

class MacroSystem
{
public:
    MacroSystem();

    void setMacroMappings(int macroIndex, const juce::String& name,
                          const std::vector<MacroMapping>& mappings);
    void clearAllMappings();

    void applyMacros(juce::AudioProcessorValueTreeState& apvts);
    void removeMacros(juce::AudioProcessorValueTreeState& apvts);

    const std::vector<juce::String>& getMacroNames() const { return macroNames; }

private:
    std::array<Macro, 4> macros;
    std::vector<juce::String> macroNames;

    // Store offsets applied so we can remove them
    struct AppliedOffset
    {
        juce::String paramID;
        float offset = 0.0f;
    };
    std::vector<AppliedOffset> appliedOffsets;
};
