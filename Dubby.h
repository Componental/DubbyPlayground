
#pragma once
#include "daisy_seed.h"
#include "dev/oled_ssd130x.h"

#include <iostream>
#include <string>

#include "ui/DubbyEncoder.h"

#include "./bitmaps/bmps.h"

#define AUDIO_BLOCK_SIZE 128 

namespace daisy
{
class Dubby
{
  public:

    enum WindowItems 
    { 
        WIN1, 
        WIN2, 
        WIN3, 
        WIN4, 
        WIN5, 
        WIN6, 
        WIN7, 
        WIN8, 
        WIN_LAST // used to know the size of enum
    };
    
    const char * WindowItemsStrings[WIN_LAST] = 
    { 
        "SCOPE", 
        "MIXER", 
        "PREFS",
        "WIN4",
        "WIN5",
        "WIN6",
        "WIN7",
        "WIN8",
    };
    
    enum PreferencesMenuItems 
    { 
        MIDI, 
        ROUTING,
        PARAMS,
        DFUMODE,
        OPTION5,
        OPTION6,
        OPTION7,
        OPTION8,
        PREFERENCESMENU_LAST // used to know the size of enum
    };
    
    const char * PreferencesMenuItemsStrings[PREFERENCESMENU_LAST] = 
    { 
        "MIDI", 
        "Routing",
        "Params",
        "DFU Mode",
        "Option5",
        "Option6",
        "Option7",
        "Option8",
    };

    enum Ctrl
    {
        CTRL_1,   // knob 1
        CTRL_2,   // knob 2
        CTRL_3,   // knob 3
        CTRL_4,   // knob 4
        CTRL_5,   // joystick horizontal
        CTRL_6,   // joystick vertical
        CTRL_LAST
    };

    enum GateInput
    {
        GATE_IN_1,  // button 1
        GATE_IN_2,  // button 2
        GATE_IN_3,  // button 3
        GATE_IN_4,  // button 4
        GATE_IN_5,  // joystick button
        GATE_IN_LAST,
    };

    enum StatusBarSide
    {
        LEFT, 
        RIGHT, 
        MIDDLE
    };

    enum ScopePages
    {
        INPUT_1_2, 
        INPUT_3_4,
        OUTPUT_1_2, 
        OUTPUT_3_4,
        INPUT_1,
        INPUT_2,
        INPUT_3,
        INPUT_4,
        OUTPUT_1,
        OUTPUT_2,
        OUTPUT_3,
        OUTPUT_4,
        SCOPE_PAGES_LAST
    };

    const char * ScopePagesStrings[SCOPE_PAGES_LAST] = 
    { 
        "in1,in2", 
        "in3,in4", 
        "out1,out2", 
        "out3,out4", 
        "in1", 
        "in2", 
        "in3", 
        "in4", 
        "out1", 
        "out2", 
        "out3", 
        "out4", 
    };


    enum MixerPages
    {
        INPUTS, 
        OUTPUTS,
        MIXER_PAGES_LAST
    };

    const char * MixerPagesStrings[MIXER_PAGES_LAST] = 
    { 
        "inputs", 
        "outputs",
    };


    enum PreferencesMidiMenuItems 
    { 
        MIDIIN, 
        MIDIOUT,
        MIDITHRU,
        MIDIWHATEV,
        MIDIWHATEVA,
        PREFERENCESMIDIMENU_LAST // used to know the size of enum
    };
    
    const char * PreferencesMidiMenuItemsStrings[PREFERENCESMENU_LAST] = 
    { 
        "MIDI In", 
        "MIDI Out", 
        "MIDI Thru", 
        "MIDI Whatev", 
        "MIDI Whateva", 
    };    
    
    enum PreferencesRoutingMenuItems 
    { 
        ROUTING1, 
        ROUTING2, 
        ROUTING3, 
        ROUTING4, 
        ROUTING5, 
        PREFERENCESROUTINGMENU_LAST // used to know the size of enum
    };
    
    const char * PreferencesRoutingMenuItemsStrings[PREFERENCESMENU_LAST] = 
    { 
        "Routing 1", 
        "Routing 2", 
        "Routing 3", 
        "Routing 4", 
        "Routing 5", 
    };

    enum EnumTypes 
    {
        PREFERENCESMIDIMENULIST,
        PREFERENCESROUTINGMENULIST,
        PREFERENCESMENU,
        WINDOWS,
        SCOPE,
        MIXERPAGES,
    };


    Dubby() {}

    ~Dubby() {}

    void Init();

    void UpdateDisplay();

    void DrawLogo();

    void DrawBitmap(int bitmapIndex);

    void UpdateWindowSelector(int increment, bool higlight = true);

    void HighlightWindowItem();

    void ReleaseWindowSelector();

    void UpdateWindowList();
    
    void UpdateMixerPane();

    void UpdateBar(int i);

    void UpdateRenderPane();

    void RenderScope();

    void DisplayPreferencesMenuList(int increment);

    void UpdatePreferencesMenuList(int increment);

    void DisplayPreferencesSubMenuList(int increment, PreferencesMenuItems preferencesMenuItemSelected);

    void UpdatePreferencesSubMenuList(int increment, PreferencesMenuItems preferencesMenuItemSelected);

    void ProcessAllControls();

    void ProcessAnalogControls();

    void ProcessDigitalControls();
    
    float GetKnobValue(Ctrl k);

    const char * GetTextForEnum(EnumTypes m, int enumVal);

    void ResetToBootloader();

    void SwitchMIDIOutThru(bool state);

    void ClearPane();

    void UpdateStatusBar(char* text, StatusBarSide side); // side = 0 => left, side = 1 => right

    DaisySeed seed; 

    WindowItems windowItemSelected = (WindowItems)0;
    
    PreferencesMenuItems preferencesMenuItemSelected = (PreferencesMenuItems)0;
    PreferencesMidiMenuItems preferencesMidiMenuItemSelected = (PreferencesMidiMenuItems)0;
    PreferencesRoutingMenuItems preferencesRoutingMenuItemSelected = (PreferencesRoutingMenuItems)0;
    int subMenuSelector = 0;
    
    bool isSubMenuActive = false;

    // const int menuTextCursors[3][2] = { {8, 55}, {50, 55}, {92, 55} }; OLD
    const int windowTextCursors[3][2] = { {3, 55}, {46, 55}, {88, 55} };  
    const int windowBoxBounding[3][4] = { {0, 53, 43, 61}, {43, 53, 85, 61}, {85, 53, 127, 61} }; 
    int menuListBoxBounding[5][4];

    int scrollbarWidth = 0;
    int barSelector = 0;
    bool isBarSelected = false;

    int scopeSelector = INPUT_1_2;

    DubbyEncoder encoder;   
    AnalogControl analogInputs[CTRL_LAST];
    GateIn gateInputs[GATE_IN_LAST];  
    Switch buttons[4];

    float scope_buffer[AUDIO_BLOCK_SIZE] = {0.f};
    
    float currentLevels[2][4] = { { 0.f }, { 0.f } }; // 0 => INPUTS, 1 => OUTPUTS 

    OledDisplay<SSD130x4WireSpi128x64Driver> display;

    MidiUartHandler midi_uart;
    dsy_gpio midi_sw_output;

    int mixerPageSelected = INPUTS;

    float audioGains[2][4] = { { 0.8f, 0.8f, 0.8f, 0.8f}, { 0.8f, 0.8f, 0.8f, 0.8f} }; // 0 => INPUTS, 1 => OUTPUTS 

    MidiUsbHandler midi_usb;

  private:

    void InitAudio();
    void InitControls();
    void InitEncoder();
    void InitDisplay();
    void InitButtons();
    void InitMidi();

    int margin = 8;
    bool windowSelectorActive = false;
    uint32_t screen_update_last_, screen_update_period_;

    bool isEncoderPressed = false;
    bool wasEncoderLongPressed = false;
    unsigned long encoderPressStartTime = 0;
};

}
