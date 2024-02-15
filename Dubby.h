
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

    enum MenuItems 
    { 
        MENU1, 
        MENU2, 
        MENU3,
        MENU4,
        MENU5,
        MENU6,
        MENU7,
        MENU8,
        MENU_LAST // used to know the size of enum
    };
    
    const char * MenuItemsStrings[MENU_LAST] = 
    { 
        "SCOPE", 
        "MIXER", 
        "PREFS",
        "MENU4",
        "MENU5",  
        "MENU6",
        "MENU7",
        "MENU8",

    };
    
    enum PreferenesMenuItems 
    { 
        OPTION1, 
        OPTION2,
        OPTION3,
        OPTION4,
        OPTION5,
        OPTION6,
        OPTION67,
        OPTION68,
        PREFERENCESMENU_LAST // used to know the size of enum
    };
    
    const char * PreferencesMenuItemsStrings[PREFERENCESMENU_LAST] = 
    { 
        "Routing", 
        "Params",
        "Elements",
        "DFU Mode",
        "DFU1",
        "DFU2",
        "DFU3",
        "DFU4",
    };

    enum MenuTypes 
    {
        MAINMENU,
        SCOPE,
        PREFERENCESMENU,
        MIXERPAGES,
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
        RIGHT
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

    Dubby() {}

    ~Dubby() {}

    void Init();

    void UpdateDisplay();

    void DrawLogo();

    void DrawBitmap(int bitmapIndex);

    void UpdateMenu(int increment, bool higlight = true);

    void HighlightMenuItem();

    void ReleaseMenu();

    void UpdateSubmenu();
    
    void UpdateMixerPane();

    void UpdateBar(int i);

    void UpdateRenderPane();

    void RenderScope();

    void DisplayPreferencesMenu(int increment);

    void UpdatePreferencesMenu(int increment);

    void ProcessAllControls();

    void ProcessAnalogControls();

    void ProcessDigitalControls();
    
    float GetKnobValue(Ctrl k);

    const char * GetTextForEnum(MenuTypes m, int enumVal);

    void ResetToBootloader();

    void SwitchMIDIOutThru(bool state);

    void ClearPane();

    void UpdateStatusBar(char* text, StatusBarSide side); // side = 0 => left, side = 1 => right

    DaisySeed seed; 

    MenuItems menuItemSelected = (MenuItems)0;
    
    PreferenesMenuItems preferencesMenuItemSelected = (PreferenesMenuItems)0;

    // const int menuTextCursors[3][2] = { {8, 55}, {50, 55}, {92, 55} }; OLD
    const int menuTextCursors[3][2] = { {3, 55}, {46, 55}, {88, 55} };  
    const int menuBoxBounding[3][4] = { {0, 53, 43, 61}, {43, 53, 85, 61}, {85, 53, 127, 61} }; 
    int submenuBoxBounding[5][4];

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
    bool menuActive = false;
    uint32_t screen_update_last_, screen_update_period_;

};

}

