#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <vector>
#include <cmath>

class FXChain
{
public:
    void prepareToPlay(double sampleRate, int samplesPerBlock);
    void process(juce::AudioBuffer<float>& buffer, double bpm);

    // Chorus
    void setChorusRate(float rate) { chorusRate = rate; }
    void setChorusDepth(float depth) { chorusDepth = depth; }
    void setChorusMix(float mix) { chorusMix = mix; }

    // Delay
    void setDelayTime(float timeMs) { delayTimeMs = timeMs; }
    void setDelayFeedback(float fb) { delayFeedback = fb; }
    void setDelayMix(float mix) { delayMix = mix; }
    void setDelaySync(bool sync) { delaySync = sync; }
    void setDelayFilter(float cutoff) { delayFilterCutoff = cutoff; }

    // Reverb
    void setReverbSize(float size) { reverbSize = size; }
    void setReverbDamping(float damp) { reverbDamping = damp; }
    void setReverbMix(float mix) { reverbMix = mix; }

    // Bulk parameter update from APVTS
    void updateFromParams(juce::AudioProcessorValueTreeState& apvts);

private:
    double sampleRate = 44100.0;

    // Chorus
    float chorusRate = 0.8f;
    float chorusDepth = 0.3f;
    float chorusMix = 0.0f;
    std::vector<float> chorusBufferL, chorusBufferR;
    int chorusWritePos = 0;
    float chorusLfoPhase = 0.0f;
    static constexpr int CHORUS_BUFFER_SIZE = 8192;

    // Delay
    float delayTimeMs = 375.0f;
    float delayFeedback = 0.4f;
    float delayMix = 0.0f;
    bool delaySync = true;
    float delayFilterCutoff = 8000.0f;
    std::vector<float> delayBufferL, delayBufferR;
    int delayWritePos = 0;
    static constexpr int MAX_DELAY_SAMPLES = 192000; // 2 sec @ 96kHz
    // Feedback filter state
    float delayFilterStateL = 0.0f;
    float delayFilterStateR = 0.0f;

    // Reverb
    float reverbSize = 0.5f;
    float reverbDamping = 0.5f;
    float reverbMix = 0.0f;
    juce::dsp::Reverb reverb;
    juce::dsp::Reverb::Parameters reverbParams;

    void processChorus(juce::AudioBuffer<float>& buffer);
    void processDelay(juce::AudioBuffer<float>& buffer, double bpm);
    void processReverb(juce::AudioBuffer<float>& buffer);

    float readDelay(const std::vector<float>& buf, float delaySamples);
    float syncedDelayTime(double bpm);
};
