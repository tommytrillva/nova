#include "PresetManager.h"
#include "BinaryData.h"

PresetManager::PresetManager(juce::AudioProcessorValueTreeState& a, MacroSystem& ms)
    : apvts(a), macroSystem(ms)
{
    loadFactoryPresets();
}

void PresetManager::loadFactoryPresets()
{
    presets.clear();

    for (int i = 0; i < BinaryData::namedResourceListSize; ++i)
    {
        juce::String name = BinaryData::namedResourceList[i];
        if (!name.endsWith("_json")) continue;

        int dataSize = 0;
        const char* data = BinaryData::getNamedResource(name.toRawUTF8(), dataSize);
        if (data == nullptr || dataSize == 0) continue;

        juce::String jsonStr(data, (size_t)dataSize);
        auto parsed = juce::JSON::parse(jsonStr);
        if (!parsed.isObject()) continue;

        PresetInfo info;
        info.name = parsed.getProperty("name", "Unknown").toString();
        info.category = parsed.getProperty("category", "Other").toString();
        info.jsonData = jsonStr;

        auto tagsVar = parsed.getProperty("tags", juce::var());
        if (tagsVar.isArray())
        {
            for (int t = 0; t < tagsVar.size(); ++t)
                info.tags.add(tagsVar[t].toString());
        }

        presets.push_back(info);
    }

    // Sort: Rage, Granular, Pads
    std::sort(presets.begin(), presets.end(), [](const PresetInfo& a, const PresetInfo& b)
    {
        auto catOrder = [](const juce::String& c) -> int
        {
            if (c == "Rage") return 0;
            if (c == "Granular") return 1;
            if (c == "Pads") return 2;
            return 3;
        };
        int ca = catOrder(a.category);
        int cb = catOrder(b.category);
        if (ca != cb) return ca < cb;
        return a.name < b.name;
    });
}

void PresetManager::loadPresetByIndex(int index)
{
    if (index < 0 || index >= (int)presets.size()) return;
    currentPresetIndex = index;
    applyPresetJSON(presets[index].jsonData);
}

void PresetManager::loadPresetByName(const juce::String& name)
{
    for (int i = 0; i < (int)presets.size(); ++i)
    {
        if (presets[i].name == name)
        {
            loadPresetByIndex(i);
            return;
        }
    }
}

void PresetManager::loadNextPreset()
{
    if (presets.empty()) return;
    int next = (currentPresetIndex + 1) % (int)presets.size();
    loadPresetByIndex(next);
}

void PresetManager::loadPrevPreset()
{
    if (presets.empty()) return;
    int prev = currentPresetIndex - 1;
    if (prev < 0) prev = (int)presets.size() - 1;
    loadPresetByIndex(prev);
}

juce::String PresetManager::getCurrentPresetName() const
{
    if (currentPresetIndex >= 0 && currentPresetIndex < (int)presets.size())
        return presets[currentPresetIndex].name;
    return {};
}

void PresetManager::parseAndApplyParam(const juce::var& obj, const juce::String& jsonKey, const juce::String& paramID)
{
    if (obj.hasProperty(jsonKey))
    {
        auto* param = apvts.getRawParameterValue(paramID);
        if (param != nullptr)
            param->store((float)obj.getProperty(jsonKey, 0.0));
    }
}

void PresetManager::applyPresetJSON(const juce::String& json)
{
    auto preset = juce::JSON::parse(json);
    if (!preset.isObject()) return;

    macroSystem.clearAllMappings();

    // Voice settings
    auto voiceModeStr = preset.getProperty("voiceMode", "poly").toString();
    int voiceMode = 0;
    if (voiceModeStr == "mono") voiceMode = 1;
    else if (voiceModeStr == "legato") voiceMode = 2;
    if (auto* p = apvts.getRawParameterValue("voiceMode")) p->store((float)voiceMode);

    parseAndApplyParam(preset, "glideTime", "glideTime");

    // Oscillator A
    auto oscA = preset.getProperty("oscA", juce::var());
    if (oscA.isObject())
    {
        auto wfStr = oscA.getProperty("waveform", "saw").toString();
        int wf = 0;
        if (wfStr == "square") wf = 1;
        else if (wfStr == "triangle") wf = 2;
        else if (wfStr == "sine") wf = 3;
        else if (wfStr == "wavetable") wf = 4;
        if (auto* p = apvts.getRawParameterValue("oscAWaveform")) p->store((float)wf);

        parseAndApplyParam(oscA, "wavetableIndex", "oscAWavetableIndex");
        parseAndApplyParam(oscA, "wavetablePosition", "oscAWavetablePos");
        parseAndApplyParam(oscA, "level", "oscALevel");
        parseAndApplyParam(oscA, "coarseTune", "oscACoarse");
        parseAndApplyParam(oscA, "fineTune", "oscAFine");
        parseAndApplyParam(oscA, "unison", "oscAUnison");
        parseAndApplyParam(oscA, "detuneAmount", "oscADetune");
        parseAndApplyParam(oscA, "unisonSpread", "oscASpread");
        parseAndApplyParam(oscA, "pulseWidth", "oscAPulseWidth");
    }

    // Oscillator B
    auto oscB = preset.getProperty("oscB", juce::var());
    if (oscB.isObject())
    {
        auto wfStr = oscB.getProperty("waveform", "saw").toString();
        int wf = 0;
        if (wfStr == "square") wf = 1;
        else if (wfStr == "triangle") wf = 2;
        else if (wfStr == "sine") wf = 3;
        else if (wfStr == "wavetable") wf = 4;
        if (auto* p = apvts.getRawParameterValue("oscBWaveform")) p->store((float)wf);

        parseAndApplyParam(oscB, "wavetableIndex", "oscBWavetableIndex");
        parseAndApplyParam(oscB, "wavetablePosition", "oscBWavetablePos");
        parseAndApplyParam(oscB, "level", "oscBLevel");
        parseAndApplyParam(oscB, "coarseTune", "oscBCoarse");
        parseAndApplyParam(oscB, "fineTune", "oscBFine");
        parseAndApplyParam(oscB, "unison", "oscBUnison");
        parseAndApplyParam(oscB, "detuneAmount", "oscBDetune");
        parseAndApplyParam(oscB, "unisonSpread", "oscBSpread");
        parseAndApplyParam(oscB, "pulseWidth", "oscBPulseWidth");
    }

    // Granular
    auto gran = preset.getProperty("granular", juce::var());
    if (gran.isObject())
    {
        bool enabled = (bool)gran.getProperty("enabled", false);
        if (auto* p = apvts.getRawParameterValue("granEnabled")) p->store(enabled ? 1.0f : 0.0f);
        parseAndApplyParam(gran, "sourceIndex", "granSource");
        parseAndApplyParam(gran, "position", "granPosition");
        parseAndApplyParam(gran, "positionRand", "granPosRand");
        parseAndApplyParam(gran, "size", "granSize");
        parseAndApplyParam(gran, "density", "granDensity");
        parseAndApplyParam(gran, "pitch", "granPitch");
        parseAndApplyParam(gran, "pitchRand", "granPitchRand");

        auto shapeStr = gran.getProperty("shape", "hann").toString();
        int shape = 0;
        if (shapeStr == "triangle") shape = 1;
        else if (shapeStr == "rect") shape = 2;
        if (auto* p = apvts.getRawParameterValue("granShape")) p->store((float)shape);

        parseAndApplyParam(gran, "panSpread", "granPanSpread");
        parseAndApplyParam(gran, "level", "granLevel");
    }

    // Sub
    auto sub = preset.getProperty("sub", juce::var());
    if (sub.isObject())
    {
        auto shapeStr = sub.getProperty("shape", "sine").toString();
        if (auto* p = apvts.getRawParameterValue("subShape")) p->store(shapeStr == "triangle" ? 1.0f : 0.0f);

        int oct = (int)sub.getProperty("octave", -1);
        if (auto* p = apvts.getRawParameterValue("subOctave")) p->store(oct == -2 ? 1.0f : 0.0f);

        parseAndApplyParam(sub, "level", "subLevel");
    }

    // Distortion
    auto dist = preset.getProperty("distortion", juce::var());
    if (dist.isObject())
    {
        auto modeStr = dist.getProperty("mode", "softclip").toString().toLowerCase();
        int mode = 0;
        if (modeStr == "hardclip" || modeStr == "hard clip") mode = 1;
        else if (modeStr == "wavefold") mode = 2;
        else if (modeStr == "bitcrush") mode = 3;
        else if (modeStr == "rectify") mode = 4;
        if (auto* p = apvts.getRawParameterValue("distMode")) p->store((float)mode);

        parseAndApplyParam(dist, "drive", "distDrive");
        parseAndApplyParam(dist, "mix", "distMix");
    }

    // Filter
    auto filter = preset.getProperty("filter", juce::var());
    if (filter.isObject())
    {
        auto typeStr = filter.getProperty("type", "lp12").toString().toLowerCase();
        int type = 0;
        if (typeStr == "lp24") type = 1;
        else if (typeStr == "hp12") type = 2;
        else if (typeStr == "bp") type = 3;
        else if (typeStr == "notch") type = 4;
        if (auto* p = apvts.getRawParameterValue("filterType")) p->store((float)type);

        parseAndApplyParam(filter, "cutoff", "filterCutoff");
        parseAndApplyParam(filter, "resonance", "filterRes");
        parseAndApplyParam(filter, "envAmount", "filterEnvAmt");
        parseAndApplyParam(filter, "keyTrack", "filterKeyTrack");
        parseAndApplyParam(filter, "drive", "filterDrive");
    }

    // Amp Envelope
    auto ampEnv = preset.getProperty("ampEnv", juce::var());
    if (ampEnv.isObject())
    {
        parseAndApplyParam(ampEnv, "attack", "ampA");
        parseAndApplyParam(ampEnv, "decay", "ampD");
        parseAndApplyParam(ampEnv, "sustain", "ampS");
        parseAndApplyParam(ampEnv, "release", "ampR");
    }

    // Filter Envelope
    auto filtEnv = preset.getProperty("filterEnv", juce::var());
    if (filtEnv.isObject())
    {
        parseAndApplyParam(filtEnv, "attack", "filtEnvA");
        parseAndApplyParam(filtEnv, "decay", "filtEnvD");
        parseAndApplyParam(filtEnv, "sustain", "filtEnvS");
        parseAndApplyParam(filtEnv, "release", "filtEnvR");
    }

    // LFO 1
    auto lfo1 = preset.getProperty("lfo1", juce::var());
    if (lfo1.isObject())
    {
        parseAndApplyParam(lfo1, "rate", "lfo1Rate");
        auto shapeStr = lfo1.getProperty("shape", "sine").toString().toLowerCase();
        int shape = 0;
        if (shapeStr == "saw") shape = 1;
        else if (shapeStr == "square") shape = 2;
        else if (shapeStr == "triangle") shape = 3;
        else if (shapeStr == "s&h" || shapeStr == "samplehold" || shapeStr == "sh") shape = 4;
        if (auto* p = apvts.getRawParameterValue("lfo1Shape")) p->store((float)shape);
        if (auto* p = apvts.getRawParameterValue("lfo1Sync"))
            p->store(lfo1.getProperty("tempoSync", false) ? 1.0f : 0.0f);
        parseAndApplyParam(lfo1, "amount", "lfo1Amount");
    }

    // LFO 2
    auto lfo2 = preset.getProperty("lfo2", juce::var());
    if (lfo2.isObject())
    {
        parseAndApplyParam(lfo2, "rate", "lfo2Rate");
        auto shapeStr = lfo2.getProperty("shape", "sine").toString().toLowerCase();
        int shape = 0;
        if (shapeStr == "saw") shape = 1;
        else if (shapeStr == "square") shape = 2;
        else if (shapeStr == "triangle") shape = 3;
        else if (shapeStr == "s&h" || shapeStr == "samplehold" || shapeStr == "sh") shape = 4;
        if (auto* p = apvts.getRawParameterValue("lfo2Shape")) p->store((float)shape);
        if (auto* p = apvts.getRawParameterValue("lfo2Sync"))
            p->store(lfo2.getProperty("tempoSync", false) ? 1.0f : 0.0f);
        parseAndApplyParam(lfo2, "amount", "lfo2Amount");
    }

    // FX
    auto fx = preset.getProperty("fx", juce::var());
    if (fx.isObject())
    {
        auto chorus = fx.getProperty("chorus", juce::var());
        if (chorus.isObject())
        {
            parseAndApplyParam(chorus, "rate", "chorusRate");
            parseAndApplyParam(chorus, "depth", "chorusDepth");
            parseAndApplyParam(chorus, "mix", "chorusMix");
        }

        auto delay = fx.getProperty("delay", juce::var());
        if (delay.isObject())
        {
            parseAndApplyParam(delay, "time", "delayTime");
            parseAndApplyParam(delay, "feedback", "delayFeedback");
            parseAndApplyParam(delay, "mix", "delayMix");
            if (auto* p = apvts.getRawParameterValue("delaySync"))
                p->store(delay.getProperty("sync", true) ? 1.0f : 0.0f);
            parseAndApplyParam(delay, "filter", "delayFilter");
        }

        auto reverb = fx.getProperty("reverb", juce::var());
        if (reverb.isObject())
        {
            parseAndApplyParam(reverb, "size", "reverbSize");
            parseAndApplyParam(reverb, "damping", "reverbDamp");
            parseAndApplyParam(reverb, "mix", "reverbMix");
        }
    }

    // Macros
    auto macrosArr = preset.getProperty("macros", juce::var());
    if (macrosArr.isArray())
    {
        for (int i = 0; i < macrosArr.size() && i < 4; ++i)
        {
            auto macroObj = macrosArr[i];
            if (!macroObj.isObject()) continue;

            juce::String name = macroObj.getProperty("name", "MACRO").toString();
            std::vector<MacroMapping> mappings;

            auto mappingsArr = macroObj.getProperty("mappings", juce::var());
            if (mappingsArr.isArray())
            {
                for (int j = 0; j < mappingsArr.size(); ++j)
                {
                    auto m = mappingsArr[j];
                    if (!m.isObject()) continue;
                    MacroMapping mapping;
                    mapping.targetParamID = m.getProperty("target", "").toString();
                    mapping.amount = (float)m.getProperty("amount", 0.0);
                    mapping.curveExponent = (float)m.getProperty("curve", 1.0);
                    mappings.push_back(mapping);
                }
            }

            macroSystem.setMacroMappings(i, name, mappings);
        }
    }

    // Reset macro positions to 0
    for (int i = 1; i <= 4; ++i)
    {
        juce::String id = "macro" + juce::String(i);
        if (auto* p = apvts.getRawParameterValue(id))
            p->store(0.0f);
    }
}
