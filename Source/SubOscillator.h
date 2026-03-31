#pragma once
#include <cmath>
#include <juce_core/juce_core.h>

class SubOscillator
{
public:
    void prepareToPlay(double sampleRate) { this->sampleRate = sampleRate; }

    void setShape(int shape) { this->shape = shape; } // 0=Sine, 1=Triangle
    void setOctave(int oct) { octaveShift = (oct == 0) ? -1 : -2; }
    void setLevel(float lvl) { level = lvl; }

    void setFrequency(float freq)
    {
        float subFreq = freq * std::pow(2.0f, (float)octaveShift);
        phaseInc = (double)subFreq / sampleRate;
    }

    void noteOn() { phase = 0.0; }

    float processSample()
    {
        if (level <= 0.0f) return 0.0f;

        float out = 0.0f;
        float t = (float)phase;

        if (shape == 0) // Sine
            out = std::sin(2.0f * juce::MathConstants<float>::pi * t);
        else // Triangle
        {
            if (t < 0.5f)
                out = 4.0f * t - 1.0f;
            else
                out = 3.0f - 4.0f * t;
        }

        phase += phaseInc;
        if (phase >= 1.0) phase -= 1.0;

        return out * level;
    }

private:
    double sampleRate = 44100.0;
    double phase = 0.0;
    double phaseInc = 0.0;
    int shape = 0;
    int octaveShift = -1;
    float level = 0.0f;
};
