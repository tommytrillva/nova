#include "FXChain.h"

void FXChain::prepareToPlay(double sr, int samplesPerBlock)
{
    sampleRate = sr;

    chorusBufferL.assign(CHORUS_BUFFER_SIZE, 0.0f);
    chorusBufferR.assign(CHORUS_BUFFER_SIZE, 0.0f);
    chorusWritePos = 0;
    chorusLfoPhase = 0.0f;

    delayBufferL.assign(MAX_DELAY_SAMPLES, 0.0f);
    delayBufferR.assign(MAX_DELAY_SAMPLES, 0.0f);
    delayWritePos = 0;
    delayFilterStateL = 0.0f;
    delayFilterStateR = 0.0f;

    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sr;
    spec.maximumBlockSize = (juce::uint32)samplesPerBlock;
    spec.numChannels = 2;
    reverb.prepare(spec);
}

void FXChain::updateFromParams(juce::AudioProcessorValueTreeState& apvts)
{
    chorusRate = apvts.getRawParameterValue("chorusRate")->load();
    chorusDepth = apvts.getRawParameterValue("chorusDepth")->load();
    chorusMix = apvts.getRawParameterValue("chorusMix")->load();
    delayTimeMs = apvts.getRawParameterValue("delayTime")->load();
    delayFeedback = apvts.getRawParameterValue("delayFeedback")->load();
    delayMix = apvts.getRawParameterValue("delayMix")->load();
    delaySync = apvts.getRawParameterValue("delaySync")->load() > 0.5f;
    delayFilterCutoff = apvts.getRawParameterValue("delayFilter")->load();
    reverbSize = apvts.getRawParameterValue("reverbSize")->load();
    reverbDamping = apvts.getRawParameterValue("reverbDamp")->load();
    reverbMix = apvts.getRawParameterValue("reverbMix")->load();
}

void FXChain::process(juce::AudioBuffer<float>& buffer, double bpm)
{
    processChorus(buffer);
    processDelay(buffer, bpm);
    processReverb(buffer);
}

void FXChain::processChorus(juce::AudioBuffer<float>& buffer)
{
    if (chorusMix <= 0.0f) return;

    auto* left = buffer.getWritePointer(0);
    auto* right = buffer.getWritePointer(1);
    int numSamples = buffer.getNumSamples();

    float lfoInc = chorusRate / (float)sampleRate;
    float baseDelaySamples = 7.0f * 0.001f * (float)sampleRate; // 7ms
    float modDepthSamples = 3.0f * 0.001f * (float)sampleRate * chorusDepth; // up to 3ms

    for (int i = 0; i < numSamples; ++i)
    {
        // Write to circular buffer
        chorusBufferL[chorusWritePos] = left[i];
        chorusBufferR[chorusWritePos] = right[i];

        // LFO modulation (opposite phase for L/R)
        float lfoL = std::sin(2.0f * juce::MathConstants<float>::pi * chorusLfoPhase);
        float lfoR = std::sin(2.0f * juce::MathConstants<float>::pi * chorusLfoPhase + juce::MathConstants<float>::pi);

        float delayL = baseDelaySamples + lfoL * modDepthSamples;
        float delayR = baseDelaySamples + lfoR * modDepthSamples;

        // Read with linear interpolation
        auto readBuf = [&](const std::vector<float>& buf, float delaySamp) -> float
        {
            float readPos = (float)chorusWritePos - delaySamp;
            if (readPos < 0.0f) readPos += (float)CHORUS_BUFFER_SIZE;
            int idx0 = (int)readPos;
            int idx1 = (idx0 + 1) % CHORUS_BUFFER_SIZE;
            float frac = readPos - (float)idx0;
            return buf[idx0 % CHORUS_BUFFER_SIZE] + frac * (buf[idx1] - buf[idx0 % CHORUS_BUFFER_SIZE]);
        };

        float wetL = readBuf(chorusBufferL, delayL);
        float wetR = readBuf(chorusBufferR, delayR);

        left[i] = left[i] * (1.0f - chorusMix) + wetL * chorusMix;
        right[i] = right[i] * (1.0f - chorusMix) + wetR * chorusMix;

        chorusWritePos = (chorusWritePos + 1) % CHORUS_BUFFER_SIZE;
        chorusLfoPhase += lfoInc;
        if (chorusLfoPhase >= 1.0f) chorusLfoPhase -= 1.0f;
    }
}

float FXChain::syncedDelayTime(double bpm)
{
    // Snap to nearest subdivision
    // delayTimeMs is the free-running time; if sync is on, quantize to beat divisions
    if (bpm <= 0.0) bpm = 120.0;
    float beatMs = 60000.0f / (float)bpm;

    // Available divisions (in beats): 4, 2, 1, 1/2, 1/4, 1/8, 1/16, 1/32
    float divisions[] = { 4.0f, 2.0f, 1.0f, 0.5f, 0.25f, 0.125f, 0.0625f, 0.03125f };
    float targetMs = delayTimeMs;
    float bestMs = beatMs;

    for (auto div : divisions)
    {
        float ms = beatMs * div;
        if (std::abs(ms - targetMs) < std::abs(bestMs - targetMs))
            bestMs = ms;
    }

    return bestMs;
}

void FXChain::processDelay(juce::AudioBuffer<float>& buffer, double bpm)
{
    if (delayMix <= 0.0f) return;

    auto* left = buffer.getWritePointer(0);
    auto* right = buffer.getWritePointer(1);
    int numSamples = buffer.getNumSamples();

    float actualTimeMs = delaySync ? syncedDelayTime(bpm) : delayTimeMs;
    float delaySamples = actualTimeMs * 0.001f * (float)sampleRate;
    delaySamples = juce::jlimit(1.0f, (float)(MAX_DELAY_SAMPLES - 1), delaySamples);

    // One-pole LP filter coefficient for feedback path
    float fc = delayFilterCutoff / (float)sampleRate;
    float lpCoeff = 1.0f - std::exp(-2.0f * juce::MathConstants<float>::pi * fc);

    for (int i = 0; i < numSamples; ++i)
    {
        // Read from delay buffers (ping-pong: L reads from R, R reads from L)
        float readPosF = (float)delayWritePos - delaySamples;
        if (readPosF < 0.0f) readPosF += (float)MAX_DELAY_SAMPLES;
        int idx0 = (int)readPosF;
        int idx1 = (idx0 + 1) % MAX_DELAY_SAMPLES;
        float frac = readPosF - (float)idx0;

        float delOutL = delayBufferL[idx0] + frac * (delayBufferL[idx1] - delayBufferL[idx0]);
        float delOutR = delayBufferR[idx0] + frac * (delayBufferR[idx1] - delayBufferR[idx0]);

        // LP filter on feedback
        delayFilterStateL += lpCoeff * (delOutL - delayFilterStateL);
        delayFilterStateR += lpCoeff * (delOutR - delayFilterStateR);

        // Ping-pong: swap channels in feedback
        delayBufferL[delayWritePos] = left[i] + delayFilterStateR * delayFeedback;
        delayBufferR[delayWritePos] = right[i] + delayFilterStateL * delayFeedback;

        left[i] = left[i] * (1.0f - delayMix) + delOutL * delayMix;
        right[i] = right[i] * (1.0f - delayMix) + delOutR * delayMix;

        delayWritePos = (delayWritePos + 1) % MAX_DELAY_SAMPLES;
    }
}

void FXChain::processReverb(juce::AudioBuffer<float>& buffer)
{
    if (reverbMix <= 0.0f) return;

    // Save dry signal
    juce::AudioBuffer<float> dry;
    dry.makeCopyOf(buffer);

    reverbParams.roomSize = reverbSize;
    reverbParams.damping = reverbDamping;
    reverbParams.wetLevel = 1.0f;
    reverbParams.dryLevel = 0.0f;
    reverbParams.width = 1.0f;
    reverb.setParameters(reverbParams);

    juce::dsp::AudioBlock<float> block(buffer);
    juce::dsp::ProcessContextReplacing<float> context(block);
    reverb.process(context);

    // Mix wet/dry
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        auto* wet = buffer.getWritePointer(ch);
        auto* dryPtr = dry.getReadPointer(ch);
        for (int i = 0; i < buffer.getNumSamples(); ++i)
            wet[i] = dryPtr[i] * (1.0f - reverbMix) + wet[i] * reverbMix;
    }
}
