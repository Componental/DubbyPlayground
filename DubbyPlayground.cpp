
#include "daisysp.h"
#include "Dubby.h"
#define NUM_STRINGS 5 // Define the number of strings

using namespace daisy;
using namespace daisysp;

Dubby dubby;
CpuLoadMeter loadMeter;
int oldestPlayingVoice = 0;
bool voiceInUse[NUM_STRINGS] = {false};


StringVoice DSY_SDRAM_BSS strings[NUM_STRINGS];
float freq;
    float accent;
    float damping;
    float structure;
    float brightness;
    float outputVolume;

float freqs[NUM_STRINGS] = {}; // Array to store frequencies for each string
int notes[NUM_STRINGS] = {}; // Array to store frequencies for each string

bool triggers[NUM_STRINGS] = {}; // Array to store trigger states for each string

bool triggerString;


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



uint8_t value1_;
uint8_t value2_;
uint8_t value3_;


void AudioCallback(AudioHandle::InputBuffer in, AudioHandle::OutputBuffer out, size_t size)
{
    loadMeter.OnBlockStart();

    double sumSquaredIns[4] = { 0.0f };
    double sumSquaredOuts[4] = { 0.0f };




    for (size_t i = 0; i < size; i++)
    {
        float mix = 0.0f;
        for (int j = 0; j < NUM_STRINGS; j++)
        {
            mix += strings[j].Process(triggers[j]);
        }
        out[0][i] = out[1][i] = mix * 0.2f * outputVolume;

        // Reset all string triggers
        triggerString = false;
         for (int j = 0; j < NUM_STRINGS; j++)
        {
            triggers[j] = false;
        }
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
    switch (m.type)
    {
    case NoteOn:
    {
        NoteOnEvent p = m.AsNoteOn();
        if (p.velocity != 0)
        {
            // Find the first unused string and assign the MIDI note to it
            bool assigned = false;
            for (int i = 0; i < NUM_STRINGS; i++)
            {
                if (!voiceInUse[i])
                {   
                    notes[i] = p.note;
                    freqs[i] = mtof(p.note);
                    strings[i].SetFreq(freqs[i]);
                    strings[i].SetAccent(p.velocity / 127.f);
                    triggers[i] = true;
                    voiceInUse[i] = true; // Mark the voice as in use
                    assigned = true;
                    break;
                }
            }

            // If no unused string is found, assign the note to the oldest playing voice
            if (!assigned)
            {
                // Find the oldest playing voice and assign the MIDI note to it
                for (int i = 0; i < NUM_STRINGS; i++)
                {
                    if (i == oldestPlayingVoice)
                    {
                        freqs[i] = mtof(p.note);
                        notes[i] = p.note;
                        strings[i].SetFreq(freqs[i]);
                    strings[i].SetAccent(p.velocity / 127.f);

                        triggers[i] = true;
                        // Don't update oldestPlayingVoice here, as it's already set
                        break;
                    }
                }
            }
        }
        break;
    }
    case NoteOff:
    {
        NoteOffEvent p = m.AsNoteOff();
        // Turn off triggering for the string with matching MIDI note
        for (int i = 0; i < NUM_STRINGS; i++)
        {
            if (mtof(p.note) == freqs[i])
            {
                triggers[i] = false;
                voiceInUse[i] = false; // Mark the voice as unused
                break;
            }
        }
        break;
    }
    default:
        break;
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
      switch (m.type)
    {
    case NoteOn:
    {
        NoteOnEvent p = m.AsNoteOn();
        if (p.velocity != 0)
        {
            // Find the first unused string and assign the MIDI note to it
            bool assigned = false;
            for (int i = 0; i < NUM_STRINGS; i++)
            {
                if (!voiceInUse[i])
                {
                    freqs[i] = mtof(p.note);
                    notes[i] = p.note;
                    strings[i].SetFreq(freqs[i]);
                    strings[i].SetAccent(p.velocity / 127.f);
                    triggers[i] = true;
                    voiceInUse[i] = true; // Mark the voice as in use
                    assigned = true;
                    break;
                }
            }

            // If no unused string is found, assign the note to the oldest playing voice
            if (!assigned)
            {
                // Find the oldest playing voice and assign the MIDI note to it
                for (int i = 0; i < NUM_STRINGS; i++)
                {
                    if (i == oldestPlayingVoice)
                    {
                        freqs[i] = mtof(p.note);
                        notes[i] = p.note;
                        strings[i].SetFreq(freqs[i]);
                        triggers[i] = true;
                        // Don't update oldestPlayingVoice here, as it's already set
                        break;
                    }
                }
            }
        }
        break;
    }
    case NoteOff:
    {
        NoteOffEvent p = m.AsNoteOff();
        // Turn off triggering for the string with matching MIDI note
        for (int i = 0; i < NUM_STRINGS; i++)
        {
            if (mtof(p.note) == freqs[i])
            {
                triggers[i] = false;
                voiceInUse[i] = false; // Mark the voice as unused
                break;
            }
        }
        break;
    }
    default:
        break;
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



struct Settings {
	float p1; 
	float p2; 
        //This is necessary as this operator is used in the PersistentStorage source code
	bool operator!=(const Settings& a) const {
        return !(a.p1==p1 && a.p2==p2);
    }
};

//Persistent Storage Declaration. Using type Settings and passed the devices qspi handle
PersistentStorage<Settings> SavedSettings(dubby.seed.qspi);

bool use_preset = false;
bool trigger_save = false;
float dryWet;

int knobValue1, knobValue2, knobValue3, knobValue4;



void Load() {

	//Reference to local copy of settings stored in flash
	Settings &LocalSettings = SavedSettings.GetSettings();
	
	value1_ = LocalSettings.p1;
	value2_ = LocalSettings.p2;

	use_preset = true;
}

void Save() {

	//Reference to local copy of settings stored in flash
	Settings &LocalSettings = SavedSettings.GetSettings();

	LocalSettings.p1 = value1_;
	LocalSettings.p2 = value2_;

	trigger_save = true;
}


void handleKnobs(){
   // Get the knob values
        float knob1Value = dubby.GetKnobValue(dubby.CTRL_1); // E.G GAIN
    float knob2Value = dubby.GetKnobValue(dubby.CTRL_2) ; // E.G RESONANCE
    float knob3Value = dubby.GetKnobValue(dubby.CTRL_3); // E.G CUTOFF
    float knob4Value = dubby.GetKnobValue(dubby.CTRL_4); // E.G CUTOFF


/*
    // Map the knob value to a logarithmic scale for cutoff frequency
    float minCutoff = 5.0f; // Minimum cutoff frequency in Hz
    float maxCutoff = 7000.0f; // Maximum cutoff frequency in Hz
    float mappedCutoff = daisysp::fmap(cutOffKnobValue, minCutoff, maxCutoff, daisysp::Mapping::LOG);

*/


    float minOutput = 0.001f;
    float maxOutput = 1.f;

    outputVolume = daisysp::fmap(knob4Value, minOutput, maxOutput, Mapping::LOG);

    std::vector<float>    knobValues = {knob1Value, knob2Value, knob3Value, knob4Value};
    dubby.updateKnobValues(knobValues);


    
     damping = knob1Value*0.7f;
     structure = knob2Value;
brightness = knob3Value;
    
    

            for (int i = 0; i < NUM_STRINGS; i++)
{
freq = freqs[i];

            strings[i].SetFreq(freq);
            //strings[i].SetAccent(accent);
            strings[i].SetDamping(damping);
            strings[i].SetSustain(false);
            strings[i].SetStructure(structure);
            strings[i].SetBrightness(brightness);
}
    // Update the parameters


    
                
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
    System::Delay(200);
    float sample_rate = dubby.seed.AudioSampleRate();
     
                 for (int i = 0; i < NUM_STRINGS; i++)
{
      strings[i].Init(sample_rate);
}

	dubby.seed.StartAudio(AudioCallback);
    dubby.UpdateWindowSelector(0, false);

    loadMeter.Init(dubby.seed.AudioSampleRate(), dubby.seed.AudioBlockSize());
    
    dubby.midi_uart.StartReceive(); 

	//SavedSettings.Init();
    clockInterval = calculateClockInterval(globalBPM);

    // Initialize a timer variable
    uint32_t lastTime = System::GetUs();

    Settings DefaultSettings = {0.0f, 0.0f};
    SavedSettings.Init(DefaultSettings);



    //Load();
   // FlashLoad(1);
	while(1) { 
        dubby.ProcessAllControls();
        dubby.UpdateDisplay();


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
     
	

    //////////////////////////////////////////////////
     
       
/*
       bassDrum.SetDecay(dubby.GetKnobValue(dubby.CTRL_1));
        bassDrum.SetTone(dubby.GetKnobValue(dubby.CTRL_2));
        bassDrum.SetDirtiness(dubby.GetKnobValue(dubby.CTRL_3));
        bassDrum.SetFreq(dubby.GetKnobValue(dubby.CTRL_4)*440.f);
        bassDrum.SetFmEnvelopeAmount(1.f);
        bassDrum.SetFmEnvelopeDecay(0.5f);

*/
 

       // snareDrum.SetSustain(0.3);
        handleKnobs();

    
    //////////////////////////////////////////////////
if (dubby.buttons[2].FallingEdge())
            {
                value1_++;
                
            }

if (dubby.buttons[3].FallingEdge())
            {
                value1_--;
            }






if (dubby.buttons[0].FallingEdge())
            {
if(use_preset)
			use_preset = false;
		else 
			Load();
	}
                

if (dubby.buttons[1].FallingEdge())
            {
		Save();

            }
            

    /////////////////////////////////////////////////
    
    
    
    



    if(trigger_save) {
			
			SavedSettings.Save(); // Writing locally stored settings to the external flash
			trigger_save = false;
		}




        if(dubby.buttons[3].TimeHeldMs() > 1000){dubby.ResetToBootloader();}


    }


    
}
