#pragma once
#include "daisy_core.h"

#include "daisysp.h"
#include "../Dubby.h"

using namespace daisy;
using namespace daisysp;

namespace daisy
{

void Init(Dubby& dubby) 
{
    dubby.seed.Init();
	// // dubby.seed.StartLog(true);

    dubby.Init();
    
	dubby.seed.SetAudioBlockSize(AUDIO_BLOCK_SIZE); // number of samples handled per callback
	dubby.seed.SetAudioSampleRate(SaiHandle::Config::SampleRate::SAI_48KHZ);

    // Implement as state machine
    dubby.ProcessAllControls();

    dubby.DrawLogo(); 
    System::Delay(1000);
    dubby.UpdateWindowSelector(0, false);

    dubby.midi_uart.StartReceive(); 
    dubby.midi_usb.StartReceive(); 

    dubby.lfo1.Init(dubby.seed.AudioSampleRate());
        dubby.lfo2.Init(dubby.seed.AudioSampleRate());



}

} // namespace daisy
