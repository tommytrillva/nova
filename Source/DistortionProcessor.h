#pragma once
#include <cmath>
#include <algorithm>
#include <juce_core/juce_core.h>

class DistortionProcessor
{
public:
    void prepareToPlay(double sampleRate);

    void setMode(int mode) { this->mode = mode; } // 0=SoftClip,1=HardClip,2=Wavefold,3=Bitcrush,4=Rectify
    void setDrive(float drive) { this->drive = drive; }
    void setMix(float mix) { this->mix = mix; }

    float processSample(float input);

private:
    double sampleRate = 44100.0;
    int mode = 0;
    float drive = 1.0f;
    float mix = 0.0f;

    // DC blocker state
    float dcX1 = 0.0f, dcY1 = 0.0f;
    static constexpr float dcCoeff = 0.9997f;

    // 2x oversampling state
    float prevInput = 0.0f;
    float prevOutput = 0.0f;

    float applyDistortion(float input);
    float dcBlock(float input);
};
