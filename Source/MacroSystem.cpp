#include "MacroSystem.h"

MacroSystem::MacroSystem()
{
    macroNames = { "MACRO 1", "MACRO 2", "MACRO 3", "MACRO 4" };
}

void MacroSystem::setMacroMappings(int macroIndex, const juce::String& name,
                                    const std::vector<MacroMapping>& mappings)
{
    if (macroIndex < 0 || macroIndex >= 4) return;
    macros[macroIndex].displayName = name;
    macros[macroIndex].mappings = mappings;

    if (macroIndex < (int)macroNames.size())
        macroNames[macroIndex] = name;
}

void MacroSystem::clearAllMappings()
{
    for (auto& m : macros)
    {
        m.displayName.clear();
        m.mappings.clear();
    }
    macroNames = { "MACRO 1", "MACRO 2", "MACRO 3", "MACRO 4" };
}

void MacroSystem::applyMacros(juce::AudioProcessorValueTreeState& apvts)
{
    appliedOffsets.clear();

    const juce::String macroParamIDs[] = { "macro1", "macro2", "macro3", "macro4" };

    for (int i = 0; i < 4; ++i)
    {
        float macroValue = apvts.getRawParameterValue(macroParamIDs[i])->load();
        if (macroValue == 0.0f) continue;

        for (auto& mapping : macros[i].mappings)
        {
            auto* param = apvts.getRawParameterValue(mapping.targetParamID);
            if (param == nullptr) continue;

            auto* paramObj = apvts.getParameter(mapping.targetParamID);
            if (paramObj == nullptr) continue;

            auto range = paramObj->getNormalisableRange();
            float paramRange = range.end - range.start;

            // Apply curve
            float curvedValue = macroValue;
            if (mapping.curveExponent != 1.0f)
                curvedValue = std::pow(macroValue, mapping.curveExponent);

            float offset = curvedValue * mapping.amount * paramRange;

            // Apply offset
            float currentVal = param->load();
            float newVal = juce::jlimit(range.start, range.end, currentVal + offset);
            float actualOffset = newVal - currentVal;

            param->store(newVal);
            appliedOffsets.push_back({ mapping.targetParamID, actualOffset });
        }
    }
}

void MacroSystem::removeMacros(juce::AudioProcessorValueTreeState& apvts)
{
    for (auto& applied : appliedOffsets)
    {
        auto* param = apvts.getRawParameterValue(applied.paramID);
        if (param != nullptr)
            param->store(param->load() - applied.offset);
    }
    appliedOffsets.clear();
}
