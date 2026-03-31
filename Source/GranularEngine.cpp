#include "GranularEngine.h"
#include "BinaryData.h"

std::array<GranularEngine::GrainSource, GranularEngine::MAX_SOURCES> GranularEngine::sources;
bool GranularEngine::sourcesLoaded = false;

GranularEngine::GranularEngine()
{
    for (auto& g : grains)
        g.active = false;
}

void GranularEngine::loadGrainSources()
{
    if (sourcesLoaded) return;

    const char* sourceNames[] = {
        "VoxChop01_wav", "VoxChop02_wav", "VoxChop03_wav", "VoxChop04_wav",
        "VoxChop05_wav", "VoxChop06_wav", "VoxChop07_wav", "VoxChop08_wav",
        "Atmosphere01_wav", "Atmosphere02_wav", "Atmosphere03_wav",
        "Atmosphere04_wav", "Atmosphere05_wav", "Atmosphere06_wav",
        "Perc01_wav", "Perc02_wav", "Perc03_wav", "Perc04_wav",
        "Noise01_wav", "Noise02_wav", "Noise03_wav", "Noise04_wav"
    };

    for (int i = 0; i < MAX_SOURCES; ++i)
    {
        int dataSize = 0;
        const char* data = BinaryData::getNamedResource(sourceNames[i], dataSize);
        if (data == nullptr || dataSize < 44) continue;

        const int16_t* pcm = reinterpret_cast<const int16_t*>(data + 44);
        int numSamples = (dataSize - 44) / 2;

        sources[i].data.resize(numSamples);
        for (int s = 0; s < numSamples; ++s)
            sources[i].data[s] = static_cast<float>(pcm[s]) / 32768.0f;
        sources[i].numSamples = numSamples;
    }

    sourcesLoaded = true;
}

void GranularEngine::prepareToPlay(double sr)
{
    sampleRate = sr;
    loadGrainSources();
    triggerCounter = 0.0f;
}

void GranularEngine::setSource(int idx) { currentSource = juce::jlimit(0, MAX_SOURCES - 1, idx); }
void GranularEngine::setPosition(float pos) { position = pos; }
void GranularEngine::setPositionRand(float r) { positionRand = r; }
void GranularEngine::setGrainSize(float ms) { grainSizeMs = ms; }
void GranularEngine::setDensity(float d) { density = d; }
void GranularEngine::setPitch(int semi) { pitchSemitones = semi; }
void GranularEngine::setPitchRand(int semi) { pitchRandSemi = semi; }
void GranularEngine::setShape(int s) { grainShape = s; }
void GranularEngine::setPanSpread(float s) { panSpread = s; }
void GranularEngine::setLevel(float l) { level = l; }

void GranularEngine::noteOn(int midiNote, float velocity)
{
    isActive = true;
    baseMidiNote = midiNote;
    velocityLevel = velocity;
    triggerCounter = 0.0f;
}

void GranularEngine::noteOff()
{
    isActive = false;
}

void GranularEngine::spawnGrain()
{
    // Find inactive grain slot (or steal oldest)
    int slot = -1;
    int oldestSlot = 0;
    float oldestPhase = 0.0f;

    for (int i = 0; i < MAX_GRAINS; ++i)
    {
        if (!grains[i].active)
        {
            slot = i;
            break;
        }
        if (grains[i].phase > oldestPhase)
        {
            oldestPhase = grains[i].phase;
            oldestSlot = i;
        }
    }
    if (slot < 0) slot = oldestSlot;

    auto& g = grains[slot];
    g.active = true;
    g.sourceIndex = currentSource;
    g.shape = grainShape;
    g.sampleCount = 0;

    // Position with randomization
    float pos = position + (rng.nextFloat() * 2.0f - 1.0f) * positionRand;
    pos = juce::jlimit(0.0f, 1.0f, pos);

    auto& src = sources[currentSource];
    if (src.numSamples == 0)
    {
        g.active = false;
        return;
    }

    g.startPos = pos * (float)(src.numSamples - 1);

    // Pitch
    float pitchOffset = (float)pitchSemitones;
    if (pitchRandSemi > 0)
        pitchOffset += (rng.nextFloat() * 2.0f - 1.0f) * (float)pitchRandSemi;

    // MIDI pitch transpose relative to middle C
    pitchOffset += (float)(baseMidiNote - 60);

    g.playbackRate = std::pow(2.0f, pitchOffset / 12.0f);

    // Grain length
    g.lengthSamples = (int)(grainSizeMs * 0.001f * (float)sampleRate);
    if (g.lengthSamples < 1) g.lengthSamples = 1;
    g.phase = 0.0f;
    g.phaseInc = 1.0f / (float)g.lengthSamples;
    g.readPos = g.startPos;

    // Pan
    g.pan = (rng.nextFloat() * 2.0f - 1.0f) * panSpread;
}

float GranularEngine::getWindow(float phase, int shape)
{
    switch (shape)
    {
        case 0: // Hann
            return 0.5f * (1.0f - std::cos(2.0f * juce::MathConstants<float>::pi * phase));
        case 1: // Triangle
            return (phase < 0.5f) ? (2.0f * phase) : (2.0f * (1.0f - phase));
        case 2: // Rect
            return 1.0f;
        default:
            return 1.0f;
    }
}

float GranularEngine::readSource(int sourceIdx, float pos)
{
    auto& src = sources[sourceIdx];
    if (src.numSamples == 0) return 0.0f;

    // Wrap position
    if (pos < 0.0f) pos = 0.0f;
    if (pos >= (float)(src.numSamples - 1)) pos = (float)(src.numSamples - 2);

    int idx0 = (int)pos;
    int idx1 = idx0 + 1;
    float frac = pos - (float)idx0;

    if (idx1 >= src.numSamples) idx1 = src.numSamples - 1;
    return src.data[idx0] + frac * (src.data[idx1] - src.data[idx0]);
}

std::pair<float, float> GranularEngine::processSample()
{
    if (level <= 0.0f) return { 0.0f, 0.0f };

    // Spawn new grains based on density
    if (isActive && density > 0.0f)
    {
        triggerInterval = (float)sampleRate / density;
        triggerCounter += 1.0f;
        if (triggerCounter >= triggerInterval)
        {
            triggerCounter -= triggerInterval;
            spawnGrain();
        }
    }

    // Process active grains
    float left = 0.0f, right = 0.0f;

    for (auto& g : grains)
    {
        if (!g.active) continue;

        float window = getWindow(g.phase, g.shape);
        float sample = readSource(g.sourceIndex, g.readPos) * window;

        float panL = std::cos((g.pan + 1.0f) * 0.25f * juce::MathConstants<float>::pi);
        float panR = std::sin((g.pan + 1.0f) * 0.25f * juce::MathConstants<float>::pi);

        left += sample * panL;
        right += sample * panR;

        g.readPos += g.playbackRate;
        g.phase += g.phaseInc;
        g.sampleCount++;

        if (g.phase >= 1.0f)
            g.active = false;
    }

    return { left * level * velocityLevel, right * level * velocityLevel };
}
