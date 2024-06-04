#pragma once

#include "daisysp.h"

class SynthVoice {
public:
    SynthVoice();

    void Init(int sample_rate);
    void TriggerEnv(bool gate);
    float Process();
    
    void SetOsc1Tune (float tune);
    void SetOsc2Tune (float tune);
    void SetFreq(float freq);
    float GetFreq();

    void SetOsc1Shape(float shape);
    void SetOsc2Shape(float shape);
    void SetFilterDrive(float filterDrive);
    void SetFilterRes(float filterRes);
    void SetFilterCutoff(float filterCutoff, float filterEnvAmp);
    void SetFilterADSR(float attack, float decay, float sustain, float release);
    void SetAmpADSR(float attack, float decay, float sustain, float release);

private:
    daisysp::Oscillator osc1, osc2;
    float oscFreq; // Store the current frequency
    float osc2Tune;
    daisysp::LadderFilter filter;
    daisysp::Adsr filterEnv, ampEnv;
    float filterEnvOut, ampEnvOut;

    
};

