#include "Dubby.h"

using namespace daisy;

// Hardware Definitions
#define PIN_GATE_IN_1 32
#define PIN_GATE_IN_2 23
#define PIN_GATE_IN_3 17
#define PIN_GATE_IN_4 19
#define PIN_KNOB_1 18
#define PIN_KNOB_2 15
#define PIN_KNOB_3 21
#define PIN_KNOB_4 22
#define PIN_JS_CLICK 3
#define PIN_JS_V 20
#define PIN_JS_H 16
#define PIN_ENC_CLICK 4
#define PIN_ENC_A 6
#define PIN_ENC_B 5 
#define PIN_OLED_DC 9
#define PIN_OLED_RESET 31
#define PIN_MIDI_OUT 13
#define PIN_MIDI_IN 14
#define PIN_MIDI_SWITCH 1

#define OLED_WIDTH 128
#define OLED_HEIGHT 64

#define PANE_X_START 1
#define PANE_X_END 126
#define PANE_Y_START 10
#define PANE_Y_END 52

#define STATUSBAR_X_START 1
#define STATUSBAR_X_END 126
#define STATUSBAR_Y_START 1
#define STATUSBAR_Y_END 11

#define MENULIST_X_START 0
#define MENULIST_X_END 63
#define MENULIST_Y_START 11
#define MENULIST_Y_END 21
#define MENULIST_SPACING 10
#define MENULIST_SUBMENU_SPACING 63

#define ENCODER_LONGPRESS_THRESHOLD 300

void Dubby::Init() 
{
    InitControls();
    InitButtons();
    
    screen_update_period_ = 17; // roughly 60Hz
    screen_update_last_   = seed.system.GetNow();

    for (int i = 0; i < 5; i++) 
    {
        menuListBoxBounding[i][0] = MENULIST_X_START;
        menuListBoxBounding[i][1] = MENULIST_Y_START + i * MENULIST_SPACING;
        menuListBoxBounding[i][2] = MENULIST_X_END;
        menuListBoxBounding[i][3] = MENULIST_Y_END + i * MENULIST_SPACING;
    }

    scrollbarWidth = int(128 / WIN_LAST);

    InitDisplay();
    InitEncoder();
    InitAudio();
    InitMidi();
}

void Dubby::InitControls()
{
    AdcChannelConfig cfg[CTRL_LAST];

    // Init ADC channels with Pins
    cfg[CTRL_1].InitSingle(seed.GetPin(PIN_KNOB_1));
    cfg[CTRL_2].InitSingle(seed.GetPin(PIN_KNOB_2));
    cfg[CTRL_3].InitSingle(seed.GetPin(PIN_KNOB_3));
    cfg[CTRL_4].InitSingle(seed.GetPin(PIN_KNOB_4));
    cfg[CTRL_5].InitSingle(seed.GetPin(PIN_JS_H));
    cfg[CTRL_6].InitSingle(seed.GetPin(PIN_JS_V));

    // Initialize ADC
    seed.adc.Init(cfg, CTRL_LAST);

    // Initialize analogInputs, with flip set to true
    for(size_t i = 0; i < CTRL_LAST; i++)
    {
        analogInputs[i].Init(seed.adc.GetPtr(i), seed.AudioCallbackRate(), true);
    }

    seed.adc.Start();
}

void Dubby::InitButtons()
{
    //Set button to pins, to be updated at a 1kHz  samplerate
    buttons[0].Init(seed.GetPin(PIN_GATE_IN_1), 1000);
    buttons[1].Init(seed.GetPin(PIN_GATE_IN_2), 1000);
    buttons[2].Init(seed.GetPin(PIN_GATE_IN_3), 1000);
    buttons[3].Init(seed.GetPin(PIN_GATE_IN_4), 1000);
}

void Dubby::InitMidi()
{
    MidiUartHandler::Config midi_uart_config;
    midi_uart.Init(midi_uart_config);

    // RELAY FOR SWITCHING MIDI OUT / MIDI THRU
    midi_sw_output.pin  = seed.GetPin(PIN_MIDI_SWITCH);
    midi_sw_output.mode = DSY_GPIO_MODE_OUTPUT_PP;
    midi_sw_output.pull = DSY_GPIO_NOPULL;
    dsy_gpio_init(&midi_sw_output);

    dsy_gpio_write(&midi_sw_output, false); 

    MidiUsbHandler::Config midi_usb_cfg;
    midi_usb_cfg.transport_config.periph = MidiUsbTransport::Config::EXTERNAL;
    midi_usb.Init(midi_usb_cfg);
}

void Dubby::InitDisplay() 
{
    /** Configure the Display */
    OledDisplay<SSD130x4WireSpi128x64Driver>::Config disp_cfg;
    disp_cfg.driver_config.transport_config.pin_config.dc    = seed.GetPin(9);
    disp_cfg.driver_config.transport_config.pin_config.reset = seed.GetPin(31);
    /** And Initialize */
    display.Init(disp_cfg);
}

void Dubby::UpdateDisplay() 
{ 
    if (encoder.TimeHeldMs() > ENCODER_LONGPRESS_THRESHOLD) 
    {
        if (!windowSelectorActive) 
        {
            windowSelectorActive = true;
            HighlightWindowItem();
        }

        if (encoder.Increment()) UpdateWindowSelector(encoder.Increment(), true);
    } 

    
    if (encoder.TimeHeldMs() < ENCODER_LONGPRESS_THRESHOLD && windowSelectorActive)
    {
        windowSelectorActive = false;
        
        ReleaseWindowSelector();
        UpdateWindowList();
    }

    switch(windowItemSelected) 
    {
        case WIN1:
            UpdateRenderPane();
            break;
        case WIN2:
            UpdateMixerPane();
            break;
        case WIN3:

            if (encoder.FallingEdge() && !isSubMenuActive && !wasEncoderLongPressed) {
                isSubMenuActive = true;
                DisplayPreferencesMenuList(0);
            }

            if (windowSelectorActive) {
                isSubMenuActive = false;
                DisplayPreferencesMenuList(0);
            }

            DisplayPreferencesSubMenuList(encoder.Increment(), preferencesMenuItemSelected);
            if (encoder.FallingEdge() && !wasEncoderLongPressed && preferencesMenuItemSelected == DFUMODE) ResetToBootloader();
            if (encoder.Increment() && !windowSelectorActive && !isSubMenuActive) UpdatePreferencesMenuList(encoder.Increment());
            else if (encoder.Increment() && !windowSelectorActive && isSubMenuActive) UpdatePreferencesSubMenuList(encoder.Increment(), preferencesMenuItemSelected);
            break;
        default:
            break;
    }
    
}

void Dubby::DrawLogo() 
{
    DrawBitmap(0);   
}

void Dubby::DrawBitmap(int bitmapIndex)
{
    display.Fill(false);
    display.SetCursor(30, 30);
    
    for (int x = 0; x < OLED_WIDTH; x += 8) {
        for (int y = 0; y < OLED_HEIGHT; y++) {
            // Calculate the index in the array
            int byteIndex = (y * OLED_WIDTH + x) / 8;

            // Get the byte containing 8 pixels
            char byte = bitmaps[bitmapIndex][byteIndex];

            // Process 8 pixels in the byte
            for (int bitIndex = 0; bitIndex < 8; bitIndex++) {

                // Get the pixel value (0 or 1)
                char pixel = (byte >> (7 - bitIndex)) & 0x01;

                bool isWhite = (pixel == 1);

                display.DrawPixel(x + bitIndex, y, isWhite);
            }

            if (y % 6 == 0) display.Update();
        }
    }
}

void Dubby::UpdateWindowSelector(int increment, bool higlight) 
{
    int wItemSelected = windowItemSelected;
    if ((int)windowItemSelected + increment >= WIN_LAST) wItemSelected = 0;
    else if ((int)windowItemSelected + increment < 0) wItemSelected = WIN_LAST - 1;
    else wItemSelected += increment;

    windowItemSelected = (WindowItems)(wItemSelected);

    display.Fill(false);

    if (higlight) HighlightWindowItem();
    else ReleaseWindowSelector();
    
    UpdateWindowList();

    display.Update();
}

void Dubby::HighlightWindowItem() 
{
    display.DrawRect(windowBoxBounding[0][0], windowBoxBounding[0][1], windowBoxBounding[0][2], windowBoxBounding[0][3] + 1, true, true);

    for (int i = 0; i < 3; i++) 
    {
        display.SetCursor(windowTextCursors[i % 3][0], windowTextCursors[i % 3][1]);
        int currentText = windowItemSelected + i < WIN_LAST ? windowItemSelected + i : (windowItemSelected + i) % WIN_LAST;
        
        display.WriteStringAligned(GetTextForEnum(WINDOWS, currentText), Font_6x8, daisy::Rectangle(windowBoxBounding[i][0], windowBoxBounding[i][1] + 3, 43, 7), daisy::Alignment::centered, i == 0 ? false : true);
    }

    display.DrawLine(PANE_X_START - 1, PANE_Y_START + 1, PANE_X_START - 1, PANE_Y_END + 1, true);
    
    display.DrawLine(PANE_X_START - 1, PANE_Y_END + 1, PANE_X_END - 1, PANE_Y_END + 1, true);

    display.DrawLine(windowItemSelected * scrollbarWidth, 63, (windowItemSelected * scrollbarWidth) + scrollbarWidth, 63, true);

    display.Update();
}

void Dubby::ReleaseWindowSelector() 
{        
    ClearPane();
    
    display.DrawRect(windowBoxBounding[0][0], windowBoxBounding[0][1], windowBoxBounding[0][2], windowBoxBounding[0][3], false, false);

    display.SetCursor(windowTextCursors[0][0], windowTextCursors[0][1]);
    display.WriteStringAligned(GetTextForEnum(WINDOWS, windowItemSelected), Font_6x8, daisy::Rectangle(windowBoxBounding[0][0], windowBoxBounding[0][1] + 3, 43, 7), daisy::Alignment::centered, true);

    display.Update();
    
}

void Dubby::ClearPane() 
{
    display.DrawRect(PANE_X_START - 1, PANE_Y_START - 1, PANE_X_END + 1, PANE_Y_END + 12, false, true);
}

void Dubby::UpdateMixerPane() 
{
    int increment = encoder.Increment();
    if (increment && !windowSelectorActive && !isBarSelected) {
        if ((((barSelector >= 0 && increment == 1 && barSelector < 7) || (increment == -1 && barSelector != 0)))) 
                barSelector += increment;
    }

    if (barSelector < 4 && mixerPageSelected == OUTPUTS) mixerPageSelected = INPUTS;
    else if (barSelector >= 4 && mixerPageSelected == INPUTS) mixerPageSelected = OUTPUTS;
    

    std::string statusStr = GetTextForEnum(MIXERPAGES, mixerPageSelected);
    UpdateStatusBar(&statusStr[0], LEFT);

    if (encoder.FallingEdge() && !wasEncoderLongPressed && !windowSelectorActive)
    {
        isBarSelected = !isBarSelected;
        if (isBarSelected)
        {
            std::string str  = (mixerPageSelected == INPUTS ? "in" : "out") + std::to_string(barSelector % 4 + 1) + ":" + std::to_string(audioGains[mixerPageSelected][barSelector % 4]).substr(0, std::to_string(audioGains[mixerPageSelected][barSelector % 4]).find(".") + 3);
            UpdateStatusBar(&str[0], RIGHT);
        }
        else 
        {
            UpdateStatusBar(" ", RIGHT);
        }
    }

    if (isBarSelected) {
        if ((increment == 1 && audioGains[mixerPageSelected][barSelector % 4] < 1.0f) || (increment == -1 && audioGains[mixerPageSelected][barSelector % 4] > 0.0001f)){
            audioGains[mixerPageSelected][barSelector % 4] += increment/20.f;
            audioGains[mixerPageSelected][barSelector % 4] = abs(audioGains[mixerPageSelected][barSelector % 4]);

            std::string str  = (mixerPageSelected == INPUTS ? "in" : "out") + std::to_string(barSelector % 4 + 1) + ":" + std::to_string(audioGains[mixerPageSelected][barSelector % 4]).substr(0, std::to_string(audioGains[mixerPageSelected][barSelector % 4]).find(".") + 3);
            UpdateStatusBar(&str[0], RIGHT);
        }
    }

    if(seed.system.GetNow() - screen_update_last_ > screen_update_period_)
    {
        screen_update_last_ = seed.system.GetNow();
        for (int i = 0; i < 4; i++) UpdateBar(i);
    }
}

void Dubby::UpdateWindowList()
{
    std::string statusStr;

    switch(windowItemSelected) 
    {
        case WIN1:    
            statusStr = GetTextForEnum(SCOPE, scopeSelector);
            UpdateStatusBar(&statusStr[0], LEFT);
            UpdateRenderPane();
            break;
        case WIN2:
            for (int i = 0; i < 4; i++) UpdateBar(i);
            break;
        case WIN3:
            DisplayPreferencesMenuList(0);
            break;
        case WIN4:
            UpdateStatusBar("pane 4", LEFT); 
            break;
        case WIN5:
            display.SetCursor(10, 15);
            UpdateStatusBar("pane 5", LEFT);
            break;
        case WIN6:
            display.SetCursor(10, 15);
            UpdateStatusBar("pane 6", LEFT);
            break;
        case WIN7:
            display.SetCursor(10, 15);
            UpdateStatusBar("pane 7", LEFT);
            break;
        case WIN8:
            display.SetCursor(10, 15);
            UpdateStatusBar("pane 8", LEFT);
            break;
        default:
            break;
    }

    display.Update();
}

void Dubby::UpdateBar(int i) 
{
    // clear bars
    display.DrawRect((i * 32)  + margin - 3, 10, ((i + 1) * 32) - margin + 3, 52, false, true);

    // highlight bar
    if (barSelector % 4 == i) display.DrawRect((i * 32)  + margin - 3, 11, ((i + 1) * 32) - margin + 3, 53, true, isBarSelected);

    // clear bars
    display.DrawRect((i * 32)  + margin, 12, ((i + 1) * 32) - margin, 52, false, true);

    // set gain
    display.DrawRect((i * 32)  + margin, int(abs(1.0f - audioGains[mixerPageSelected][i]) * 41.0f) + 12, ((i + 1) * 32) - margin, 52, true, false);

    // display sound output
    display.DrawRect((i * 32)  + margin, int(abs((currentLevels[mixerPageSelected][i] * 5.0f) - 1.0f) * 41.0f) + 12, ((i + 1) * 32) - margin, 53, true, true);

    
    display.Update();
}

void Dubby::UpdateRenderPane() 
{
    int increment = encoder.Increment();
    if (increment && !windowSelectorActive) {
        if ((((scopeSelector >= 0 && increment == 1 && scopeSelector < SCOPE_PAGES_LAST - 1) || (increment == -1 && scopeSelector != 0)))) 
                scopeSelector += increment;
    }

    if (increment) 
    {
        std::string statusStr = GetTextForEnum(SCOPE, scopeSelector);
        UpdateStatusBar(&statusStr[0], LEFT);
    }

    RenderScope();
}

void Dubby::RenderScope()
{
    if(seed.system.GetNow() - screen_update_last_ > screen_update_period_)
    {
        screen_update_last_ = seed.system.GetNow();
        display.DrawRect(PANE_X_START, PANE_Y_START, PANE_X_END, PANE_Y_END, false, true);
        int prev_x = 0;
        int prev_y = (OLED_HEIGHT - 15) / 2;
        for(size_t i = 0; i < AUDIO_BLOCK_SIZE; i++)
        {
            int y = 1 + std::min(std::max((OLED_HEIGHT - 15) / 2
                                        - int(scope_buffer[i] * 150),
                                    10),
                            OLED_HEIGHT - 15);
            int x = 1 + i * (OLED_WIDTH - 2) / AUDIO_BLOCK_SIZE;
            if(i != 0)
            {
                display.DrawLine(prev_x, prev_y, x, y, true);
            }
            prev_x = x;
            prev_y = y;
        }

        display.Update();
    }   
}

void Dubby::DisplayPreferencesMenuList(int increment)
{
    // clear bounding box
    //display.DrawRect(PANE_X_START - 1, 1, PANE_X_END, PANE_Y_END, false, true);

    int optionStart = 0;
    if (preferencesMenuItemSelected > 3)
    {
        optionStart = preferencesMenuItemSelected - 3;
    }
    
    // display each item, j for text cursor
    for (int i = optionStart, j = 0; i < optionStart + 4; i++, j++)
    {
        // clear item spaces
        if ((optionStart > 0 || (!optionStart && increment < 0))) {
            display.DrawRect(menuListBoxBounding[j][0], menuListBoxBounding[j][1], menuListBoxBounding[j][2], menuListBoxBounding[j][3], false, true);
        }

        // display and remove bounding boxes
        if (preferencesMenuItemSelected == i) {
            if(optionStart >= 0 && increment < 0 && j < 3) 
                display.DrawRect(menuListBoxBounding[j + 1][0], menuListBoxBounding[j + 1][1], menuListBoxBounding[j + 1][2], menuListBoxBounding[j + 1][3], false);
            else if(optionStart == 0 && j > 0)
                display.DrawRect(menuListBoxBounding[j - 1][0], menuListBoxBounding[j - 1][1], menuListBoxBounding[j - 1][2], menuListBoxBounding[j - 1][3], false);
            
            display.DrawRect(menuListBoxBounding[j][0], menuListBoxBounding[j][1], menuListBoxBounding[j][2], menuListBoxBounding[j][3], true);

            if (increment == 0)
                display.DrawRect(menuListBoxBounding[j][0], menuListBoxBounding[j][1], menuListBoxBounding[j][2], menuListBoxBounding[j][3], false, true);
            
            if (!isSubMenuActive)
                display.DrawRect(menuListBoxBounding[j][0], menuListBoxBounding[j][1], menuListBoxBounding[j][2], menuListBoxBounding[j][3], true, false);
            else
                display.DrawRect(menuListBoxBounding[j][0], menuListBoxBounding[j][1], menuListBoxBounding[j][2], menuListBoxBounding[j][3], true, true);
        } 

        display.SetCursor(5, MENULIST_Y_START + 2 + (j * MENULIST_SPACING));
        display.WriteString(GetTextForEnum(PREFERENCESMENU, i), Font_6x8, i == preferencesMenuItemSelected && isSubMenuActive ? false : true);

    }

    display.Update();
}

void Dubby::UpdatePreferencesMenuList(int increment) 
{
    if (((preferencesMenuItemSelected >= 0 && increment == 1 && preferencesMenuItemSelected < PREFERENCESMENU_LAST - 1) || (increment != 1 && preferencesMenuItemSelected != 0))) 
    {
        preferencesMenuItemSelected = (PreferencesMenuItems)(preferencesMenuItemSelected + increment);
        
        DisplayPreferencesMenuList(increment);
    }
}

void Dubby::DisplayPreferencesSubMenuList(int increment, PreferencesMenuItems prefMenuItemSelected)
{
    // clear bounding box
    display.DrawRect(PANE_X_START + MENULIST_SUBMENU_SPACING - 1, PANE_Y_START, PANE_X_END, PANE_Y_END, false, true);


    EnumTypes type; 

    switch (prefMenuItemSelected)
    {
    case MIDI:
        type = PREFERENCESMIDIMENULIST;
        break;
    case ROUTING:
        type = PREFERENCESROUTINGMENULIST;
        break;
    default:
        type = PREFERENCESMIDIMENULIST;
        break;
    }

    int optionStart = 0;
    if (subMenuSelector > 3)
    {
        optionStart = subMenuSelector - 3;
    }
    
    // display each item, j for text cursor
    for (int i = optionStart, j = 0; i < optionStart + 4; i++, j++)
    {
        // clear item spaces
        if ((optionStart > 0 || (!optionStart && increment < 0))) {
            display.DrawRect(menuListBoxBounding[j][0] + MENULIST_SUBMENU_SPACING, menuListBoxBounding[j][1], menuListBoxBounding[j][2] + MENULIST_SUBMENU_SPACING, menuListBoxBounding[j][3], false, true);
        }

        // display and remove bounding boxes
        if (subMenuSelector == i) {
            if(optionStart >= 0 && increment < 0 && j < 3) 
                display.DrawRect(menuListBoxBounding[j + 1][0] + MENULIST_SUBMENU_SPACING, menuListBoxBounding[j + 1][1], menuListBoxBounding[j + 1][2] + MENULIST_SUBMENU_SPACING, menuListBoxBounding[j + 1][3], false);
            else if(optionStart == 0 && j > 0)
                display.DrawRect(menuListBoxBounding[j - 1][0] + MENULIST_SUBMENU_SPACING, menuListBoxBounding[j - 1][1], menuListBoxBounding[j - 1][2] + MENULIST_SUBMENU_SPACING, menuListBoxBounding[j - 1][3], false);
            
            if (isSubMenuActive)
                display.DrawRect(menuListBoxBounding[j][0] + MENULIST_SUBMENU_SPACING, menuListBoxBounding[j][1], menuListBoxBounding[j][2] + MENULIST_SUBMENU_SPACING, menuListBoxBounding[j][3], true);
        } 

        display.SetCursor(5 + MENULIST_SUBMENU_SPACING, MENULIST_Y_START + 2 + (j * MENULIST_SPACING));
        display.WriteString(GetTextForEnum(type, i), Font_6x8, true);

    }

    display.DrawRect(PANE_X_START + MENULIST_SUBMENU_SPACING - 1, PANE_Y_START + 1, PANE_X_END, PANE_Y_END - 1, true, false);

    display.Update();
}

void Dubby::UpdatePreferencesSubMenuList(int increment, PreferencesMenuItems prefMenuItemSelected) 
{
    int endSelector = 0;

    EnumTypes type; 

    switch (prefMenuItemSelected)
    {
    case MIDI:
        endSelector = PREFERENCESMIDIMENU_LAST;
        break;
    case ROUTING:
        endSelector = PREFERENCESROUTINGMENU_LAST;
        break;
    default:
        endSelector = 0;
        break;
    }


    if (((subMenuSelector >= 0 && increment == 1 && subMenuSelector < endSelector - 1) || (increment != 1 && subMenuSelector != 0))) 
    {
        subMenuSelector = (PreferencesMenuItems)(subMenuSelector + increment);
        
        DisplayPreferencesSubMenuList(increment, prefMenuItemSelected);
    }
}


void Dubby::UpdateStatusBar(char* text, StatusBarSide side = LEFT) 
{
    if (side == LEFT) 
    {   
        display.DrawRect(STATUSBAR_X_START, STATUSBAR_Y_START, 63, STATUSBAR_Y_END - 3, false, true);
        display.WriteStringAligned(&text[0], Font_6x8, daisy::Rectangle(STATUSBAR_X_START, STATUSBAR_Y_START, STATUSBAR_X_END - 64, STATUSBAR_Y_END - 1), daisy::Alignment::centeredLeft, true);
    } 
    else if (side == RIGHT) 
    {   
        display.DrawRect(64, STATUSBAR_Y_START, 127, STATUSBAR_Y_END - 3, false, true);
        display.WriteStringAligned(&text[0], Font_6x8, daisy::Rectangle(64, STATUSBAR_Y_START, 64, STATUSBAR_Y_END - 1), daisy::Alignment::centeredRight, true);
    }
    else if (side == MIDDLE) 
    {   
        display.DrawRect(52, STATUSBAR_Y_START, 72, STATUSBAR_Y_END - 3, false, true);
        display.WriteStringAligned(&text[0], Font_6x8, daisy::Rectangle(36, STATUSBAR_Y_START, 58, STATUSBAR_Y_END - 1), daisy::Alignment::centered, true);
    }


    display.Update();
}

void Dubby::ResetToBootloader() 
{
    DrawBitmap(1);

    display.SetCursor(20, 55);
    display.WriteString("FIRMWARE UPDATE", Font_6x8, true);

    display.Update();

    System::ResetToBootloader(System::DAISY_INFINITE_TIMEOUT);
}

void Dubby::SwitchMIDIOutThru(bool state) 
{
    // state == true => PIN IS HIGH => MIDI THRU
    // state == false => PIN IS LOW => MIDI OUT
    dsy_gpio_write(&midi_sw_output, state); 
}

void Dubby::InitEncoder()
{
    encoder.Init(seed.GetPin(PIN_ENC_A),
                seed.GetPin(PIN_ENC_B),
                seed.GetPin(PIN_ENC_CLICK));
}

void Dubby::ProcessAllControls()
{
    ProcessAnalogControls();
    ProcessDigitalControls();
}

void Dubby::ProcessAnalogControls()
{
    for(size_t i = 0; i < CTRL_LAST; i++)
        analogInputs[i].Process();
}

void Dubby::ProcessDigitalControls()
{
    encoder.Debounce();

    if (encoder.Pressed()) {
        if (!isEncoderPressed) {
            // Encoder has just been pressed, record the start time
            isEncoderPressed = true;
            encoderPressStartTime = seed.system.GetNow();
        }
    } else {
        if (isEncoderPressed) {
            // Encoder has just been released
            isEncoderPressed = false;

            // Check if it was a short or long press
            unsigned long buttonPressDuration = seed.system.GetNow() - encoderPressStartTime;
            if (buttonPressDuration < ENCODER_LONGPRESS_THRESHOLD) {
                // Short press action
                wasEncoderLongPressed = false;

            } else {
                // Long press action
                wasEncoderLongPressed = true;
            }
        }
    }

    
    for (int i = 0; i < 4; i++) buttons[i].Debounce();
}

float Dubby::GetKnobValue(Ctrl k)
{
    return (analogInputs[k].Value());
}

void Dubby::InitAudio() 
{
    // Handle Seed Audio as-is and then
    SaiHandle::Config sai_config[2];
    // Internal Codec
    if(seed.CheckBoardVersion() == DaisySeed::BoardVersion::DAISY_SEED_1_1)
    {
        sai_config[0].pin_config.sa = {DSY_GPIOE, 6};
        sai_config[0].pin_config.sb = {DSY_GPIOE, 3};
        sai_config[0].a_dir         = SaiHandle::Config::Direction::RECEIVE;
        sai_config[0].b_dir         = SaiHandle::Config::Direction::TRANSMIT;
    }
    else
    {
        sai_config[0].pin_config.sa = {DSY_GPIOE, 6};
        sai_config[0].pin_config.sb = {DSY_GPIOE, 3};
        sai_config[0].a_dir         = SaiHandle::Config::Direction::TRANSMIT;
        sai_config[0].b_dir         = SaiHandle::Config::Direction::RECEIVE;
    }
    sai_config[0].periph          = SaiHandle::Config::Peripheral::SAI_1;
    sai_config[0].sr              = SaiHandle::Config::SampleRate::SAI_48KHZ;
    sai_config[0].bit_depth       = SaiHandle::Config::BitDepth::SAI_24BIT;
    sai_config[0].a_sync          = SaiHandle::Config::Sync::MASTER;
    sai_config[0].b_sync          = SaiHandle::Config::Sync::SLAVE;
    sai_config[0].pin_config.fs   = {DSY_GPIOE, 4};
    sai_config[0].pin_config.mclk = {DSY_GPIOE, 2};
    sai_config[0].pin_config.sck  = {DSY_GPIOE, 5};

    // External Codec

    I2CHandle::Config i2c_cfg;
    i2c_cfg.periph         = I2CHandle::Config::Peripheral::I2C_1;
    i2c_cfg.mode           = I2CHandle::Config::Mode::I2C_MASTER;
    i2c_cfg.speed          = I2CHandle::Config::Speed::I2C_400KHZ;
    i2c_cfg.pin_config.scl = {DSY_GPIOB, 8};
    i2c_cfg.pin_config.sda = {DSY_GPIOB, 9};

    I2CHandle i2c2;
    i2c2.Init(i2c_cfg);

    // pullups must be enabled
    GPIOB->PUPDR &= ~((GPIO_PUPDR_PUPD8)|(GPIO_PUPDR_PUPD9)); 
    GPIOB->PUPDR |= ((GPIO_PUPDR_PUPD8_0)|(GPIO_PUPDR_PUPD9_0)); 

    Pcm3060 codec;

    codec.Init(i2c2);        

    sai_config[1].periph          = SaiHandle::Config::Peripheral::SAI_2;
    sai_config[1].sr              = SaiHandle::Config::SampleRate::SAI_48KHZ;
    sai_config[1].bit_depth       = SaiHandle::Config::BitDepth::SAI_24BIT;
    sai_config[1].a_sync          = SaiHandle::Config::Sync::SLAVE;
    sai_config[1].b_sync          = SaiHandle::Config::Sync::MASTER;
    sai_config[1].pin_config.fs   = {DSY_GPIOG, 9};
    sai_config[1].pin_config.mclk = {DSY_GPIOA, 1};
    sai_config[1].pin_config.sck  = {DSY_GPIOA, 2};
    sai_config[1].a_dir         = SaiHandle::Config::Direction::TRANSMIT;
    sai_config[1].pin_config.sa = {DSY_GPIOD, 11};
    sai_config[1].b_dir         = SaiHandle::Config::Direction::RECEIVE;
    sai_config[1].pin_config.sb = {DSY_GPIOA, 0};

    SaiHandle sai_handle[2];
    sai_handle[0].Init(sai_config[1]);
    sai_handle[1].Init(sai_config[0]);

    // Reinit Audio for _both_ codecs...
    AudioHandle::Config cfg;
    cfg.blocksize  = 48;
    cfg.samplerate = SaiHandle::Config::SampleRate::SAI_48KHZ;
    cfg.postgain   = 0.5f;
    seed.audio_handle.Init(cfg, sai_handle[0], sai_handle[1]);
}

const char * Dubby::GetTextForEnum(EnumTypes m, int enumVal)
{
    switch (m)
    {
        case WINDOWS:
            return WindowItemsStrings[enumVal];
            break;
        case PREFERENCESMENU:
            return PreferencesMenuItemsStrings[enumVal];
            break;
        case PREFERENCESMIDIMENULIST:
            return PreferencesMidiMenuItemsStrings[enumVal];
            break;
        case PREFERENCESROUTINGMENULIST:
            return PreferencesRoutingMenuItemsStrings[enumVal];
            break;
        case SCOPE:
            return ScopePagesStrings[enumVal];
            break;
        case MIXERPAGES:
            return MixerPagesStrings[enumVal];
            break;
        default:
            return "";
            break;
    }
}