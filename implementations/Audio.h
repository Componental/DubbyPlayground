#pragma once
#include "daisy_core.h"

#include "daisysp.h"
#include "../Dubby.h"

using namespace daisy;
using namespace daisysp;

namespace daisy
{

float SetGains(Dubby& dubby, int channel, size_t sample, AudioHandle::InputBuffer& in, AudioHandle::OutputBuffer& out) 
{
    float _in  = dubby.GetAudioInGain((Dubby::AudioIns)channel) * in[channel][sample];
    out[channel][sample] = _in;
    out[channel][sample] = dubby.GetAudioOutGain((Dubby::AudioOuts)channel) * _in;

    return _in;
}

void CalculateRMS(Dubby& dubby, float in, float out, int channel, double sumSquared[][NUM_AUDIO_CHANNELS])
{
    sumSquared[0][channel] += in * in;
    sumSquared[1][channel] += out * out;
}

void SetRMSValues(Dubby& dubby, double sumSquared[][NUM_AUDIO_CHANNELS])
{
    for (int j = 0; j < NUM_AUDIO_CHANNELS; j++) 
    {
        dubby.currentLevels[0][j] = sqrt(sumSquared[0][j] / AUDIO_BLOCK_SIZE);
        dubby.currentLevels[1][j] = sqrt(sumSquared[1][j] / AUDIO_BLOCK_SIZE);
    }
}

} // namespace daisy
