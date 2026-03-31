#include "OscillatorEngine.h"
#include "BinaryData.h"

bool OscillatorEngine::wavetablesLoaded = false;
std::array<OscillatorEngine::WavetableData, OscillatorEngine::MAX_WAVETABLES> OscillatorEngine::wavetables;

OscillatorEngine::OscillatorEngine()
{
    for (auto& v : unisonVoices)
    {
        v.phase = 0.0;
        v.phaseInc = 0.0;
        v.pan = 0.0f;
        v.detuneRatio = 1.0f;
    }
}

void OscillatorEngine::loadWavetables()
{
    if (wavetablesLoaded) return;

    // Map wavetable filenames to indices
    const char* wavetableNames[] = {
        "BasicSaw_wav", "BasicSquare_wav", "SuperSaw_wav",
        "Digital01_wav", "Digital02_wav", "Digital03_wav", "Digital04_wav",
        "Digital05_wav", "Digital06_wav", "Digital07_wav", "Digital08_wav",
        "Formant01_wav", "Formant02_wav", "Formant03_wav", "Formant04_wav",
        "Growl01_wav", "Growl02_wav", "Growl03_wav", "Growl04_wav",
        "Texture01_wav", "Texture02_wav", "Texture03_wav", "Texture04_wav",
        "Texture05_wav", "Texture06_wav"
    };

    for (int i = 0; i < MAX_WAVETABLES && i < (int)(sizeof(wavetableNames) / sizeof(wavetableNames[0])); ++i)
    {
        int dataSize = 0;
        const char* data = BinaryData::getNamedResource(wavetableNames[i], dataSize);
        if (data == nullptr || dataSize == 0) continue;

        // Parse WAV: skip 44-byte header, read 16-bit PCM samples
        if (dataSize < 44) continue;
        const int16_t* pcm = reinterpret_cast<const int16_t*>(data + 44);
        int numSamples = (dataSize - 44) / 2;

        wavetables[i].data.resize(numSamples);
        for (int s = 0; s < numSamples; ++s)
            wavetables[i].data[s] = static_cast<float>(pcm[s]) / 32768.0f;

        wavetables[i].numFrames = std::max(1, numSamples / WAVETABLE_SIZE);
    }

    wavetablesLoaded = true;
}

void OscillatorEngine::prepareToPlay(double sr)
{
    sampleRate = sr;
    loadWavetables();
}

void OscillatorEngine::setFrequency(float freq)
{
    baseFrequency = freq;
    targetFrequency = freq;
    currentFrequency = freq;
    updatePhaseIncrements();
}

void OscillatorEngine::setTargetFrequency(float freq)
{
    targetFrequency = freq;
}

void OscillatorEngine::setWaveform(int wf) { waveform = wf; }
void OscillatorEngine::setWavetableIndex(int idx) { wavetableIndex = juce::jlimit(0, MAX_WAVETABLES - 1, idx); }
void OscillatorEngine::setWavetablePosition(float pos) { wavetablePosition = juce::jlimit(0.0f, 1.0f, pos); }
void OscillatorEngine::setPulseWidth(float pw) { pulseWidth = juce::jlimit(0.1f, 0.9f, pw); }
void OscillatorEngine::setCoarseTune(int semi) { coarseTune = semi; }
void OscillatorEngine::setFineTune(float cents) { fineTune = cents; }

void OscillatorEngine::setGlideTime(float timeMs)
{
    if (timeMs <= 0.0f || sampleRate <= 0.0)
        glideCoeff = 1.0f;
    else
        glideCoeff = 1.0f - std::exp(-1.0f / (timeMs * 0.001f * (float)sampleRate));
}

void OscillatorEngine::setUnison(int count, float detuneCents, float spread)
{
    unisonCount = juce::jlimit(1, 8, count);
    detuneAmount = detuneCents;
    unisonSpread = spread;

    if (unisonCount == 1)
    {
        unisonVoices[0].detuneRatio = 1.0f;
        unisonVoices[0].pan = 0.0f;
    }
    else
    {
        for (int i = 0; i < unisonCount; ++i)
        {
            float t = (float)i / (float)(unisonCount - 1); // 0 to 1
            float detuneSemi = detuneAmount / 100.0f * (t * 2.0f - 1.0f); // symmetric spread
            unisonVoices[i].detuneRatio = std::pow(2.0f, detuneSemi / 12.0f);
            unisonVoices[i].pan = (t * 2.0f - 1.0f) * unisonSpread;
        }
    }
    updatePhaseIncrements();
}

void OscillatorEngine::noteOn()
{
    for (auto& v : unisonVoices)
        v.phase = 0.0;
}

void OscillatorEngine::noteOff() {}

void OscillatorEngine::updateGlide()
{
    if (glideCoeff >= 1.0f)
        currentFrequency = targetFrequency;
    else
        currentFrequency += (targetFrequency - currentFrequency) * glideCoeff;

    updatePhaseIncrements();
}

void OscillatorEngine::updatePhaseIncrements()
{
    float pitchMult = std::pow(2.0f, (coarseTune + fineTune / 100.0f) / 12.0f);
    float freq = currentFrequency * pitchMult;

    for (int i = 0; i < unisonCount; ++i)
        unisonVoices[i].phaseInc = (double)(freq * unisonVoices[i].detuneRatio) / sampleRate;
}

float OscillatorEngine::polyBLEP(float t, float dt)
{
    if (t < dt)
    {
        t /= dt;
        return t + t - t * t - 1.0f;
    }
    else if (t > 1.0f - dt)
    {
        t = (t - 1.0f) / dt;
        return t * t + t + t + 1.0f;
    }
    return 0.0f;
}

float OscillatorEngine::generateSaw(UnisonVoice& v)
{
    float dt = (float)v.phaseInc;
    float t = (float)v.phase;
    float out = 2.0f * t - 1.0f; // naive saw
    out -= polyBLEP(t, dt);
    return out;
}

float OscillatorEngine::generateSquare(UnisonVoice& v)
{
    float dt = (float)v.phaseInc;
    float t = (float)v.phase;
    float out = (t < pulseWidth) ? 1.0f : -1.0f; // naive square
    out -= polyBLEP(t, dt);
    out += polyBLEP(std::fmod(t + (1.0f - pulseWidth), 1.0f), dt);
    return out;
}

float OscillatorEngine::generateTriangle(UnisonVoice& v)
{
    // Integrated square approach with leaky integrator
    float sq = generateSquare(v);
    // Use a simple leaky integration per sample (stateless approximation via phase)
    float t = (float)v.phase;
    float tri;
    if (t < 0.5f)
        tri = 4.0f * t - 1.0f;
    else
        tri = 3.0f - 4.0f * t;
    return tri;
}

float OscillatorEngine::generateSine(UnisonVoice& v)
{
    return std::sin(2.0f * juce::MathConstants<float>::pi * (float)v.phase);
}

float OscillatorEngine::generateWavetable(UnisonVoice& v)
{
    auto& wt = wavetables[wavetableIndex];
    if (wt.data.empty()) return 0.0f;

    // Determine which frame(s) to read based on wavetablePosition
    float framePos = wavetablePosition * (float)(wt.numFrames - 1);
    int frame0 = (int)framePos;
    int frame1 = std::min(frame0 + 1, wt.numFrames - 1);
    float frameFrac = framePos - (float)frame0;

    // Read position within frame
    float readPos = (float)v.phase * (float)WAVETABLE_SIZE;
    int idx0 = (int)readPos;
    int idx1 = (idx0 + 1) % WAVETABLE_SIZE;
    float frac = readPos - (float)idx0;

    // Linear interpolation within each frame
    int offset0 = frame0 * WAVETABLE_SIZE;
    int offset1 = frame1 * WAVETABLE_SIZE;

    auto safeRead = [&](int offset, int idx) -> float
    {
        int pos = offset + idx;
        if (pos >= 0 && pos < (int)wt.data.size())
            return wt.data[pos];
        return 0.0f;
    };

    float s0 = safeRead(offset0, idx0) + frac * (safeRead(offset0, idx1) - safeRead(offset0, idx0));
    float s1 = safeRead(offset1, idx0) + frac * (safeRead(offset1, idx1) - safeRead(offset1, idx0));

    return s0 + frameFrac * (s1 - s0);
}

std::pair<float, float> OscillatorEngine::processSample()
{
    updateGlide();

    float left = 0.0f, right = 0.0f;
    float gain = 1.0f / std::sqrt((float)unisonCount);

    for (int i = 0; i < unisonCount; ++i)
    {
        auto& v = unisonVoices[i];
        float sample = 0.0f;

        switch (waveform)
        {
            case 0: sample = generateSaw(v); break;
            case 1: sample = generateSquare(v); break;
            case 2: sample = generateTriangle(v); break;
            case 3: sample = generateSine(v); break;
            case 4: sample = generateWavetable(v); break;
            default: break;
        }

        sample *= gain;

        // Pan law
        float panL = std::cos((v.pan + 1.0f) * 0.25f * juce::MathConstants<float>::pi);
        float panR = std::sin((v.pan + 1.0f) * 0.25f * juce::MathConstants<float>::pi);
        left += sample * panL;
        right += sample * panR;

        // Advance phase
        v.phase += v.phaseInc;
        if (v.phase >= 1.0)
            v.phase -= 1.0;
    }

    return { left, right };
}
