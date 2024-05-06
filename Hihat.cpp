#include "Hihat.h"


void Hihat::Init(float sample_rate)
{
    sample_rate_  = sample_rate;
    
    noise.Init();
    bpFilter.Init(sample_rate_);
    env.Init(sample_rate_);

    env.SetTime(ADENV_SEG_ATTACK, 0.01);
    env.SetTime(ADENV_SEG_DECAY, 0.35);
    env.SetMin(0.0);
    env.SetMax(0.25);
    env.SetCurve(0); // linear

    bpFilter.SetFreq(1000.0f);
    bpFilter.SetRes(0.5f);
    
}

float Hihat::Process()
{
    
    float envOut = env.Process();
    noise.SetAmp(envOut);
    float noiseOut = noise.Process();

    bpFilter.Process(noiseOut);
    float filteredNoise = bpFilter.Band();
    return filteredNoise;
}


void Hihat::SetFreq(float freq)
{

     // Map the knob value to a logarithmic scale for cutoff frequency
    float minFreq = 100.f; // Minimum cutoff frequency in Hz
    float maxFreq = 4000.0f; // Maximum cutoff frequency in Hz
    float mappedFreq = daisysp::fmap(freq, minFreq, maxFreq, daisysp::Mapping::LOG);



    bpFilter.SetFreq(mappedFreq);
}

void Hihat::SetAttack(float attack)
{
    float attackValueAdjusted = 0.5f * attack + 0.01f;
        env.SetTime(ADENV_SEG_ATTACK, attackValueAdjusted);

}

void Hihat::SetDecay(float decay)
{
        float decayValueAdjusted = 0.99f * decay + 0.01f;

        env.SetTime(ADENV_SEG_DECAY, decayValueAdjusted);

}

void Hihat::SetFilterRes(float res)
{
    bpFilter.SetRes(0.9f*res);
}


void Hihat::Trigger()
{
    env.Trigger();
}


