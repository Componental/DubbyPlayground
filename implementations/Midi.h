#pragma once
#include "daisy_core.h"

#include "daisysp.h"
#include "../Dubby.h"

using namespace daisy;
using namespace daisysp;


// BPM range: 30 - 300
#define TTEMPO_MIN 30
#define TTEMPO_MAX 400


namespace daisy
{

TimerHandle midiClockTimer;
bool clockRunning = false;

uint16_t tt_count = 0;
uint32_t prev_ms = 0;

uint32_t clockInterval;

// Initialize a timer variable
uint32_t lastTime; // = System::GetUs();


void MIDIUartSendNoteOn(Dubby& dubby, uint8_t channel, uint8_t note, uint8_t velocity) {
    uint8_t data[3] = { 0 };
    
    data[0] = (channel & 0x0F) + 0x90;  // limit channel byte, add status byte
    data[1] = note & 0x7F;              // remove MSB on data
    data[2] = velocity & 0x7F;

    dubby.midi_uart.SendMessage(data, 3);
}

void MIDIUartSendNoteOff(Dubby& dubby, uint8_t channel, uint8_t note) {
    uint8_t data[3] = { 0 };

    data[0] = (channel & 0x0F) + 0x80;  // limit channel byte, add status byte
    data[1] = note & 0x7F;              // remove MSB on data
    data[2] = 0 & 0x7F;

    dubby.midi_uart.SendMessage(data, 3);
}



void MIDIUsbSendNoteOn(Dubby& dubby, uint8_t channel, uint8_t note, uint8_t velocity) {
    uint8_t data[3] = { 0 };
    
    data[0] = (channel & 0x0F) + 0x90;  // limit channel byte, add status byte
    data[1] = note & 0x7F;              // remove MSB on data
    data[2] = velocity & 0x7F;

    dubby.midi_usb.SendMessage(data, 3);
}

void MIDIUsbSendNoteOff(Dubby& dubby, uint8_t channel, uint8_t note) {
    uint8_t data[3] = { 0 };

    data[0] = (channel & 0x0F) + 0x80;  // limit channel byte, add status byte
    data[1] = note & 0x7F;              // remove MSB on data
    data[2] = 0 & 0x7F;

    dubby.midi_usb.SendMessage(data, 3);
}

void MIDISendNoteOn(Dubby& dubby, uint8_t channel, uint8_t note, uint8_t velocity) 
{
    MIDIUsbSendNoteOn(dubby, channel, note, velocity);
    MIDIUartSendNoteOn(dubby, channel, note, velocity);
}

void MIDISendNoteOff(Dubby& dubby, uint8_t channel, uint8_t note) 
{
    MIDIUsbSendNoteOff(dubby, channel, note);
    MIDIUartSendNoteOff(dubby, channel, note);
}

int convertRange(int value, int oldMin, int oldMax, int newMin, int newMax) {
    // First, scale the value from the old range to a value between 0 and 1
    double scaledValue = static_cast<double>(value - oldMin) / (oldMax - oldMin);
    
    // Then, scale this value to the new range
    int newValue = newMin + static_cast<int>(scaledValue * (newMax - newMin));
    
    return newValue;
}

uint32_t ms_to_bpm(uint32_t ms)
{
    return 60000 / ms;
}

void HandleSystemRealTime(uint8_t srt_type)
{
    switch(srt_type)
    {
        // 0xFA - start
        case Start: 
            clockRunning = true;
            break;

        // 0xFC - stop
        case Stop: 
            clockRunning = false;
            break;   

        // MIDI Clock -  24 clicks per quarter note
        case TimingClock:
            tt_count++;
            if(tt_count == 24)
            {
                uint32_t ms   = System::GetNow();
                uint32_t diff = ms - prev_ms;
                uint32_t bpm  = ms_to_bpm(diff);

                // std::string stra = std::to_string(bpm);
                // dubby.UpdateStatusBar(&stra[0], dubby.MIDDLE, 127);

                prev_ms = ms;
                tt_count = 0;
            }
            break;    }
}

// Function to send MIDI Timing Clock
void sendTimingClock(Dubby& dubby) {
    // MIDI timing clock message byte
    uint8_t timingClockByte[1] = { 0xF8 };

    dubby.midi_usb.SendMessage(timingClockByte, 1); // Send the single byte
}

void MidiClockSend(Dubby& dubby) 
{
    uint32_t currentTime = System::GetUs();
    // Check if it's time to send the timing clock
    if (currentTime - lastTime >= ((60000000.0f / dubby.globalBPM) / 24.0f)) {
        // Update the last time
        lastTime = currentTime;
        // Send timing clock
        sendTimingClock(dubby);
    }
}

void MIDICallback(void *dubby) 
{
    MidiClockSend(*(Dubby*)dubby);
}

void InitMidiClock(Dubby& dubby) 
{
    TimerHandle::Config config = TimerHandle::Config();
	config.periph = TimerHandle::Config::Peripheral::TIM_5;
    config.period = 0x1FFF;
	config.dir = TimerHandle::Config::CounterDir::UP;
	config.enable_irq = true;

    midiClockTimer.Init(config);
	midiClockTimer.SetCallback(MIDICallback, &dubby);
    midiClockTimer.Start();
}


} // namespace daisy
