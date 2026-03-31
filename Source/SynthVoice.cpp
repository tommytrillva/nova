#include "SynthVoice.h"
#include "SynthSound.h"

// ---- ADSR Envelope ----
void SynthVoice::ADSREnvelope::setParams(float a, float d, float s, float r, double sampleRate)
{
    sustainLevel = s;
    attackCoeff = 1.0f - std::exp(-1.0f / (a * (float)sampleRate));
    decayCoeff = 1.0f - std::exp(-1.0f / (d * (float)sampleRate));
    releaseCoeff = 1.0f - std::exp(-1.0f / (r * (float)sampleRate));
    attackTarget = 1.0f + 0.3f; // overshoot
}

float SynthVoice::ADSREnvelope::process()
{
    switch (stage)
    {
        case Stage::Attack:
            value += (attackTarget - value) * attackCoeff;
            if (value >= 1.0f)
            {
                value = 1.0f;
                stage = Stage::Decay;
            }
            break;

        case Stage::Decay:
            value += (sustainLevel - value) * decayCoeff;
            if (std::abs(value - sustainLevel) < 0.0001f)
            {
                value = sustainLevel;
                stage = Stage::Sustain;
            }
            break;

        case Stage::Sustain:
            value = sustainLevel;
            break;

        case Stage::Release:
            value += (0.0f - value) * releaseCoeff;
            if (value < 0.0001f)
            {
                value = 0.0f;
                stage = Stage::Idle;
            }
            break;

        case Stage::Idle:
            value = 0.0f;
            break;
    }
    return value;
}

void SynthVoice::ADSREnvelope::noteOn()
{
    stage = Stage::Attack;
    // Don't reset value for legato smoothness
}

void SynthVoice::ADSREnvelope::noteOff()
{
    if (stage != Stage::Idle)
        stage = Stage::Release;
}

// ---- LFO ----
float SynthVoice::LFO::getSyncedRate(double bpm)
{
    if (bpm <= 0.0) bpm = 120.0;
    // Map rate parameter to tempo divisions
    // rate 0-30 mapped to different note divisions
    float beatHz = (float)(bpm / 60.0);

    // Simple mapping: rate value determines division
    if (rate < 1.0f) return beatHz * 0.0625f;      // 4 bars
    if (rate < 2.0f) return beatHz * 0.125f;        // 2 bars
    if (rate < 4.0f) return beatHz * 0.25f;         // 1 bar
    if (rate < 6.0f) return beatHz * 0.5f;          // 1/2
    if (rate < 10.0f) return beatHz;                 // 1/4
    if (rate < 15.0f) return beatHz * 2.0f;          // 1/8
    if (rate < 22.0f) return beatHz * 4.0f;          // 1/16
    return beatHz * 8.0f;                             // 1/32
}

float SynthVoice::LFO::process(double sampleRate, double bpm)
{
    if (amount <= 0.0f) return 0.0f;

    float actualRate = tempoSync ? getSyncedRate(bpm) : rate;
    float phaseInc = actualRate / (float)sampleRate;

    prevPhase = phase;
    phase += phaseInc;
    if (phase >= 1.0f) phase -= 1.0f;

    float out = 0.0f;
    switch (shape)
    {
        case 0: // Sine
            out = std::sin(2.0f * juce::MathConstants<float>::pi * phase);
            break;
        case 1: // Saw
            out = 2.0f * phase - 1.0f;
            break;
        case 2: // Square
            out = phase < 0.5f ? 1.0f : -1.0f;
            break;
        case 3: // Triangle
            out = phase < 0.5f ? (4.0f * phase - 1.0f) : (3.0f - 4.0f * phase);
            break;
        case 4: // S&H
            if (phase < prevPhase) // wrapped around
                shValue = rng.nextFloat() * 2.0f - 1.0f;
            out = shValue;
            break;
    }

    return out * amount;
}

// ---- SynthVoice ----
SynthVoice::SynthVoice(juce::AudioProcessorValueTreeState& a) : apvts(a) {}

bool SynthVoice::canPlaySound(juce::SynthesiserSound* sound)
{
    return dynamic_cast<SynthSound*>(sound) != nullptr;
}

void SynthVoice::prepareToPlay(double sampleRate, int)
{
    currentSampleRate = sampleRate;
    oscA.prepareToPlay(sampleRate);
    oscB.prepareToPlay(sampleRate);
    sub.prepareToPlay(sampleRate);
    granular.prepareToPlay(sampleRate);
    distortion.prepareToPlay(sampleRate);
    filter.prepareToPlay(sampleRate);
}

float SynthVoice::midiNoteToFreq(int note)
{
    return 440.0f * std::pow(2.0f, (note - 69) / 12.0f);
}

void SynthVoice::startNote(int midiNoteNumber, float vel, juce::SynthesiserSound*, int)
{
    velocity = vel;
    currentMidiNote = midiNoteNumber;
    float freq = midiNoteToFreq(midiNoteNumber);

    float glideTime = apvts.getRawParameterValue("glideTime")->load();
    int voiceMode = (int)apvts.getRawParameterValue("voiceMode")->load();

    if (voiceMode > 0 && glideTime > 0.0f) // mono or legato
    {
        oscA.setGlideTime(glideTime);
        oscB.setGlideTime(glideTime);
        oscA.setTargetFrequency(freq);
        oscB.setTargetFrequency(freq);
    }
    else
    {
        oscA.setGlideTime(0.0f);
        oscB.setGlideTime(0.0f);
        oscA.setFrequency(freq);
        oscB.setFrequency(freq);
    }

    sub.setFrequency(freq);
    oscA.noteOn();
    oscB.noteOn();
    sub.noteOn();
    granular.noteOn(midiNoteNumber, vel);
    ampEnv.noteOn();
    filterEnv.noteOn();
    filter.reset();
}

void SynthVoice::stopNote(float, bool allowTailOff)
{
    if (allowTailOff)
    {
        ampEnv.noteOff();
        filterEnv.noteOff();
        granular.noteOff();
    }
    else
    {
        ampEnv.stage = ADSREnvelope::Stage::Idle;
        ampEnv.value = 0.0f;
        clearCurrentNote();
    }
}

void SynthVoice::pitchWheelMoved(int) {}
void SynthVoice::controllerMoved(int, int) {}

void SynthVoice::updateParams(double bpm)
{
    // Oscillator A
    oscA.setWaveform((int)apvts.getRawParameterValue("oscAWaveform")->load());
    oscA.setWavetableIndex((int)apvts.getRawParameterValue("oscAWavetableIndex")->load());
    oscA.setWavetablePosition(apvts.getRawParameterValue("oscAWavetablePos")->load());
    oscA.setCoarseTune((int)apvts.getRawParameterValue("oscACoarse")->load());
    oscA.setFineTune(apvts.getRawParameterValue("oscAFine")->load());
    oscA.setPulseWidth(apvts.getRawParameterValue("oscAPulseWidth")->load());
    oscA.setUnison((int)apvts.getRawParameterValue("oscAUnison")->load(),
                   apvts.getRawParameterValue("oscADetune")->load(),
                   apvts.getRawParameterValue("oscASpread")->load());

    // Oscillator B
    oscB.setWaveform((int)apvts.getRawParameterValue("oscBWaveform")->load());
    oscB.setWavetableIndex((int)apvts.getRawParameterValue("oscBWavetableIndex")->load());
    oscB.setWavetablePosition(apvts.getRawParameterValue("oscBWavetablePos")->load());
    oscB.setCoarseTune((int)apvts.getRawParameterValue("oscBCoarse")->load());
    oscB.setFineTune(apvts.getRawParameterValue("oscBFine")->load());
    oscB.setPulseWidth(apvts.getRawParameterValue("oscBPulseWidth")->load());
    oscB.setUnison((int)apvts.getRawParameterValue("oscBUnison")->load(),
                   apvts.getRawParameterValue("oscBDetune")->load(),
                   apvts.getRawParameterValue("oscBSpread")->load());

    // Sub
    sub.setShape((int)apvts.getRawParameterValue("subShape")->load());
    sub.setOctave((int)apvts.getRawParameterValue("subOctave")->load());
    sub.setLevel(apvts.getRawParameterValue("subLevel")->load());

    // Granular
    granularEnabled = apvts.getRawParameterValue("granEnabled")->load() > 0.5f;
    granular.setSource((int)apvts.getRawParameterValue("granSource")->load());
    granular.setPosition(apvts.getRawParameterValue("granPosition")->load());
    granular.setPositionRand(apvts.getRawParameterValue("granPosRand")->load());
    granular.setGrainSize(apvts.getRawParameterValue("granSize")->load());
    granular.setDensity(apvts.getRawParameterValue("granDensity")->load());
    granular.setPitch((int)apvts.getRawParameterValue("granPitch")->load());
    granular.setPitchRand((int)apvts.getRawParameterValue("granPitchRand")->load());
    granular.setShape((int)apvts.getRawParameterValue("granShape")->load());
    granular.setPanSpread(apvts.getRawParameterValue("granPanSpread")->load());
    granular.setLevel(apvts.getRawParameterValue("granLevel")->load());

    // Distortion
    distortion.setMode((int)apvts.getRawParameterValue("distMode")->load());
    distortion.setDrive(apvts.getRawParameterValue("distDrive")->load());
    distortion.setMix(apvts.getRawParameterValue("distMix")->load());

    // Filter
    filter.setType((int)apvts.getRawParameterValue("filterType")->load());
    filter.setResonance(apvts.getRawParameterValue("filterRes")->load());
    filter.setDrive(apvts.getRawParameterValue("filterDrive")->load());

    // Envelopes
    ampEnv.setParams(apvts.getRawParameterValue("ampA")->load(),
                     apvts.getRawParameterValue("ampD")->load(),
                     apvts.getRawParameterValue("ampS")->load(),
                     apvts.getRawParameterValue("ampR")->load(),
                     currentSampleRate);

    filterEnv.setParams(apvts.getRawParameterValue("filtEnvA")->load(),
                        apvts.getRawParameterValue("filtEnvD")->load(),
                        apvts.getRawParameterValue("filtEnvS")->load(),
                        apvts.getRawParameterValue("filtEnvR")->load(),
                        currentSampleRate);

    // LFOs
    lfo1.rate = apvts.getRawParameterValue("lfo1Rate")->load();
    lfo1.shape = (int)apvts.getRawParameterValue("lfo1Shape")->load();
    lfo1.tempoSync = apvts.getRawParameterValue("lfo1Sync")->load() > 0.5f;
    lfo1.amount = apvts.getRawParameterValue("lfo1Amount")->load();

    lfo2.rate = apvts.getRawParameterValue("lfo2Rate")->load();
    lfo2.shape = (int)apvts.getRawParameterValue("lfo2Shape")->load();
    lfo2.tempoSync = apvts.getRawParameterValue("lfo2Sync")->load() > 0.5f;
    lfo2.amount = apvts.getRawParameterValue("lfo2Amount")->load();
}

void SynthVoice::renderNextBlock(juce::AudioBuffer<float>& outputBuffer, int startSample, int numSamples)
{
    if (!ampEnv.isActive()) return;

    float oscALevel = apvts.getRawParameterValue("oscALevel")->load();
    float oscBLevel = apvts.getRawParameterValue("oscBLevel")->load();
    float filterCutoff = apvts.getRawParameterValue("filterCutoff")->load();
    float filterEnvAmt = apvts.getRawParameterValue("filterEnvAmt")->load();
    float filterKeyTrack = apvts.getRawParameterValue("filterKeyTrack")->load();

    double bpm = 120.0; // Updated from processor

    for (int sample = 0; sample < numSamples; ++sample)
    {
        // LFO modulation
        float lfo1Val = lfo1.process(currentSampleRate, bpm);
        float lfo2Val = lfo2.process(currentSampleRate, bpm);

        // Oscillators
        auto [oscAL, oscAR] = oscA.processSample();
        auto [oscBL, oscBR] = oscB.processSample();
        float subSample = sub.processSample();

        // Granular
        float granL = 0.0f, granR = 0.0f;
        if (granularEnabled)
        {
            auto [gL, gR] = granular.processSample();
            granL = gL;
            granR = gR;
        }

        // Mix (mono for distortion/filter, then split back to stereo)
        float mixL = oscAL * oscALevel + oscBL * oscBLevel + granL + subSample;
        float mixR = oscAR * oscALevel + oscBR * oscBLevel + granR + subSample;

        // Distortion (process each channel)
        mixL = distortion.processSample(mixL);
        mixR = distortion.processSample(mixR);

        // Filter with envelope and key tracking
        float filtEnvVal = filterEnv.process();
        float modCutoff = filterCutoff;

        // LFO1 modulation on cutoff
        modCutoff *= std::pow(2.0f, lfo1Val * 2.0f); // ±2 octaves

        // Envelope modulation
        modCutoff *= std::pow(2.0f, filterEnvAmt * filtEnvVal * 10.0f);

        // Key tracking
        modCutoff *= std::pow(2.0f, filterKeyTrack * (currentMidiNote - 60) / 12.0f);

        modCutoff = juce::jlimit(20.0f, 20000.0f, modCutoff);
        filter.setCutoff(modCutoff);

        mixL = filter.processSample(mixL);
        // Note: SVF is stateful, so for true stereo we'd need two filters.
        // For now, apply same filter to both channels (mono filter on stereo signal is acceptable for v1)

        // Amp envelope
        float ampVal = ampEnv.process();
        if (!ampEnv.isActive())
        {
            clearCurrentNote();
            return;
        }

        float gain = ampVal * velocity;
        outputBuffer.addSample(0, startSample + sample, mixL * gain);
        outputBuffer.addSample(1, startSample + sample, mixR * gain);
    }
}
