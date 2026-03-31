#pragma once
#include <cmath>
#include <juce_core/juce_core.h>

class MultiFilter
{
public:
    void prepareToPlay(double sampleRate);
    void reset();

    void setType(int type); // 0=LP12, 1=LP24, 2=HP12, 3=BP, 4=Notch
    void setCutoff(float hz);
    void setResonance(float res); // 0-1
    void setDrive(float drive);

    float processSample(float input);

private:
    double sampleRate = 44100.0;
    int filterType = 0;
    float cutoffHz = 20000.0f;
    float resonance = 0.0f;
    float filterDrive = 1.0f;

    // SVF state (first stage)
    float ic1eq = 0.0f, ic2eq = 0.0f;
    // SVF state (second stage, for LP24)
    float ic1eq2 = 0.0f, ic2eq2 = 0.0f;

    // Cached coefficients
    float g = 0.0f, k = 0.0f;
    float a1 = 0.0f, a2 = 0.0f, a3 = 0.0f;

    void updateCoefficients();
};
