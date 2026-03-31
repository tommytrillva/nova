#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <cmath>
#include <vector>
#include <array>

class OscillatorEngine
{
public:
    OscillatorEngine();

    void prepareToPlay(double sampleRate);
    void setFrequency(float freq);
    void setWaveform(int waveform); // 0=Saw, 1=Square, 2=Tri, 3=Sine, 4=Wavetable
    void setWavetableIndex(int index);
    void setWavetablePosition(float pos);
    void setPulseWidth(float pw);
    void setUnison(int count, float detuneCents, float spread);
    void setCoarseTune(int semitones);
    void setFineTune(float cents);
    void setGlideTime(float timeMs);

    // Returns stereo pair (left, right)
    std::pair<float, float> processSample();

    void noteOn();
    void noteOff();
    void setTargetFrequency(float freq);
    void updateGlide();

    // Static wavetable loading
    static void loadWavetables();
    static bool wavetablesLoaded;

private:
    double sampleRate = 44100.0;
    int waveform = 0;
    float pulseWidth = 0.5f;

    // Pitch
    float baseFrequency = 440.0f;
    float currentFrequency = 440.0f;
    float targetFrequency = 440.0f;
    float glideCoeff = 1.0f;
    int coarseTune = 0;
    float fineTune = 0.0f;

    // Unison
    int unisonCount = 1;
    float detuneAmount = 0.0f;
    float unisonSpread = 0.5f;

    // Per-unison-voice state
    struct UnisonVoice
    {
        double phase = 0.0;
        double phaseInc = 0.0;
        float pan = 0.0f; // -1 to 1
        float detuneRatio = 1.0f;
    };
    std::array<UnisonVoice, 8> unisonVoices;

    // Wavetable
    int wavetableIndex = 0;
    float wavetablePosition = 0.0f;

    static constexpr int WAVETABLE_SIZE = 2048;
    static constexpr int MAX_WAVETABLES = 24;
    struct WavetableData
    {
        std::vector<float> data; // may contain multiple frames of WAVETABLE_SIZE
        int numFrames = 1;
    };
    static std::array<WavetableData, MAX_WAVETABLES> wavetables;

    float polyBLEP(float t, float dt);
    float generateSaw(UnisonVoice& v);
    float generateSquare(UnisonVoice& v);
    float generateTriangle(UnisonVoice& v);
    float generateSine(UnisonVoice& v);
    float generateWavetable(UnisonVoice& v);

    void updatePhaseIncrements();
};
