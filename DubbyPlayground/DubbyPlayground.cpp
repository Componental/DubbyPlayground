
#include "daisysp.h"
#include "Dubby.h"

using namespace daisy;
using namespace daisysp;

Dubby dubby;

void AudioCallback(AudioHandle::InputBuffer in, AudioHandle::OutputBuffer out, size_t size)
{
    double sumSquared[4] = { 0.0f };

	for (size_t i = 0; i < size; i++)
	{
        for (int j = 0; j < 4; j++) 
        {
            out[j][i] = abs(dubby.pots[j] - 1.0f) * in[j][i];

            float sample = out[j][i];
            sumSquared[j] += sample * sample;
        } 

        dubby.scope_buffer[i] = (out[0][i] + out[1][i]) * .5f;   
	}

    for (int j = 0; j < 4; j++) dubby.currentLevels[j] = sqrt(sumSquared[j] / AUDIO_BLOCK_SIZE);
}

int main(void)
{
	dubby.seed.Init();
    dubby.InitAudio();
	dubby.seed.SetAudioBlockSize(AUDIO_BLOCK_SIZE); // number of samples handled per callback
	dubby.seed.SetAudioSampleRate(SaiHandle::Config::SampleRate::SAI_48KHZ);
	// dubby.seed.StartLog(true);

    dubby.Init();
    dubby.ProcessAllControls();

    dubby.DrawLogo(); 
    System::Delay(1000);
	dubby.seed.StartAudio(AudioCallback);
    dubby.UpdateMenu(0, false);

	while(1) { 
        dubby.ProcessAllControls();
        dubby.UpdateDisplay();
	}
}
