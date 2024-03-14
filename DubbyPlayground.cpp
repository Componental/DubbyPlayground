
#include "daisysp.h"
#include "Dubby.h"

using namespace daisy;
using namespace daisysp;

Dubby dubby;
CpuLoadMeter loadMeter;


// BPM range: 30 - 300
#define TTEMPO_MIN 30
#define TTEMPO_MAX 300

uint32_t prev_ms = 0;
uint16_t tt_count = 0;
uint8_t clockCounter = 0;

int globalBPM = 120;
int systemRunning = false;

TimerHandle timer;

uint32_t timer_freq = 0;
uint32_t clockInterval;


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

        switch (dubby.scopeSelector)
        {
            case 0: dubby.scope_buffer[i] = (in[0][i] + in[1][i]) * .5f; break;
            case 1: dubby.scope_buffer[i] = (in[0][i] + in[1][i]) * .5f; break;
            case 2: dubby.scope_buffer[i] = (out[0][i] + out[1][i]) * .5f; break;
            case 3: dubby.scope_buffer[i] = (out[2][i] + out[3][i]) * .5f; break;
            case 4: dubby.scope_buffer[i] = in[0][i]; break;
            case 5: dubby.scope_buffer[i] = in[1][i]; break;
            case 6: dubby.scope_buffer[i] = in[2][i]; break;
            case 7: dubby.scope_buffer[i] = in[3][i]; break;
            case 8: dubby.scope_buffer[i] = out[0][i]; break;
            case 9: dubby.scope_buffer[i] = out[1][i]; break;
            case 10: dubby.scope_buffer[i] = out[2][i]; break;
            case 11: dubby.scope_buffer[i] = out[3][i]; break;
            default: dubby.scope_buffer[i] = (in[0][i] + in[1][i]) * .5f; break;
        }
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

float bpm_to_freq(uint32_t tempo)
{
    return tempo / 60.0f;
}

uint32_t ms_to_bpm(uint32_t ms)
{
    return 60000 / ms;
}


// Function to calculate the timing clock interval based on BPM
uint32_t calculateClockInterval(float bpm) {
    // Calculate the interval in microseconds between each timing clock message
    // Note: 24 PPQN corresponds to 24 clock messages per quarter note
    //bpm = TTEMPO_MIN + (bpm - 30) * (TTEMPO_MAX - TTEMPO_MIN) / (255 - 30);    
    bpm = bpm * 0.8f;

    float usPerBeat = 60000000.0f / bpm; // Convert BPM to microseconds per beat
    return static_cast<uint32_t>(usPerBeat / 24.0f); // Divide by 24 for 24 PPQN
}
// Function to send MIDI Timing Clock
void sendTimingClock() {
    // MIDI timing clock message byte
    uint8_t timingClockByte[1] = { 0xF8 };

    dubby.midi_usb.SendMessage(timingClockByte, 1); // Send the single byte
}

void HandleSystemRealTime(uint8_t srt_type)
{
    switch(srt_type)
    {
        // 0xFA - start
        case Start: 
            systemRunning = true;
            break;

        // 0xFC - stop
        case Stop: 
            systemRunning = false;
            break;

        // MIDI Clock -  24 clicks per quarter note
        case TimingClock:
            tt_count++;
            if(tt_count == 24)
            {
                uint32_t ms   = System::GetNow();
                uint32_t diff = ms - prev_ms;
                uint32_t bpm  = ms_to_bpm(diff);

                // globalBPM = bpm;
                // std::string stra = std::to_string(bpm);
                // dubby.UpdateStatusBar(&stra[0], dubby.MIDDLE);

                prev_ms = ms;
                tt_count = 0;
            }
            break;
    }
}


int main(void)
{
	dubby.seed.Init();
	// dubby.seed.StartLog(true);

    dubby.Init();
    
	dubby.seed.SetAudioBlockSize(AUDIO_BLOCK_SIZE); // number of samples handled per callback
	dubby.seed.SetAudioSampleRate(SaiHandle::Config::SampleRate::SAI_48KHZ);

    // Implement as state machine
    dubby.ProcessAllControls();

    dubby.DrawLogo(); 
    System::Delay(1000);
	dubby.seed.StartAudio(AudioCallback);
    dubby.UpdateWindowSelector(0, false);

    loadMeter.Init(dubby.seed.AudioSampleRate(), dubby.seed.AudioBlockSize());
    
    dubby.midi_uart.StartReceive(); 
    
    clockInterval = calculateClockInterval(globalBPM);

    // Initialize a timer variable
    uint32_t lastTime = System::GetUs();

	while(1) { 
        dubby.ProcessAllControls();
        dubby.UpdateDisplay();

        // CPU METER =====
        // std::string str = std::to_string(int(loadMeter.GetAvgCpuLoad() * 100.0f)) + "%"; 
        // dubby.UpdateStatusBar(&str[0], dubby.MIDDLE);
        // ===============


        // This should be handled in monitor class

        dubby.midi_uart.Listen();
        dubby.midi_usb.Listen();

        // Handle UART MIDI Events
        while(dubby.midi_uart.HasEvents())
        {
            MidiEvent m = dubby.midi_uart.PopEvent();
            HandleMidiUartMessage(m);
            if(m.type == SystemRealTime)
                HandleSystemRealTime(m.srt_type);

            // add handler

        }

        // Handle USB MIDI Events
        while(dubby.midi_usb.HasEvents())
        {
            MidiEvent m = dubby.midi_usb.PopEvent();
            HandleMidiUsbMessage(m);
            if(m.type == SystemRealTime)
                HandleSystemRealTime(m.srt_type);
        }

        globalBPM = TTEMPO_MIN + ((TTEMPO_MAX - TTEMPO_MIN) / (1 - 0)) * (dubby.GetKnobValue(dubby.CTRL_1));
        uint32_t currentTime = System::GetUs();
        
        clockInterval = calculateClockInterval(globalBPM);

        // Check if it's time to send the timing clock
        if (currentTime - lastTime >= clockInterval) {
            // Send timing clock
            sendTimingClock();

            // Update the last time
            lastTime = currentTime;
        }
     
	}
}
