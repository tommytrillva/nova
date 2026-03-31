#include "MultiFilter.h"

void MultiFilter::prepareToPlay(double sr)
{
    sampleRate = sr;
    reset();
}

void MultiFilter::reset()
{
    ic1eq = ic2eq = 0.0f;
    ic1eq2 = ic2eq2 = 0.0f;
}

void MultiFilter::setType(int type) { filterType = type; updateCoefficients(); }
void MultiFilter::setCutoff(float hz)
{
    cutoffHz = juce::jlimit(20.0f, (float)(sampleRate * 0.49), hz);
    updateCoefficients();
}
void MultiFilter::setResonance(float res) { resonance = res; updateCoefficients(); }
void MultiFilter::setDrive(float drv) { filterDrive = drv; }

void MultiFilter::updateCoefficients()
{
    g = std::tan(juce::MathConstants<float>::pi * cutoffHz / (float)sampleRate);
    k = 2.0f - 2.0f * resonance; // resonance 0->k=2 (no res), resonance 1->k=0 (max res)
    a1 = 1.0f / (1.0f + g * (g + k));
    a2 = g * a1;
    a3 = g * a2;
}

float MultiFilter::processSample(float input)
{
    // Apply filter drive (soft clip on input)
    if (filterDrive > 1.0f)
        input = std::tanh(input * filterDrive) / std::tanh(filterDrive);

    // First SVF stage
    float v3 = input - ic2eq;
    float v1 = a1 * ic1eq + a2 * v3;
    float v2 = ic2eq + a2 * ic1eq + a3 * v3;
    ic1eq = 2.0f * v1 - ic1eq;
    ic2eq = 2.0f * v2 - ic2eq;

    float lp = v2;
    float bp = v1;
    float hp = input - k * v1 - v2;
    float notch = hp + lp;

    float output = 0.0f;
    switch (filterType)
    {
        case 0: // LP 12dB
            output = lp;
            break;

        case 1: // LP 24dB - cascade second SVF
        {
            float v3b = lp - ic2eq2;
            float v1b = a1 * ic1eq2 + a2 * v3b;
            float v2b = ic2eq2 + a2 * ic1eq2 + a3 * v3b;
            ic1eq2 = 2.0f * v1b - ic1eq2;
            ic2eq2 = 2.0f * v2b - ic2eq2;
            output = v2b;
            break;
        }

        case 2: // HP 12dB
            output = hp;
            break;

        case 3: // BP
            output = bp;
            break;

        case 4: // Notch
            output = notch;
            break;

        default:
            output = input;
            break;
    }

    return output;
}
