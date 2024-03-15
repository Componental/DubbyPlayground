#pragma once
#include "daisy_core.h"

#include "daisysp.h"
#include "../Dubby.h"

using namespace daisy;
using namespace daisysp;


// BPM range: 30 - 300
#define TTEMPO_MIN 30
#define TTEMPO_MAX 300


namespace daisy
{

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
void sendTimingClock(Dubby& dubby) {
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
                // Dubby::getInstance().UpdateStatusBar(&stra[0], Dubby::getInstance().MIDDLE);

                prev_ms = ms;
                tt_count = 0;
            }
            break;
    }
}


void MidiMonitor(Dubby& dubby) 
{
    // This should be handled in monitor class
    dubby.midi_uart.Listen();
    dubby.midi_usb.Listen();

    // Handle UART MIDI Events
    while(dubby.midi_uart.HasEvents())
    {
        MidiEvent m = dubby.midi_uart.PopEvent();
        if(m.type == SystemRealTime)
            HandleSystemRealTime(m.srt_type);
    }

    // Handle USB MIDI Events
    while(dubby.midi_usb.HasEvents())
    {
        MidiEvent m = dubby.midi_usb.PopEvent();
        if(m.type == SystemRealTime)
            HandleSystemRealTime(m.srt_type);
    }

    dubby.globalBPM = TTEMPO_MIN + ((TTEMPO_MAX - TTEMPO_MIN) / (1 - 0)) * (dubby.GetKnobValue(dubby.CTRL_1));
    uint32_t currentTime = System::GetUs();

    clockInterval = calculateClockInterval(dubby.globalBPM);

    // Check if it's time to send the timing clock
    if (currentTime - lastTime >= clockInterval) {
        // Send timing clock
        sendTimingClock(dubby);

        // Update the last time
        lastTime = currentTime;
    }
}

} // namespace daisy
