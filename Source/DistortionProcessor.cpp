#include "DistortionProcessor.h"

void DistortionProcessor::prepareToPlay(double sr)
{
    sampleRate = sr;
    dcX1 = dcY1 = 0.0f;
    prevInput = prevOutput = 0.0f;
}

float DistortionProcessor::applyDistortion(float input)
{
    float x = input * drive;

    switch (mode)
    {
        case 0: // Soft Clip (tanh)
            return std::tanh(x);

        case 1: // Hard Clip
            return std::clamp(x, -1.0f, 1.0f);

        case 2: // Wavefold
        {
            float folded = std::fmod(x + 1.0f, 4.0f);
            if (folded < 0.0f) folded += 4.0f;
            return std::abs(folded - 2.0f) - 1.0f;
        }

        case 3: // Bitcrush
        {
            float bits = juce::jlimit(1.0f, 16.0f, 16.0f - (drive - 1.0f) * 0.3f);
            float levels = std::pow(2.0f, bits);
            return std::round(input * levels) / levels;
        }

        case 4: // Rectify
        {
            float rect = std::abs(x) * 2.0f - 1.0f;
            return std::clamp(rect, -1.0f, 1.0f);
        }

        default:
            return input;
    }
}

float DistortionProcessor::dcBlock(float input)
{
    float output = input - dcX1 + dcCoeff * dcY1;
    dcX1 = input;
    dcY1 = output;
    return output;
}

float DistortionProcessor::processSample(float input)
{
    if (mix <= 0.0f) return input;

    // 2x oversampling: process at midpoint and at sample
    float mid = (prevInput + input) * 0.5f;
    float dist1 = applyDistortion(mid);
    float dist2 = applyDistortion(input);

    // Average the two oversampled outputs (simple linear downsampling)
    float wet = (dist1 + dist2) * 0.5f;
    prevInput = input;

    // DC offset removal
    wet = dcBlock(wet);

    return input * (1.0f - mix) + wet * mix;
}
