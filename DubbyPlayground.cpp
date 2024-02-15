
#include "daisysp.h"
#include "Dubby.h"

using namespace daisy;
using namespace daisysp;

Dubby dubby;
CpuLoadMeter loadMeter;

void AudioCallback(AudioHandle::InputBuffer in, AudioHandle::OutputBuffer out, size_t size)
{
    loadMeter.OnBlockStart();

    double sumSquaredIns[4] = { 0.0f };
    double sumSquaredOuts[4] = { 0.0f };

	for (size_t i = 0; i < size; i++)
	{
        for (int j = 0; j < 4; j++) 
        {
            float _in = dubby.audioGains[0][j] * in[j][i];

            out[j][i] = dubby.audioGains[1][j] * _in;

            
            float sample = _in;
            sumSquaredIns[j] += sample * sample;

            sample = out[j][i];
            sumSquaredOuts[j] += sample * sample;

        } 


        if (dubby.scopeSelector == 0) dubby.scope_buffer[i] = (in[0][i] + in[1][i]) * .5f;   
        else if (dubby.scopeSelector == 1) dubby.scope_buffer[i] = (in[2][i] + in[3][i]) * .5f;   
        else if (dubby.scopeSelector == 2) dubby.scope_buffer[i] = (out[0][i] + out[1][i]) * .5f; 
        else if (dubby.scopeSelector == 3) dubby.scope_buffer[i] = (out[2][i] + out[3][i]) * .5f; 
        else if (dubby.scopeSelector == 4) dubby.scope_buffer[i] = in[0][i]; 
        else if (dubby.scopeSelector == 5) dubby.scope_buffer[i] = in[1][i]; 
        else if (dubby.scopeSelector == 6) dubby.scope_buffer[i] = in[2][i]; 
        else if (dubby.scopeSelector == 7) dubby.scope_buffer[i] = in[3][i]; 
        else if (dubby.scopeSelector == 8) dubby.scope_buffer[i] = out[0][i]; 
        else if (dubby.scopeSelector == 9) dubby.scope_buffer[i] = out[1][i]; 
        else if (dubby.scopeSelector == 10) dubby.scope_buffer[i] = out[2][i]; 
        else if (dubby.scopeSelector == 11) dubby.scope_buffer[i] = out[3][i]; 
	}

    for (int j = 0; j < 4; j++) 
    {
        dubby.currentLevels[0][j] = sqrt(sumSquaredIns[j] / AUDIO_BLOCK_SIZE);
        dubby.currentLevels[1][j] = sqrt(sumSquaredOuts[j] / AUDIO_BLOCK_SIZE);
    }

    loadMeter.OnBlockEnd();
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

    loadMeter.Init(dubby.seed.AudioSampleRate(), dubby.seed.AudioBlockSize());
    
    dubby.midi_uart.StartReceive();

	while(1) { 
        dubby.ProcessAllControls();
        dubby.UpdateDisplay();

        // CPU METER =====
        std::string str = std::to_string(int(loadMeter.GetAvgCpuLoad() * 100.0f)) + "%"; 
        dubby.UpdateStatusBar(&str[0], dubby.MIDDLE);
        // ===============

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
	}
}
