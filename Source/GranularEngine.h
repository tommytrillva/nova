#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <vector>
#include <array>
#include <cmath>

class GranularEngine
{
public:
    GranularEngine();

    void prepareToPlay(double sampleRate);
    void loadGrainSources();

    void setSource(int index);
    void setPosition(float pos);
    void setPositionRand(float rand);
    void setGrainSize(float ms);
    void setDensity(float grainsPerSec);
    void setPitch(int semitones);
    void setPitchRand(int semitones);
    void setShape(int shape); // 0=Hann, 1=Tri, 2=Rect
    void setPanSpread(float spread);
    void setLevel(float level);

    void noteOn(int midiNote, float velocity);
    void noteOff();

    // Returns stereo pair
    std::pair<float, float> processSample();

private:
    static constexpr int MAX_GRAINS = 40;
    static constexpr int MAX_SOURCES = 22;

    struct Grain
    {
        bool active = false;
        int sourceIndex = 0;
        float startPos = 0.0f;     // position in source buffer (samples)
        float phase = 0.0f;         // 0..1 progress through grain
        float phaseInc = 0.0f;      // increment per sample
        float playbackRate = 1.0f;
        float readPos = 0.0f;       // current read position
        float pan = 0.0f;           // -1..1
        int shape = 0;
        int lengthSamples = 0;
        int sampleCount = 0;
    };

    struct GrainSource
    {
        std::vector<float> data;
        int numSamples = 0;
    };

    double sampleRate = 44100.0;
    std::array<Grain, MAX_GRAINS> grains;
    static std::array<GrainSource, MAX_SOURCES> sources;
    static bool sourcesLoaded;

    // Parameters
    int currentSource = 0;
    float position = 0.5f;
    float positionRand = 0.0f;
    float grainSizeMs = 80.0f;
    float density = 10.0f;
    int pitchSemitones = 0;
    int pitchRandSemi = 0;
    int grainShape = 0;
    float panSpread = 0.3f;
    float level = 0.0f;

    // Trigger state
    float triggerCounter = 0.0f;
    float triggerInterval = 0.0f;
    bool isActive = false;
    int baseMidiNote = 60;
    float velocityLevel = 1.0f;

    juce::Random rng;

    void spawnGrain();
    float getWindow(float phase, int shape);
    float readSource(int sourceIdx, float pos);
};
