#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "SynthVoice.h"
#include "SynthSound.h"

VoidSynthAudioProcessor::VoidSynthAudioProcessor()
    : AudioProcessor(BusesProperties()
                     .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "PARAMETERS", createParameterLayout()),
      presetManager(apvts, macroSystem)
{
    synth.addSound(new SynthSound());
    for (int i = 0; i < 16; ++i)
        synth.addVoice(new SynthVoice(apvts));
}

VoidSynthAudioProcessor::~VoidSynthAudioProcessor() {}

void VoidSynthAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    synth.setCurrentPlaybackSampleRate(sampleRate);
    for (int i = 0; i < synth.getNumVoices(); ++i)
    {
        if (auto* voice = dynamic_cast<SynthVoice*>(synth.getVoice(i)))
            voice->prepareToPlay(sampleRate, samplesPerBlock);
    }
    fxChain.prepareToPlay(sampleRate, samplesPerBlock);
}

void VoidSynthAudioProcessor::releaseResources() {}

bool VoidSynthAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

void VoidSynthAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    buffer.clear();

    // Apply macro offsets before synthesis
    macroSystem.applyMacros(apvts);

    // Get tempo from host for sync
    double bpm = 120.0;
    if (auto* playHead = getPlayHead())
    {
        if (auto pos = playHead->getPosition())
        {
            if (pos->getBpm().hasValue())
                bpm = *pos->getBpm();
        }
    }

    // Update voice parameters
    for (int i = 0; i < synth.getNumVoices(); ++i)
    {
        if (auto* voice = dynamic_cast<SynthVoice*>(synth.getVoice(i)))
            voice->updateParams(bpm);
    }

    synth.renderNextBlock(buffer, midiMessages, 0, buffer.getNumSamples());

    // Apply global FX chain
    fxChain.updateFromParams(apvts);
    fxChain.process(buffer, bpm);

    // Master volume
    float masterVol = apvts.getRawParameterValue("masterVol")->load();
    buffer.applyGain(masterVol);

    // Remove macro offsets so base values are preserved
    macroSystem.removeMacros(apvts);
}

juce::AudioProcessorEditor* VoidSynthAudioProcessor::createEditor()
{
    return new VoidSynthAudioProcessorEditor(*this);
}

void VoidSynthAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    // Store current preset name
    state.setProperty("currentPreset", presetManager.getCurrentPresetName(), nullptr);
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void VoidSynthAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data, sizeInBytes));
    if (xml != nullptr && xml->hasTagName(apvts.state.getType()))
    {
        apvts.replaceState(juce::ValueTree::fromXml(*xml));
        auto presetName = apvts.state.getProperty("currentPreset", "").toString();
        if (presetName.isNotEmpty())
            presetManager.loadPresetByName(presetName);
    }
}

juce::AudioProcessorValueTreeState::ParameterLayout VoidSynthAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    auto addFloat = [&](const juce::String& id, const juce::String& name,
                        float min, float max, float def, float skew = 1.0f)
    {
        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{id, 1}, name,
            juce::NormalisableRange<float>(min, max, 0.0f, skew), def));
    };

    auto addInt = [&](const juce::String& id, const juce::String& name,
                      int min, int max, int def)
    {
        params.push_back(std::make_unique<juce::AudioParameterInt>(
            juce::ParameterID{id, 1}, name, min, max, def));
    };

    auto addBool = [&](const juce::String& id, const juce::String& name, bool def)
    {
        params.push_back(std::make_unique<juce::AudioParameterBool>(
            juce::ParameterID{id, 1}, name, def));
    };

    // ---- Oscillator A ----
    addInt("oscAWaveform", "Osc A Waveform", 0, 4, 0);
    addInt("oscAWavetableIndex", "Osc A Wavetable", 0, 23, 0);
    addFloat("oscAWavetablePos", "Osc A WT Pos", 0.0f, 1.0f, 0.0f);
    addFloat("oscALevel", "Osc A Level", 0.0f, 1.0f, 0.8f);
    addInt("oscACoarse", "Osc A Coarse", -24, 24, 0);
    addFloat("oscAFine", "Osc A Fine", -100.0f, 100.0f, 0.0f);
    addInt("oscAUnison", "Osc A Unison", 1, 8, 1);
    addFloat("oscADetune", "Osc A Detune", 0.0f, 100.0f, 0.0f);
    addFloat("oscASpread", "Osc A Spread", 0.0f, 1.0f, 0.5f);
    addFloat("oscAPulseWidth", "Osc A PW", 0.1f, 0.9f, 0.5f);

    // ---- Oscillator B ----
    addInt("oscBWaveform", "Osc B Waveform", 0, 4, 0);
    addInt("oscBWavetableIndex", "Osc B Wavetable", 0, 23, 0);
    addFloat("oscBWavetablePos", "Osc B WT Pos", 0.0f, 1.0f, 0.0f);
    addFloat("oscBLevel", "Osc B Level", 0.0f, 1.0f, 0.0f);
    addInt("oscBCoarse", "Osc B Coarse", -24, 24, 0);
    addFloat("oscBFine", "Osc B Fine", -100.0f, 100.0f, 0.0f);
    addInt("oscBUnison", "Osc B Unison", 1, 8, 1);
    addFloat("oscBDetune", "Osc B Detune", 0.0f, 100.0f, 0.0f);
    addFloat("oscBSpread", "Osc B Spread", 0.0f, 1.0f, 0.5f);
    addFloat("oscBPulseWidth", "Osc B PW", 0.1f, 0.9f, 0.5f);

    // ---- Granular ----
    addBool("granEnabled", "Granular Enabled", false);
    addInt("granSource", "Grain Source", 0, 21, 0);
    addFloat("granPosition", "Grain Position", 0.0f, 1.0f, 0.5f);
    addFloat("granPosRand", "Grain Pos Rand", 0.0f, 1.0f, 0.0f);
    addFloat("granSize", "Grain Size", 5.0f, 500.0f, 80.0f);
    addFloat("granDensity", "Grain Density", 1.0f, 40.0f, 10.0f);
    addInt("granPitch", "Grain Pitch", -24, 24, 0);
    addInt("granPitchRand", "Grain Pitch Rand", 0, 12, 0);
    addInt("granShape", "Grain Shape", 0, 2, 0);
    addFloat("granPanSpread", "Grain Pan Spread", 0.0f, 1.0f, 0.3f);
    addFloat("granLevel", "Grain Level", 0.0f, 1.0f, 0.0f);

    // ---- Sub Oscillator ----
    addInt("subShape", "Sub Shape", 0, 1, 0);
    addInt("subOctave", "Sub Octave", 0, 1, 0);
    addFloat("subLevel", "Sub Level", 0.0f, 1.0f, 0.0f);

    // ---- Distortion ----
    addInt("distMode", "Dist Mode", 0, 4, 0);
    addFloat("distDrive", "Dist Drive", 1.0f, 50.0f, 1.0f);
    addFloat("distMix", "Dist Mix", 0.0f, 1.0f, 0.0f);

    // ---- Filter ----
    addInt("filterType", "Filter Type", 0, 4, 0);
    addFloat("filterCutoff", "Filter Cutoff", 20.0f, 20000.0f, 20000.0f, 0.25f); // exponential skew
    addFloat("filterRes", "Filter Resonance", 0.0f, 1.0f, 0.0f);
    addFloat("filterEnvAmt", "Filter Env Amt", -1.0f, 1.0f, 0.0f);
    addFloat("filterKeyTrack", "Filter Key Track", 0.0f, 1.0f, 0.0f);
    addFloat("filterDrive", "Filter Drive", 1.0f, 5.0f, 1.0f);

    // ---- Amp Envelope ----
    addFloat("ampA", "Amp Attack", 0.001f, 5.0f, 0.005f, 0.3f);
    addFloat("ampD", "Amp Decay", 0.001f, 5.0f, 0.3f, 0.3f);
    addFloat("ampS", "Amp Sustain", 0.0f, 1.0f, 0.8f);
    addFloat("ampR", "Amp Release", 0.001f, 10.0f, 0.1f, 0.3f);

    // ---- Filter Envelope ----
    addFloat("filtEnvA", "Filt Env Attack", 0.001f, 5.0f, 0.005f, 0.3f);
    addFloat("filtEnvD", "Filt Env Decay", 0.001f, 5.0f, 0.2f, 0.3f);
    addFloat("filtEnvS", "Filt Env Sustain", 0.0f, 1.0f, 0.5f);
    addFloat("filtEnvR", "Filt Env Release", 0.001f, 10.0f, 0.3f, 0.3f);

    // ---- LFO 1 ----
    addFloat("lfo1Rate", "LFO 1 Rate", 0.01f, 30.0f, 2.0f);
    addInt("lfo1Shape", "LFO 1 Shape", 0, 4, 0);
    addBool("lfo1Sync", "LFO 1 Sync", false);
    addFloat("lfo1Amount", "LFO 1 Amount", 0.0f, 1.0f, 0.0f);

    // ---- LFO 2 ----
    addFloat("lfo2Rate", "LFO 2 Rate", 0.01f, 30.0f, 2.0f);
    addInt("lfo2Shape", "LFO 2 Shape", 0, 4, 0);
    addBool("lfo2Sync", "LFO 2 Sync", false);
    addFloat("lfo2Amount", "LFO 2 Amount", 0.0f, 1.0f, 0.0f);

    // ---- FX: Chorus ----
    addFloat("chorusRate", "Chorus Rate", 0.1f, 5.0f, 0.8f);
    addFloat("chorusDepth", "Chorus Depth", 0.0f, 1.0f, 0.3f);
    addFloat("chorusMix", "Chorus Mix", 0.0f, 1.0f, 0.0f);

    // ---- FX: Delay ----
    addFloat("delayTime", "Delay Time", 1.0f, 2000.0f, 375.0f);
    addFloat("delayFeedback", "Delay Feedback", 0.0f, 0.95f, 0.4f);
    addFloat("delayMix", "Delay Mix", 0.0f, 1.0f, 0.0f);
    addBool("delaySync", "Delay Sync", true);
    addFloat("delayFilter", "Delay Filter", 500.0f, 15000.0f, 8000.0f, 0.3f);

    // ---- FX: Reverb ----
    addFloat("reverbSize", "Reverb Size", 0.0f, 1.0f, 0.5f);
    addFloat("reverbDamp", "Reverb Damping", 0.0f, 1.0f, 0.5f);
    addFloat("reverbMix", "Reverb Mix", 0.0f, 1.0f, 0.0f);

    // ---- Global ----
    addInt("voiceMode", "Voice Mode", 0, 2, 0);
    addInt("maxVoices", "Max Voices", 1, 16, 8);
    addFloat("glideTime", "Glide Time", 0.0f, 2000.0f, 0.0f);
    addFloat("masterVol", "Master Volume", 0.0f, 1.0f, 0.75f);

    // ---- Macros ----
    addFloat("macro1", "Macro 1", 0.0f, 1.0f, 0.0f);
    addFloat("macro2", "Macro 2", 0.0f, 1.0f, 0.0f);
    addFloat("macro3", "Macro 3", 0.0f, 1.0f, 0.0f);
    addFloat("macro4", "Macro 4", 0.0f, 1.0f, 0.0f);

    return { params.begin(), params.end() };
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new VoidSynthAudioProcessor();
}
