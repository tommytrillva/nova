#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "OscillatorEngine.h"
#include "SubOscillator.h"
#include "GranularEngine.h"
#include "DistortionProcessor.h"
#include "MultiFilter.h"

class SynthVoice : public juce::SynthesiserVoice
{
public:
    explicit SynthVoice(juce::AudioProcessorValueTreeState& apvts);

    bool canPlaySound(juce::SynthesiserSound*) override;
    void startNote(int midiNoteNumber, float velocity, juce::SynthesiserSound*, int currentPitchWheelPosition) override;
    void stopNote(float velocity, bool allowTailOff) override;
    void pitchWheelMoved(int newPitchWheelValue) override;
    void controllerMoved(int controllerNumber, int newControllerValue) override;
    void renderNextBlock(juce::AudioBuffer<float>& outputBuffer, int startSample, int numSamples) override;

    void prepareToPlay(double sampleRate, int samplesPerBlock);
    void updateParams(double bpm);

private:
    juce::AudioProcessorValueTreeState& apvts;

    OscillatorEngine oscA, oscB;
    SubOscillator sub;
    GranularEngine granular;
    DistortionProcessor distortion;
    MultiFilter filter;

    // ADSR Envelopes
    struct ADSREnvelope
    {
        enum class Stage { Idle, Attack, Decay, Sustain, Release };
        Stage stage = Stage::Idle;
        float value = 0.0f;
        float attackCoeff = 0.0f, decayCoeff = 0.0f, releaseCoeff = 0.0f;
        float sustainLevel = 0.8f;
        float attackTarget = 1.3f; // overshoot

        void setParams(float a, float d, float s, float r, double sampleRate);
        float process();
        void noteOn();
        void noteOff();
        bool isActive() const { return stage != Stage::Idle; }
    };

    ADSREnvelope ampEnv, filterEnv;

    // LFOs
    struct LFO
    {
        float phase = 0.0f;
        float rate = 2.0f;
        int shape = 0; // 0=Sine,1=Saw,2=Square,3=Tri,4=S&H
        float amount = 0.0f;
        bool tempoSync = false;
        float shValue = 0.0f; // sample-and-hold value
        float prevPhase = 0.0f;
        juce::Random rng;

        float process(double sampleRate, double bpm);
        float getSyncedRate(double bpm);
    };

    LFO lfo1, lfo2;

    float velocity = 0.0f;
    int currentMidiNote = 60;
    double currentSampleRate = 44100.0;
    bool granularEnabled = false;

    float midiNoteToFreq(int note);
};
