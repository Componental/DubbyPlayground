
#include "daisysp.h"
#include "Dubby.h"

using namespace daisy;
using namespace daisysp;

Dubby dubby;

#define kBuffSize 48000 * 60 // 60 seconds at 48kHz

// Loopers and the buffers they'll use
Looper              looper[2];
float DSY_SDRAM_BSS buffer[2][kBuffSize];
 
static int32_t pitchShiftAmount = 12;
static PitchShifter DSY_SDRAM_BSS pitchShifter[2];
static CrossFade pitchCrossfade[2];

float min1 = 0.0;  // Minimum value of the original range
float max1 = 1.0;  // Maximum value of the original range
float min2 = -9.0; // Minimum value of the target range
float max2 = 9.0;  // Maximum value of the target range

float min3 = 0; // Minimum value of the target range
float max3 = 1;  // Maximum value of the target range

// Set in and loop gain from CV_1 and CV_2 respectively
float in_level   = 0.8f;
float loop_level = 0.8f;

bool pitchBefore = false;

float outGain = 0.0f;
float knv1 = 0.0f;


void AudioCallback(AudioHandle::InputBuffer in, AudioHandle::OutputBuffer out, size_t size)
{
    double sumSquared[4] = { 0.0f };
    float shifted;

    float res;
    float in_;
    float sig;

	for (size_t i = 0; i < size; i++)
	{
        for (int j = 0; j < 2; j++) 
        {
            in_ = in[j][i] * in_level;
            res = in_;

            if (pitchBefore)
            {
                shifted = pitchShifter[j].Process(in_);

                // store signal = loop signal * loop gain + in * in_gain
                sig = looper[j].Process(shifted) * loop_level + shifted;

                // send that signal to the outputs
                res = sig;
            } 
            else 
            {
                // store signal = loop signal * loop gain + in * in_gain
                sig = looper[j].Process(in_) * loop_level + in_;

                shifted = pitchShifter[j].Process(sig);

                // send that signal to the outputs
                res = shifted;
            }


            // if (j < 2)
            // {
            //     float dryl  = res;
            //     float sendl = res * reverbSend;
            //     float wetl;
            //     reverb.Process(sendl, sendl, &wetl, &wetl);
            //     res = dryl + wetl;
            // }

            out[j][i] = res * outGain;

            float sample = out[j][i];
            sumSquared[j] += sample * sample;
        } 

        dubby.scope_buffer[i] = (out[0][i] + out[1][i]) * .5f;   
	}

    for (int j = 0; j < 4; j++) dubby.currentLevels[j] = sqrt(sumSquared[j] / AUDIO_BLOCK_SIZE);
   
}

void MIDIUartSendNoteOn(uint8_t channel, uint8_t note, uint8_t velocity) {
    uint8_t data[3] = { 0 };
    
    data[0] = (channel & 0x0F) + 0x90;  // limit channel byte, add status byte
    data[1] = note & 0x7F;              // remove MSB on data
    data[2] = velocity & 0x7F;

    dubby.midi_uart.SendMessage(data, 3);
}

void MIDIUartSendNoteOff(uint8_t channel, uint8_t note) {
    uint8_t data[3] = { 0 };

    data[0] = (channel & 0x0F) + 0x80;  // limit channel byte, add status byte
    data[1] = note & 0x7F;              // remove MSB on data
    data[2] = 0 & 0x7F;

    dubby.midi_uart.SendMessage(data, 3);
}


void HandleMidiUartMessage(MidiEvent m)
{
    switch(m.type)
    {
        case NoteOn:
        {
            NoteOnEvent p = m.AsNoteOn();
            p = m.AsNoteOn(); // p.note, p.velocity
            break;
        }
        case NoteOff:
        {
            NoteOffEvent p = m.AsNoteOff();
            break;
        }
        default: break;
    }
}


void MIDIUsbSendNoteOn(uint8_t channel, uint8_t note, uint8_t velocity) {
    uint8_t data[3] = { 0 };
    
    data[0] = (channel & 0x0F) + 0x90;  // limit channel byte, add status byte
    data[1] = note & 0x7F;              // remove MSB on data
    data[2] = velocity & 0x7F;

    dubby.midi_usb.SendMessage(data, 3);
}

void MIDIUsbSendNoteOff(uint8_t channel, uint8_t note) {
    uint8_t data[3] = { 0 };

    data[0] = (channel & 0x0F) + 0x80;  // limit channel byte, add status byte
    data[1] = note & 0x7F;              // remove MSB on data
    data[2] = 0 & 0x7F;

    dubby.midi_usb.SendMessage(data, 3);
}

void HandleMidiUsbMessage(MidiEvent m)
{
    switch(m.type)
    {
        case NoteOn:
        {
            NoteOnEvent p = m.AsNoteOn();
            p = m.AsNoteOn(); // p.note, p.velocity
            MIDIUsbSendNoteOn(0, p.note, p.velocity);
            break;
        }
        case NoteOff:
        {
            NoteOffEvent p = m.AsNoteOff();
            break;
        }
        default: break;
    }
}


int main(void)
{
	dubby.seed.Init();
	// dubby.seed.StartLog(true);

    dubby.Init();
    
	dubby.seed.SetAudioBlockSize(AUDIO_BLOCK_SIZE); // number of samples handled per callback
	dubby.seed.SetAudioSampleRate(SaiHandle::Config::SampleRate::SAI_48KHZ);
    dubby.ProcessAllControls();

    dubby.DrawLogo(); 
    System::Delay(1000);
	dubby.seed.StartAudio(AudioCallback);
    dubby.UpdateMenu(0, false);
    
    dubby.midi_uart.StartReceive();

    for (int i = 0; i < 2; i++) 
    {
        pitchShifter[i].Init(dubby.seed.AudioSampleRate());
        pitchShifter[i].SetDelSize(2400);
        pitchCrossfade[i].Init(CROSSFADE_CPOW);
    }

    // Init the loopers
    for (int i = 0; i < 2; i++) 
    {
        looper[i].Init(buffer[i], kBuffSize);
        looper[i].SetMode(Looper::Mode::FRIPPERTRONICS);
    }

	while(1) { 
        dubby.ProcessAllControls();
        dubby.UpdateDisplay();

        dubby.midi_uart.Listen();
        dubby.midi_usb.Listen();

        // Handle UART MIDI Events
        while(dubby.midi_uart.HasEvents())
        {
            HandleMidiUartMessage(dubby.midi_uart.PopEvent());
        }

        // Handle USB MIDI Events
        while(dubby.midi_usb.HasEvents())
        {
            HandleMidiUsbMessage(dubby.midi_usb.PopEvent());
        }

        knv1 = dubby.GetKnobValue(static_cast<Dubby::Ctrl>(0));

        dubby.GetKnobValue(static_cast<Dubby::Ctrl>(2));
        outGain = dubby.GetKnobValue(static_cast<Dubby::Ctrl>(3));

        for (int i = 0; i < 4; i++) 
        {
            pitchShifter[i].SetTransposition(min2 + (max2 - min2) *  (knv1 - min1) / (max1 - min1));
            // pitchShifter.SetFun((knv2 + 1) * 12.0f);
            // pitchShifter.SetDelSize((knv3 + 1) * 12.0f);
        }

        //if you press the button, toggle the record state
        if(dubby.buttons[0].RisingEdge())
        {
            for (int i = 0; i < 4; i++)
                looper[i].TrigRecord();
        }

        // if you hold the button longer than 1000 ms (1 sec), clear the loop
        if(dubby.buttons[0].TimeHeldMs() >= 1000.f)
        {
            for (int i = 0; i < 4; i++)
                looper[i].Clear();
        }

        if(dubby.buttons[2].RisingEdge())
        {
            for (int i = 0; i < 4; i++)
                looper[i].ToggleHalfSpeed();
        }

        
        if(dubby.buttons[1].RisingEdge())
        {
            pitchBefore = !pitchBefore;
        }

        if(dubby.buttons[3].RisingEdge())
        {
            for (int i = 0; i < 4; i++)
                looper[i].ToggleReverse();
        }
	}
}
