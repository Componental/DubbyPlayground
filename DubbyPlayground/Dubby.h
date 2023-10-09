
#pragma once
#include "daisy_seed.h"
#include "dev/oled_ssd130x.h"


#include "./bitmaps/bmps.h"

#define AUDIO_BLOCK_SIZE 128 

namespace daisy
{
class Dubby
{
  public:

    enum MenuItems { 
        MENU1, 
        MENU2, 
        MENU3,
        MENU_LAST // used to know the size of enum
    };
    
    const char * MenuItemsStrings[MENU_LAST] = { 
        "SCOPE", 
        "MIXER", 
        "PREFS" 
    };
    
    enum PreferenesMenuItems { 
        OPTION1, 
        OPTION2,
        OPTION3,
        OPTION4,
        PREFERENCESMENU_LAST // used to know the size of enum
    };
    
    const char * PreferencesMenuItemsStrings[PREFERENCESMENU_LAST] = { 
        "Routing", 
        "Parameters",
        "Elements",
        "DFU Mode"
    };

    enum MenuTypes {
        MAINMENU,
        PREFERENCESMENU
    };

    Dubby() {}

    ~Dubby() {}

    void Init();

    void InitDisplay();

    void UpdateDisplay();

    void DrawLogo();

    void DrawBitmap(int bitmapIndex);

    void UpdateMenu(int increment, bool higlight = true);

    void HightlightMenuItem();

    void ReleaseMenu();

    void UpdateSubmenu();

    void UpdateMixerPane();

    void UpdateBar(int i);

    void RenderScope();

    void DisplayPreferencesMenu();

    void UpdatePreferencesMenu(int increment);

    void InitEncoder();

    void ProcessAllControls();

    void ProcessAnalogControls();

    void ProcessDigitalControls();

    void InitAudio();

    const char * GetTextForEnum(MenuTypes m, int enumVal);

    void ResetToBootloader();

    DaisySeed seed; 

    MenuItems menuItemSelected = (MenuItems)0;
    
    PreferenesMenuItems preferencesMenuItemSelected = (PreferenesMenuItems)0;

    const int menuTextCursors[3][2] = { {8, 55}, {50, 55}, {92, 55} }; 
    const int menuBoxBounding[3][4] = { {0, 53, 43, 63}, {43, 53, 85, 63}, {85, 53, 127, 63} }; 
    int submenuBoxBounding[5][4];

    // IO declaration,
    Switch button1, button2, button3, button4, js_sel;
    Encoder encoder;   

    float pots[4];
    float js_v, js_h;

    float scope_buffer[AUDIO_BLOCK_SIZE] = {0.f};
    
    float currentLevels[4] = { 0.f };

    OledDisplay<SSD130x4WireSpi128x64Driver> display;

  private:
  
    int margin = 8;
    bool menuActive = false;
    uint32_t screen_update_last_, screen_update_period_;

};

}

