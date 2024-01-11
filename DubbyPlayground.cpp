
#include "daisysp.h"
#include "Dubby.h"

using namespace daisy;
using namespace daisysp;

Dubby dubby;

Oscillator osc;

void AudioCallback(AudioHandle::InputBuffer in, AudioHandle::OutputBuffer out, size_t size)
{
    double sumSquared[4] = { 0.0f };

	for (size_t i = 0; i < size; i++)
	{
        for (int j = 0; j < 4; j++) 
        {
            // out[j][i] = dubby.GetKnobValue(static_cast<Dubby::Ctrl>(j)) * in[j][i];
            out[j][i] = osc.Process();

            float sample = out[j][i];
            sumSquared[j] += sample * sample;
        } 

        dubby.scope_buffer[i] = (out[0][i] + out[1][i]) * .5f;   
	}

    for (int j = 0; j < 4; j++) dubby.currentLevels[j] = sqrt(sumSquared[j] / AUDIO_BLOCK_SIZE);
}

void HandleMidiMessage(MidiEvent m)
{
    switch(m.type)
    {
        case NoteOn:
        {
            NoteOnEvent p = m.AsNoteOn();
            
            p = m.AsNoteOn();
            osc.SetFreq(mtof(p.note));
            osc.SetAmp((p.velocity / 127.0f));
            //dubby.seed.PrintLine("NOTE ON %d %f", p.note, (float)p.velocity);
            break;
        }
        case NoteOff:
        {
            NoteOffEvent p = m.AsNoteOff();

            osc.SetAmp(0);
            //dubby.seed.PrintLine("NOTE OFF %d", p.note);
            break;
        }
        default: break;
    }
}


int main(void)
{
	dubby.seed.Init();
    // dubby.InitAudio();
	// dubby.seed.StartLog(true);

    dubby.Init();

    
    osc.Init(48000);
    osc.SetWaveform(Oscillator::WAVE_POLYBLEP_SAW);
    
	dubby.seed.SetAudioBlockSize(AUDIO_BLOCK_SIZE); // number of samples handled per callback
	dubby.seed.SetAudioSampleRate(SaiHandle::Config::SampleRate::SAI_48KHZ);
    dubby.ProcessAllControls();

    dubby.DrawLogo(); 
    System::Delay(1000);
	dubby.seed.StartAudio(AudioCallback);
    dubby.UpdateMenu(0, false);
    
    dubby.midi.StartReceive();

	while(1) { 
        dubby.ProcessAllControls();
        dubby.UpdateDisplay();

        dubby.midi.Listen();
        // Handle MIDI Events
        while(dubby.midi.HasEvents())
        {
            HandleMidiMessage(dubby.midi.PopEvent());
        }

        // for (int i = 0; i < 4; i++) {
        //     if (dubby.buttons[i].RisingEdge())
        //         dubby.seed.PrintLine("Button %d pressed", i+1);
        //     else if (dubby.buttons[i].FallingEdge())
        //         dubby.seed.PrintLine("Button %d released", i+1);
        // }
	}
}
