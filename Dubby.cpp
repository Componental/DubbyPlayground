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
#define PANE_Y_END 55

#define STATUSBAR_X_START 1
#define STATUSBAR_X_END 127
#define STATUSBAR_Y_START 0
#define STATUSBAR_Y_END 7

#define MENULIST_X_START 0
#define MENULIST_X_END 63
#define MENULIST_Y_START 8
#define MENULIST_Y_END 19
#define MENULIST_SPACING 8
#define MENULIST_SUBMENU_SPACING 63
#define MENULIST_ROWS_ON_SCREEN 5

#define PARAMLIST_X_START 1
#define PARAMLIST_X_END 127
#define PARAMLIST_Y_START 8
#define PARAMLIST_Y_END 15
#define PARAMLIST_SPACING 6
#define PARAMLIST_ROWS_ON_SCREEN 8

#define MIDILIST_X_START 1
#define MIDILIST_X_END 123
#define MIDILIST_Y_START 11
#define MIDILIST_Y_END 19
#define MIDILIST_SPACING 8
#define MIDILIST_ROWS_ON_SCREEN 5

#define ENCODER_LONGPRESS_THRESHOLD 300

int currentBitmapIndex = 2; // Initial bitmap index

void Dubby::Init()
{
    InitControls();
    InitButtons();

    screen_update_period_ = 17; // roughly 60Hz
    screen_update_last_ = seed.system.GetNow();

    for (int i = 0; i < MENULIST_ROWS_ON_SCREEN; i++)
    {
        menuListBoxBounding[i][0] = MENULIST_X_START;
        menuListBoxBounding[i][1] = MENULIST_Y_START + i * MENULIST_SPACING;
        menuListBoxBounding[i][2] = MENULIST_X_END;
        menuListBoxBounding[i][3] = MENULIST_Y_END + i * MENULIST_SPACING;
    }

    for (int i = 0; i < PARAMLIST_ROWS_ON_SCREEN; i++)
    {
        paramListBoxBounding[i][0] = PARAMLIST_X_START;
        paramListBoxBounding[i][1] = PARAMLIST_Y_START + i * PARAMLIST_SPACING;
        paramListBoxBounding[i][2] = PARAMLIST_X_END;
        paramListBoxBounding[i][3] = PARAMLIST_Y_END + i * PARAMLIST_SPACING;
    }

    for (int i = 0; i < MIDILIST_ROWS_ON_SCREEN; i++)
    {
        midiListBoxBounding[i][0] = MIDILIST_X_START;
        midiListBoxBounding[i][1] = MIDILIST_Y_START + i * MIDILIST_SPACING;
        midiListBoxBounding[i][2] = MIDILIST_X_END;
        midiListBoxBounding[i][3] = MIDILIST_Y_END + i * MIDILIST_SPACING;
    }

    scrollbarWidth = int(128 / WIN_LAST);

    InitDisplay();
    InitEncoder();
    InitAudio();
    InitMidi();
    InitDubbyParameters();
    InitDubbyControls();
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
    for (size_t i = 0; i < CTRL_LAST; i++)
    {
        analogInputs[i].Init(seed.adc.GetPtr(i), seed.AudioCallbackRate(), true);
    }

    seed.adc.Start();
}

void Dubby::InitButtons()
{
    // Set button to pins, to be updated at a 1kHz  samplerate
    buttons[0].Init(seed.GetPin(PIN_GATE_IN_1), 1000);
    buttons[1].Init(seed.GetPin(PIN_GATE_IN_2), 1000);
    buttons[2].Init(seed.GetPin(PIN_GATE_IN_3), 1000);
    buttons[3].Init(seed.GetPin(PIN_GATE_IN_4), 1000);

    joystickButton.Init(seed.GetPin(PIN_JS_CLICK), 1000);
}

void Dubby::InitMidi()
{
    MidiUartHandler::Config midi_uart_config;
    midi_uart.Init(midi_uart_config);

    // RELAY FOR SWITCHING MIDI OUT / MIDI THRU
    midi_sw_output.pin = seed.GetPin(PIN_MIDI_SWITCH);
    midi_sw_output.mode = DSY_GPIO_MODE_OUTPUT_PP;
    midi_sw_output.pull = DSY_GPIO_NOPULL;
    dsy_gpio_init(&midi_sw_output);

    dsy_gpio_write(&midi_sw_output, false);

    MidiUsbHandler::Config midi_usb_cfg;
    midi_usb_cfg.transport_config.periph = MidiUsbTransport::Config::EXTERNAL;
    midi_usb.Init(midi_usb_cfg);

    SwitchMIDIOutThru(true);
}

void Dubby::InitDisplay()
{
    /** Configure the Display */
    OledDisplay<SSD130x4WireSpi128x64Driver>::Config disp_cfg;
    disp_cfg.driver_config.transport_config.pin_config.dc = seed.GetPin(9);
    disp_cfg.driver_config.transport_config.pin_config.reset = seed.GetPin(31);
    /** And Initialize */
    display.Init(disp_cfg);
}

void Dubby::InitDubbyControls()
{
    dubbyCtrls[0].Init(CONTROL_NONE, 0);
    dubbyCtrls[1].Init(KN1, 0);
    dubbyCtrls[2].Init(KN2, 0);
    dubbyCtrls[3].Init(KN3, 0);
    dubbyCtrls[4].Init(KN4, 0);
    dubbyCtrls[5].Init(BTN1, 0);
    dubbyCtrls[6].Init(BTN2, 0);
    dubbyCtrls[7].Init(BTN3, 0);
    dubbyCtrls[8].Init(BTN4, 0);
    dubbyCtrls[9].Init(JSX, 0);
    dubbyCtrls[10].Init(JSY, 0);
    dubbyCtrls[11].Init(JSSW, 0);
}

void Dubby::InitDubbyParameters()
{
    dubbyParameters[DLY_TIME].Init(Params(DLY_TIME), KN1, 0, 0, 1000, EXPONENTIAL);
    dubbyParameters[DLY_FEEDBACK].Init(Params(DLY_FEEDBACK), KN2, 0, 0, 1, LINEAR);
    dubbyParameters[DLY_DIVISION].Init(Params(DLY_DIVISION), JSY, 0, 0, 1, LINEAR);
    dubbyParameters[DLY_SPREAD].Init(Params(DLY_SPREAD), JSX, 0, -250, 250, LINEAR);
    dubbyParameters[DLY_FREEZE].Init(Params(DLY_FREEZE), BTN3, 0, 0, 1, LINEAR);
    dubbyParameters[DLY_MAXWET].Init(Params(DLY_MAXWET), BTN4, 0, 0, 1, LINEAR);
    dubbyParameters[DLY_MIX].Init(Params(DLY_MIX), KN4, 0, 0, 1, LINEAR);
    dubbyParameters[FLT_CUTOFF].Init(Params(FLT_CUTOFF), KN3, 1, 0, 2000, EXPONENTIAL);
    dubbyParameters[FLT_RESONANCE].Init(Params(FLT_RESONANCE), CONTROL_NONE, 0.3, 0, 1, LINEAR);
    dubbyParameters[OUT_GAIN].Init(Params(OUT_GAIN), CONTROL_NONE, 1, 0, 1, LOGARITHMIC);
}

void Dubby::SetAudioInGain(AudioIns in, float gain)
{
    if (gain > 1.0f)
        gain = 1.0f;
    else if (gain < 0.0f)
        gain = 0.0f;

    audioGains[0][in] = gain * 0.8f;
}

float Dubby::GetAudioInGain(AudioIns in)
{
    return audioGains[0][in];
}

void Dubby::SetAudioOutGain(AudioOuts out, float gain)
{
    if (gain > 1.0f)
        gain = 1.0f;
    else if (gain < 0.0f)
        gain = 0.0f;

    audioGains[0][out] = gain * 0.8f;
}

float Dubby::GetAudioOutGain(AudioOuts out)
{
    return audioGains[1][out];
}

void Dubby::UpdateDisplay()
{

    if (encoder.TimeHeldMs() > ENCODER_LONGPRESS_THRESHOLD && !windowSelectorActive)
    {
        windowSelectorActive = true;
    }

    if (windowSelectorActive)
    {
        HighlightWindowItem();
        if (encoder.Increment())
            UpdateWindowSelector(encoder.Increment(), true);

        if (encoder.RisingEdge())
        {
            windowSelectorActive = false;

            ReleaseWindowSelector();
            UpdateWindowList();
        }

        if (!wasEncoderJustInHighlightMenu && encoder.FallingEdge())
            wasEncoderJustInHighlightMenu = true;
    }

    if (wasEncoderJustInHighlightMenu && encoder.FallingEdge())
    {
        if (highlightMenuCounter < 2)
        {
            highlightMenuCounter++;
        }
        else
        {
            wasEncoderJustInHighlightMenu = false;
            highlightMenuCounter = 0;
        }
    }

    switch (windowItemSelected)
    {
    case WIN1:
        UpdateRenderPane();
        break;
    case WIN2:
        UpdateMixerPane();
        break;
    case WIN3:

        if (encoder.FallingEdge() && !isSubMenuActive && !wasEncoderJustInHighlightMenu)
        {
            isSubMenuActive = true;
            DisplayPreferencesMenuList(0);
        }

        if (windowSelectorActive)
        {
            isSubMenuActive = false;
            DisplayPreferencesMenuList(0);
        }

        DisplayPreferencesSubMenuList(encoder.Increment(), preferencesMenuItemSelected);
        if (encoder.FallingEdge() && !wasEncoderJustInHighlightMenu && preferencesMenuItemSelected == DFUMODE)
            ResetToBootloader();
        if (encoder.Increment() && !windowSelectorActive && !isSubMenuActive)
            UpdatePreferencesMenuList(encoder.Increment());
        else if (encoder.Increment() && !windowSelectorActive && isSubMenuActive)
            UpdatePreferencesSubMenuList(encoder.Increment(), preferencesMenuItemSelected);
        break;
    case WIN4:
        // if (encoder.FallingEdge() && !wasEncoderJustInHighlightMenu) {
        DisplayParameterList(encoder.Increment());

        if (encoder.Increment() && !isEncoderIncrementDisabled && !windowSelectorActive && !isParameterSelected)
            UpdateParameterList(encoder.Increment());

        if (encoder.FallingEdge() && !wasEncoderJustInHighlightMenu && !windowSelectorActive && !isParameterSelected)
        {
            isParameterSelected = true;
            parameterOptionSelected = PARAM;

            DisplayParameterList(encoder.Increment());
        }

        if (isListeningControlChange)
        {

            if (encoder.Increment())
            {
                if (dubbyParameters[parameterSelected].control == CONTROLS_LAST - 1 && encoder.Increment() == 1)
                    dubbyParameters[parameterSelected].control = CONTROL_NONE;
                else if (dubbyParameters[parameterSelected].control == CONTROL_NONE && encoder.Increment() == -1)
                    dubbyParameters[parameterSelected].control = (DubbyControls)(CONTROLS_LAST - 1);
                else
                    dubbyParameters[parameterSelected].control = static_cast<DubbyControls>(static_cast<int>(dubbyParameters[parameterSelected].control) + encoder.Increment());
            }

            if (EncoderFallingEdgeCustom())
            {
                isListeningControlChange = false;
                isEncoderIncrementDisabled = false;
                UpdateStatusBar(" PARAM       CTRL      VALUE  >", LEFT);

                trigger_save_parameters_qspi = true;
            }
            else
            {
                for (int i = 0; i < CONTROLS_LAST; i++)
                {
                    if (abs(dubbyCtrls[i].tempValue - dubbyCtrls[i].value) > 0.1f)
                    {

                    dubbyParameters[parameterSelected].control = (DubbyControls)i;

                        DisplayParameterList(0);
                        UpdateStatusBar(" PARAM       CTRL      VALUE  >", LEFT);

                        isListeningControlChange = false;
                        isEncoderIncrementDisabled = false;
                        trigger_save_parameters_qspi = true;
                    }
                }
            }
        }
        else if (isCurveChanging)
        {
            if (encoder.Increment())
            {
                if (dubbyParameters[parameterSelected].curve == CURVES_LAST - 1 && encoder.Increment() == 1)
                    dubbyParameters[parameterSelected].curve = (Curves)0;
                else if (dubbyParameters[parameterSelected].curve == 0 && encoder.Increment() == -1)
                    dubbyParameters[parameterSelected].curve = (Curves)(CURVES_LAST - 1);
                else
                    dubbyParameters[parameterSelected].curve = static_cast<Curves>(static_cast<int>(dubbyParameters[parameterSelected].curve) + encoder.Increment());
            }
            if (EncoderFallingEdgeCustom())
            {
                isCurveChanging = false;
                isEncoderIncrementDisabled = false;
                UpdateStatusBar(" PARAM       CTRL   <  CURVE   ", LEFT);

                trigger_save_parameters_qspi = true;
            }
        }
        else if (isMinChanging)
        {
            if (encoder.Increment())
            {
                int incrementValue = encoder.Increment();
                float newValue = dubbyParameters[parameterSelected].min + incrementValue;

                // Check min limit
                if (dubbyParameters[parameterSelected].hasMinLimit && newValue < dubbyParameters[parameterSelected].minLimit)
                    newValue = dubbyParameters[parameterSelected].minLimit;

                // Check max limit
                if (dubbyParameters[parameterSelected].hasMaxLimit && newValue > dubbyParameters[parameterSelected].maxLimit)
                    newValue = dubbyParameters[parameterSelected].maxLimit;

                // Apply the new value
                dubbyParameters[parameterSelected].min = newValue;
            }
            if (EncoderFallingEdgeCustom())
            {
                isMinChanging = false;
                isEncoderIncrementDisabled = false;
                UpdateStatusBar(" PARAM       CTRL   <  MIN    >", LEFT);

                trigger_save_parameters_qspi = true;
            }
        }
        else if (isMaxChanging)
        {
            if (encoder.Increment())
            {
                int incrementValue = encoder.Increment();
                float newValue = dubbyParameters[parameterSelected].max + incrementValue;

                // Check min limit
                if (dubbyParameters[parameterSelected].hasMinLimit && newValue < dubbyParameters[parameterSelected].minLimit)
                    newValue = dubbyParameters[parameterSelected].minLimit;

                // Check max limit
                if (dubbyParameters[parameterSelected].hasMaxLimit && newValue > dubbyParameters[parameterSelected].maxLimit)
                    newValue = dubbyParameters[parameterSelected].maxLimit;

                // Apply the new value
                dubbyParameters[parameterSelected].max = newValue;
            }

            if (EncoderFallingEdgeCustom())
            {
                isMaxChanging = false;
                isEncoderIncrementDisabled = false;
                UpdateStatusBar(" PARAM       CTRL   <  MAX    >", LEFT);

                trigger_save_parameters_qspi = true;
            }
        }
        else if (isValueChanging)
        {
            if (encoder.Increment())
            {

                if (encoder.Increment() == -1 && dubbyParameters[parameterSelected].value > dubbyParameters[parameterSelected].minLimit && dubbyParameters[parameterSelected].hasMinLimit)
                    dubbyParameters[parameterSelected].value += encoder.Increment();
                else if (encoder.Increment() == 1 && dubbyParameters[parameterSelected].value < dubbyParameters[parameterSelected].maxLimit && dubbyParameters[parameterSelected].hasMaxLimit)
                    dubbyParameters[parameterSelected].value += encoder.Increment();
                else
                    dubbyParameters[parameterSelected].value += encoder.Increment();
            }
            if (EncoderFallingEdgeCustom())
            {
                isValueChanging = false;
                isEncoderIncrementDisabled = false;
                UpdateStatusBar(" PARAM       CTRL      VALUE  >", LEFT);

                trigger_save_parameters_qspi = true;
            }
        }

        if (isParameterSelected)
        {
            if (encoder.Increment() && !isEncoderIncrementDisabled)
            {
                parameterOptionSelected = static_cast<ParameterOptions>(static_cast<int>(parameterOptionSelected) + encoder.Increment());

                if (parameterOptionSelected < PARAM || parameterOptionSelected >= POPTIONS_LAST)
                {
                    isParameterSelected = false;
                }

                switch (parameterOptionSelected)
                {
                case MIN:
                    UpdateStatusBar(" PARAM       CTRL   <  MIN    >", LEFT);
                    break;
                case MAX:
                    UpdateStatusBar(" PARAM       CTRL   <  MAX    >", LEFT);
                    break;
                case CURVE:
                    UpdateStatusBar(" PARAM       CTRL   <  CURVE   ", LEFT);
                    break;
                default:
                    UpdateStatusBar(" PARAM       CTRL      VALUE  >", LEFT);
                    break;
                }

                DisplayParameterList(encoder.Increment());
            }
            else if (parameterOptionSelected == CTRL)
            {
                if (EncoderFallingEdgeCustom())
                {
                    UpdateStatusBar("SELECT A CONTROL", MIDDLE, 127);
                    isListeningControlChange = true;
                    isEncoderIncrementDisabled = true;

                    for (int i = 0; i < CONTROLS_LAST; i++)
                        dubbyCtrls[i].tempValue = dubbyCtrls[i].value;
                }
            }
            else if (parameterOptionSelected == CURVE)
            {
                if (EncoderFallingEdgeCustom())
                {
                    UpdateStatusBar("SELECT A CURVE", MIDDLE, 127);
                    isEncoderIncrementDisabled = true;
                    isCurveChanging = true;
                }
            }
            else if (parameterOptionSelected == MIN)
            {
                if (EncoderFallingEdgeCustom())
                {
                    UpdateStatusBar("SELECT MIN VALUE", MIDDLE, 127);
                    isEncoderIncrementDisabled = true;
                    isMinChanging = true;
                }
            }
            else if (parameterOptionSelected == MAX)
            {
                if (EncoderFallingEdgeCustom())
                {
                    UpdateStatusBar("SELECT MAX VALUE", MIDDLE, 127);
                    isEncoderIncrementDisabled = true;
                    isMaxChanging = true;
                }
            }
            else if (parameterOptionSelected == VALUE && dubbyParameters[parameterSelected].control == CONTROL_NONE)
            {
                if (EncoderFallingEdgeCustom())
                {
                    UpdateStatusBar("SELECT A VALUE", MIDDLE, 127);
                    isEncoderIncrementDisabled = true;
                    isValueChanging = true;
                }
            }
        }

        break;
        
    case WIN6:
        DisplayMidiSettingsList(encoder.Increment());

        if (encoder.Increment() && !windowSelectorActive && !isMidiSettingSelected)
            UpdateMidiSettingsList(encoder.Increment());

        if (encoder.FallingEdge() && !wasEncoderJustInHighlightMenu && !windowSelectorActive)
        {
            isMidiSettingSelected = !isMidiSettingSelected;

            UpdateMidiSettingsList(encoder.Increment());
        }

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

    for (int x = 0; x < OLED_WIDTH; x += 8)
    {
        for (int y = 0; y < OLED_HEIGHT; y++)
        {
            // Calculate the index in the array
            int byteIndex = (y * OLED_WIDTH + x) / 8;

            // Get the byte containing 8 pixels
            char byte = bitmaps[bitmapIndex][byteIndex];

            // Process 8 pixels in the byte
            for (int bitIndex = 0; bitIndex < 8; bitIndex++)
            {

                // Get the pixel value (0 or 1)
                char pixel = (byte >> (7 - bitIndex)) & 0x01;

                bool isWhite = (pixel == 1);

                display.DrawPixel(x + bitIndex, y, isWhite);
            }

            if (y % 6 == 0)
                display.Update();
        }
    }
    display.Update();
}

void Dubby::UpdateWindowSelector(int increment, bool higlight)
{
    int wItemSelected = windowItemSelected;
    if ((int)windowItemSelected + increment >= WIN_LAST)
        wItemSelected = 0;
    else if ((int)windowItemSelected + increment < 0)
        wItemSelected = WIN_LAST - 1;
    else
        wItemSelected += increment;

    windowItemSelected = (WindowItems)(wItemSelected);

    display.Fill(false);

    if (higlight)
        HighlightWindowItem();

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

        display.WriteStringAligned(GetTextForEnum(WINDOWS, currentText), Font_4x5, daisy::Rectangle(windowBoxBounding[i][0], windowBoxBounding[i][1] + 1, 43, 7), daisy::Alignment::centered, i == 0 ? false : true);
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
    display.WriteStringAligned(GetTextForEnum(WINDOWS, windowItemSelected), Font_4x5, daisy::Rectangle(windowBoxBounding[0][0], windowBoxBounding[0][1] + 1, 43, 7), daisy::Alignment::centered, true);

    display.Update();
}

void Dubby::ClearPane()
{
    display.DrawRect(PANE_X_START - 1, PANE_Y_START - 1, PANE_X_END + 1, PANE_Y_END + 12, false, true);
}

void Dubby::UpdateMixerPane()
{
    int increment = encoder.Increment();
    if (increment && !windowSelectorActive && !isBarSelected)
    {
        if ((((barSelector >= 0 && increment > 0 && barSelector < 7) || (increment < 0 && barSelector != 0))))
            barSelector += increment;
    }

    if (barSelector < 4 && mixerPageSelected == OUTPUTS)
        mixerPageSelected = INPUTS;
    else if (barSelector >= 4 && mixerPageSelected == INPUTS)
        mixerPageSelected = OUTPUTS;

    std::string statusStr = GetTextForEnum(MIXERPAGES, mixerPageSelected);
    UpdateStatusBar(&statusStr[0], LEFT);

    if (encoder.FallingEdge() && !wasEncoderJustInHighlightMenu && !windowSelectorActive)
    {
        isBarSelected = !isBarSelected;
        if (isBarSelected)
        {
            std::string str = (mixerPageSelected == INPUTS ? "in" : "out") + std::to_string(barSelector % 4 + 1) + ":" + std::to_string(audioGains[mixerPageSelected][barSelector % 4]).substr(0, std::to_string(audioGains[mixerPageSelected][barSelector % 4]).find(".") + 3);
            UpdateStatusBar(&str[0], RIGHT);
        }
        else
        {
            UpdateStatusBar(" ", RIGHT);
        }
    }

    if (isBarSelected)
    {
        if ((increment == 1 && audioGains[mixerPageSelected][barSelector % 4] < 1.0f) || (increment == -1 && audioGains[mixerPageSelected][barSelector % 4] > 0.0001f))
        {
            audioGains[mixerPageSelected][barSelector % 4] += increment / 20.f;
            audioGains[mixerPageSelected][barSelector % 4] = abs(audioGains[mixerPageSelected][barSelector % 4]);

            std::string str = (mixerPageSelected == INPUTS ? "in" : "out") + std::to_string(barSelector % 4 + 1) + ":" + std::to_string(audioGains[mixerPageSelected][barSelector % 4]).substr(0, std::to_string(audioGains[mixerPageSelected][barSelector % 4]).find(".") + 3);
            UpdateStatusBar(&str[0], RIGHT);
        }
    }

    if (seed.system.GetNow() - screen_update_last_ > screen_update_period_)
    {
        screen_update_last_ = seed.system.GetNow();
        for (int i = 0; i < 4; i++)
            UpdateBar(i);
    }
}

void Dubby::UpdateWindowList()
{
    std::string statusStr;

    switch (windowItemSelected)
    {
    case WIN1:
        statusStr = GetTextForEnum(SCOPE, scopeSelector);
        UpdateStatusBar(&statusStr[0], LEFT, 70);
        UpdateRenderPane();
        break;
    case WIN2:
        for (int i = 0; i < 4; i++)
            UpdateBar(i);
        break;
    case WIN3:
        DisplayPreferencesMenuList(0);
        break;
    case WIN4:
        UpdateStatusBar(" PARAM       CTRL      VALUE  >", LEFT);
        display.DrawLine(6, 7, 127, 7, true);

        DisplayParameterList(0);

        break;
    case WIN5:

        break;
    case WIN6:
        //            display.SetCursor(10, 15);
        UpdateStatusBar(" SETTING              VALUE    ", LEFT);
        display.DrawLine(6, 10, 121, 10, true);

        DisplayMidiSettingsList(0);

        break;
    case WIN7:
        display.SetCursor(10, 15);
        UpdateStatusBar("PANE 7", LEFT);
        break;
    case WIN8:
        display.SetCursor(10, 15);
        UpdateStatusBar("PANE 8", LEFT);
        break;
    default:
        break;
    }

    display.Update();
}

void Dubby::UpdateBar(int i)
{
    // clear bars
    display.DrawRect((i * 32) + margin - 3, 10, ((i + 1) * 32) - margin + 3, 52, false, true);

    // highlight bar
    if (barSelector % 4 == i)
        display.DrawRect((i * 32) + margin - 3, 11, ((i + 1) * 32) - margin + 3, 53, true, isBarSelected);

    // clear bars
    display.DrawRect((i * 32) + margin, 12, ((i + 1) * 32) - margin, 52, false, true);

    // set gain
    display.DrawRect((i * 32) + margin, int(abs(1.0f - audioGains[mixerPageSelected][i]) * 41.0f) + 12, ((i + 1) * 32) - margin, 52, true, false);

    // display sound output
    display.DrawRect((i * 32) + margin, int(abs((currentLevels[mixerPageSelected][i] * 5.0f) - 1.0f) * 41.0f) + 12, ((i + 1) * 32) - margin, 53, true, true);

    display.Update();
}

void Dubby::UpdateRenderPane()
{
    int increment = encoder.Increment();
    if (increment && !windowSelectorActive)
    {
        if ((((scopeSelector >= 0 && increment > 0 && scopeSelector < SCOPE_PAGES_LAST - 1) || (increment < 0 && scopeSelector != 0))))
            scopeSelector += increment;
    }

    if (increment)
    {
        std::string statusStr = GetTextForEnum(SCOPE, scopeSelector);
        UpdateStatusBar(&statusStr[0], LEFT, 70);
    }

    RenderScope();
}

void Dubby::RenderScope()
{
    if (seed.system.GetNow() - screen_update_last_ > screen_update_period_)
    {
        screen_update_last_ = seed.system.GetNow();
        display.DrawRect(PANE_X_START, PANE_Y_START, PANE_X_END, PANE_Y_END, false, true);
        int prev_x = 0;
        int prev_y = (OLED_HEIGHT - 15) / 2;
        for (size_t i = 0; i < AUDIO_BLOCK_SIZE; i++)
        {
            int y = 1 + std::min(std::max((OLED_HEIGHT - 15) / 2 - int(scope_buffer[i] * 150),
                                          10),
                                 OLED_HEIGHT - 15);
            int x = 1 + i * (OLED_WIDTH - 2) / AUDIO_BLOCK_SIZE;
            if (i != 0)
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
    // display.DrawRect(PANE_X_START - 1, 1, PANE_X_END, PANE_Y_END, false, true);

    int optionStart = 0;
    if (preferencesMenuItemSelected > (MENULIST_ROWS_ON_SCREEN - 1))
    {
        optionStart = preferencesMenuItemSelected - (MENULIST_ROWS_ON_SCREEN - 1);
    }

    // display each item, j for text cursor
    for (int i = optionStart, j = 0; i < optionStart + MENULIST_ROWS_ON_SCREEN; i++, j++)
    {
        // clear item spaces
        if ((optionStart > 0 || (!optionStart && increment < 0)))
        {
            display.DrawRect(menuListBoxBounding[j][0], menuListBoxBounding[j][1], menuListBoxBounding[j][2], menuListBoxBounding[j][3], false, true);
        }

        // display and remove bounding boxes
        if (preferencesMenuItemSelected == i)
        {
            if (optionStart >= 0 && increment < 0 && j < MENULIST_ROWS_ON_SCREEN - 1)
                display.DrawRect(menuListBoxBounding[j + 1][0], menuListBoxBounding[j + 1][1], menuListBoxBounding[j + 1][2], menuListBoxBounding[j + 1][3], false);
            else if (optionStart == 0 && j > 0)
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
        display.WriteString(GetTextForEnum(PREFERENCESMENU, i), Font_4x5, i == preferencesMenuItemSelected && isSubMenuActive ? false : true);
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

    int optionStart = 1;
    if (subMenuSelector > (MENULIST_ROWS_ON_SCREEN - 1))
    {
        optionStart = subMenuSelector - (MENULIST_ROWS_ON_SCREEN - 1);
    }

    // display each item, j for text cursor
    for (int i = optionStart, j = 0; i < optionStart + MENULIST_ROWS_ON_SCREEN; i++, j++)
    {
        // clear item spaces
        if ((optionStart > 0 || (!optionStart && increment < 0)))
        {
            display.DrawRect(menuListBoxBounding[j][0] + MENULIST_SUBMENU_SPACING, menuListBoxBounding[j][1], menuListBoxBounding[j][2] + MENULIST_SUBMENU_SPACING, menuListBoxBounding[j][3], false, true);
        }

        // display and remove bounding boxes
        if (subMenuSelector == i)
        {
            if (optionStart >= 0 && increment < 0 && j < 3)
                display.DrawRect(menuListBoxBounding[j + 1][0] + MENULIST_SUBMENU_SPACING, menuListBoxBounding[j + 1][1], menuListBoxBounding[j + 1][2] + MENULIST_SUBMENU_SPACING, menuListBoxBounding[j + 1][3], false);
            else if (optionStart == 0 && j > 0)
                display.DrawRect(menuListBoxBounding[j - 1][0] + MENULIST_SUBMENU_SPACING, menuListBoxBounding[j - 1][1], menuListBoxBounding[j - 1][2] + MENULIST_SUBMENU_SPACING, menuListBoxBounding[j - 1][3], false);

            if (isSubMenuActive)
                display.DrawRect(menuListBoxBounding[j][0] + MENULIST_SUBMENU_SPACING, menuListBoxBounding[j][1], menuListBoxBounding[j][2] + MENULIST_SUBMENU_SPACING, menuListBoxBounding[j][3], true);
        }

        display.SetCursor(5 + MENULIST_SUBMENU_SPACING, MENULIST_Y_START + 2 + (j * MENULIST_SPACING));
        display.WriteString(GetTextForEnum(type, i), Font_4x5, true);
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

void Dubby::UpdateStatusBar(char *text, StatusBarSide side = LEFT, int width)
{
    Rectangle barRec = daisy::Rectangle(STATUSBAR_X_START, STATUSBAR_Y_START, STATUSBAR_X_END, STATUSBAR_Y_END);

    if (side == LEFT)
    {
        display.DrawRect(STATUSBAR_X_START, STATUSBAR_Y_START, width, STATUSBAR_Y_END - 1, false, true);
        display.WriteStringAligned(&text[0], Font_4x5, barRec, daisy::Alignment::centeredLeft, true);
    }
    else if (side == MIDDLE)
    {
        display.DrawRect(64 - (width / 2), STATUSBAR_Y_START, 64 + (width / 2), STATUSBAR_Y_END - 1, false, true);
        display.WriteStringAligned(&text[0], Font_4x5, barRec, daisy::Alignment::centered, true);
    }
    else if (side == RIGHT)
    {
        display.DrawRect(STATUSBAR_X_END - width, STATUSBAR_Y_START, STATUSBAR_X_END, STATUSBAR_Y_END - 1, false, true);
        display.WriteStringAligned(&text[0], Font_4x5, barRec, daisy::Alignment::centeredRight, true);
    }

    display.Update();
}

void Dubby::DisplayParameterList(int increment)
{
    // clear bounding box
    // display.DrawRect(PANE_X_START - 1, 1, PANE_X_END, PANE_Y_END, false, true);

    int optionStart = 1;
    if (parameterSelected > (PARAMLIST_ROWS_ON_SCREEN - 1))
    {
        optionStart = parameterSelected - (PARAMLIST_ROWS_ON_SCREEN - 1);
    }

    // display each item, j for text cursor
    for (int i = optionStart, j = 0; i < optionStart + PARAMLIST_ROWS_ON_SCREEN; i++, j++)
    {
        // clear item spaces
        display.DrawRect(paramListBoxBounding[j][0], paramListBoxBounding[j][1], paramListBoxBounding[j][2], paramListBoxBounding[j][3], false, true);

        if (parameterSelected == i)
        {

            if (isParameterSelected)
                display.DrawRect(paramListBoxBounding[j][0], paramListBoxBounding[j][1], paramListBoxBounding[j][2], paramListBoxBounding[j][3], true, true);

            int x;

            if (isParameterSelected)
            {
                switch (parameterOptionSelected)
                {
                case CTRL:
                    x = 51;
                    break;
                case VALUE:
                case MIN:
                case MAX:
                case CURVE:
                    x = 91;
                    break;
                default:
                    x = 3;
                    break;
                }

                display.DrawCircle(x, paramListBoxBounding[j][1] + 3, 1, !isParameterSelected);
            }
            else
            {
                display.DrawCircle(3, paramListBoxBounding[j][1] + 3, 1, !isParameterSelected);
            }
        }

        display.SetCursor(5, PARAMLIST_Y_START + 1 + (j * PARAMLIST_SPACING));
        display.WriteString(ParamsStrings[i], Font_4x5, !(parameterSelected == i && isParameterSelected));

        display.SetCursor(53, PARAMLIST_Y_START + 1 + (j * PARAMLIST_SPACING));
        display.WriteString(ControlsStrings[dubbyParameters[i].control], Font_4x5, !(parameterSelected == i && isParameterSelected));

        std::string str;
        switch (parameterOptionSelected)
        {
        case MIN:
            str = std::to_string(dubbyParameters[i].min).substr(0, std::to_string(dubbyParameters[i].min).find(".") + 3);
            break;
        case MAX:
            str = std::to_string(dubbyParameters[i].max).substr(0, std::to_string(dubbyParameters[i].max).find(".") + 3);
            break;
        case CURVE:
            str = CurvesStrings[dubbyParameters[i].curve];
            break;
        default:
            str = std::to_string(dubbyParameters[i].value).substr(0, std::to_string(dubbyParameters[i].value).find(".") + 3);
            break;
        }

        display.SetCursor(93, PARAMLIST_Y_START + 1 + (j * PARAMLIST_SPACING));
        display.WriteString(&str[0], Font_4x5, !(parameterSelected == i && isParameterSelected));
    }

    display.Update();
}

void Dubby::DisplayMidiSettingsList(int increment)
{

    testBool = isMidiSettingSelected;
    if (dubbyMidiSettings.currentMidiClockOption == FOLLOWER)
    {

        dubbyMidiSettings.currentBpm = receivedBPM;
    }

    if (isMidiSettingSelected)
    {
        // Adjust the selected option based on the encoder's input
        switch (midiSettingSelected)
        {
        case MIDICLOCK:
            dubbyMidiSettings.currentMidiClockOption = (dubbyMidiSettings.currentMidiClockOption + increment + MIDICLOCKOPTIONS_LAST) % MIDICLOCKOPTIONS_LAST;
            break;
        case BPM:
            if (dubbyMidiSettings.currentMidiClockOption == LEADER)
            {

                dubbyMidiSettings.currentBpm += increment;
                if (dubbyMidiSettings.currentBpm < 20)
                    dubbyMidiSettings.currentBpm = 20;
                else if (dubbyMidiSettings.currentBpm > 300)
                    dubbyMidiSettings.currentBpm = 300;
            }
            else
            {
                dubbyMidiSettings.currentBpm = receivedBPM;
            }
            break;

        case MIDIIN:
            dubbyMidiSettings.currentMidiInOption = (dubbyMidiSettings.currentMidiInOption + increment + MIDIINOPTIONS_LAST) % MIDIINOPTIONS_LAST;
            break;
        case MIDIINCHN:
            dubbyMidiSettings.currentMidiInChannelOption = (dubbyMidiSettings.currentMidiInChannelOption + increment + MIDIINCHNOPTIONS_LAST) % MIDIINCHNOPTIONS_LAST;
            break;
        case MIDIOUT:
            dubbyMidiSettings.currentMidiOutOption = (dubbyMidiSettings.currentMidiOutOption + increment + MIDIOUTOPTIONS_LAST) % MIDIOUTOPTIONS_LAST;
            break;
        case MIDIOUTCHN:
            dubbyMidiSettings.currentMidiOutChannelOption = (dubbyMidiSettings.currentMidiOutChannelOption + increment + MIDIOUTCHNOPTIONS_LAST) % MIDIOUTCHNOPTIONS_LAST;
            break;
        case MIDITHRUOUT:
            dubbyMidiSettings.currentMidiThruOutOption += increment;
            if (dubbyMidiSettings.currentMidiThruOutOption < 0)
                dubbyMidiSettings.currentMidiThruOutOption = 0;
            else if (dubbyMidiSettings.currentMidiThruOutOption > 1)
                dubbyMidiSettings.currentMidiThruOutOption = 1;
            break;
        default:
            // Add default action if needed
            break;
        }
    }

    int optionStart = 0;
    if (midiSettingSelected > (MIDILIST_ROWS_ON_SCREEN - 1))
    {
        optionStart = midiSettingSelected - (MIDILIST_ROWS_ON_SCREEN - 1);
    }

    for (int i = optionStart, j = 0; i < optionStart + MIDILIST_ROWS_ON_SCREEN; i++, j++)
    {
        display.DrawRect(midiListBoxBounding[j][0], midiListBoxBounding[j][1], midiListBoxBounding[j][2], midiListBoxBounding[j][3], false, true);

        if (midiSettingSelected == i)
        {
            if (isMidiSettingSelected)
                display.DrawRect(midiListBoxBounding[j][0], midiListBoxBounding[j][1], midiListBoxBounding[j][2], midiListBoxBounding[j][3], true, true);

            int x = 3;
            display.DrawCircle(x, midiListBoxBounding[j][1] + 4, 1, !isMidiSettingSelected);
        }

        display.SetCursor(5, MIDILIST_Y_START + 2 + (j * MIDILIST_SPACING));
        display.WriteString(dubbyMidiSettings.MidiSettingsStrings[i], Font_4x5, !(midiSettingSelected == i && isMidiSettingSelected));

        // Displaying the corresponding first index string for each enum
        switch (i)

        {
        case MIDICLOCK:
            display.SetCursor(90, MIDILIST_Y_START + 2 + (j * MIDILIST_SPACING));
            display.WriteString(dubbyMidiSettings.MidiClockOptionsStrings[dubbyMidiSettings.currentMidiClockOption], Font_4x5, !(midiSettingSelected == i && isMidiSettingSelected));
            break;
        case BPM:
            display.SetCursor(90, MIDILIST_Y_START + 2 + (j * MIDILIST_SPACING));
            char bpmStr[4];
            snprintf(bpmStr, 4, "%d", dubbyMidiSettings.currentBpm);
            display.WriteString(bpmStr, Font_4x5, !(midiSettingSelected == i && isMidiSettingSelected));
            break;

        case MIDIIN:
            display.SetCursor(90, MIDILIST_Y_START + 2 + (j * MIDILIST_SPACING));
            display.WriteString(dubbyMidiSettings.MidiInOptionsStrings[dubbyMidiSettings.currentMidiInOption], Font_4x5, !(midiSettingSelected == i && isMidiSettingSelected));
            break;
        case MIDIINCHN:
            display.SetCursor(90, MIDILIST_Y_START + 2 + (j * MIDILIST_SPACING));
            display.WriteString(dubbyMidiSettings.MidiInChannelOptionsStrings[dubbyMidiSettings.currentMidiInChannelOption], Font_4x5, !(midiSettingSelected == i && isMidiSettingSelected));
            break;
        case MIDIOUT:
            display.SetCursor(90, MIDILIST_Y_START + 2 + (j * MIDILIST_SPACING));
            display.WriteString(dubbyMidiSettings.MidiOutOptionsStrings[dubbyMidiSettings.currentMidiOutOption], Font_4x5, !(midiSettingSelected == i && isMidiSettingSelected));
            break;
        case MIDIOUTCHN:
            display.SetCursor(90, MIDILIST_Y_START + 2 + (j * MIDILIST_SPACING));
            display.WriteString(dubbyMidiSettings.MidiOutChannelOptionsStrings[dubbyMidiSettings.currentMidiOutChannelOption], Font_4x5, !(midiSettingSelected == i && isMidiSettingSelected));
            break;
        case MIDITHRUOUT:
            display.SetCursor(90, MIDILIST_Y_START + 2 + (j * MIDILIST_SPACING));
            display.WriteString(dubbyMidiSettings.MidiThruOutOptionsStrings[dubbyMidiSettings.currentMidiThruOutOption], Font_4x5, !(midiSettingSelected == i && isMidiSettingSelected));
            break;
        default:
            // Add default action if needed
            break;
        }
    }

    // CONTROL MIDI OUT/ THRU RELAY
    if (dubbyMidiSettings.currentMidiThruOutOption == MIDI_THRU)
    {
        SwitchMIDIOutThru(true);
    }

    if (dubbyMidiSettings.currentMidiThruOutOption == MIDI_OUT)
    {
        SwitchMIDIOutThru(false);
    }

    globalBPM = dubbyMidiSettings.currentBpm;

    display.Update();
}

void Dubby::UpdateParameterList(int increment)
{
    if (((parameterSelected > 0 && increment == 1 && parameterSelected < PARAMS_LAST - 2) || (increment != 1 && parameterSelected != 1)))
    {
        parameterSelected = (Params)(parameterSelected + increment);

        DisplayParameterList(increment);
    }
}

void Dubby::UpdateMidiSettingsList(int increment)
{

    // Check if increment is 1 (moving forward) and within bounds
    if ((increment == 1 && midiSettingSelected < MIDISETTINGS_LAST - 1) || (increment == -1 && midiSettingSelected > 0))
    {
        midiSettingSelected = static_cast<MidiSettings>(midiSettingSelected + increment);
        DisplayMidiSettingsList(increment);
    }
}

void Dubby::ResetToBootloader()
{
    DrawBitmap(1);
    display.WriteStringAligned("FIRMWARE UPDATE", Font_4x5, daisy::Rectangle(1, 35, 127, 45), daisy::Alignment::centered, true);

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

    dubbyCtrls[1].value = GetKnobValue(CTRL_1);
    dubbyCtrls[2].value = GetKnobValue(CTRL_2);
    dubbyCtrls[3].value = GetKnobValue(CTRL_3);
    dubbyCtrls[4].value = GetKnobValue(CTRL_4);
    dubbyCtrls[5].value = buttons[0].Pressed();
    dubbyCtrls[6].value = buttons[1].Pressed();
    dubbyCtrls[7].value = buttons[2].Pressed();
    dubbyCtrls[8].value = buttons[3].Pressed();
    dubbyCtrls[9].value = GetKnobValue(CTRL_5);
    dubbyCtrls[10].value = GetKnobValue(CTRL_6);
    dubbyCtrls[11].value = joystickButton.Pressed();

    for (int i = 0; i < PARAMS_LAST; i++)
        if (dubbyParameters[i].control != CONTROL_NONE)
            dubbyParameters[i].CalculateRealValue(dubbyCtrls[dubbyParameters[i].control].value);
}

void Dubby::ProcessAnalogControls()
{
    for (size_t i = 0; i < CTRL_LAST; i++)
        analogInputs[i].Process();
}

void Dubby::ProcessDigitalControls()
{
    encoder.Debounce();

    if (encoder.Pressed())
    {
        if (!isEncoderPressed)
        {
            // Encoder has just been pressed, record the start time
            isEncoderPressed = true;
            encoderPressStartTime = seed.system.GetNow();
        }
    }
    else
    {
        if (isEncoderPressed)
        {
            // Encoder has just been released
            isEncoderPressed = false;

            // Check if it was a short or long press
            unsigned long buttonPressDuration = seed.system.GetNow() - encoderPressStartTime;
            if (buttonPressDuration < ENCODER_LONGPRESS_THRESHOLD)
            {
                // Short press action
                wasEncoderLongPressed = false;
            }
            else
            {
                // Long press action
                wasEncoderLongPressed = true;
            }
        }
    }

    for (int i = 0; i < 4; i++)
        buttons[i].Debounce();

    joystickButton.Debounce();
}

float Dubby::GetKnobValue(Ctrl k)
{
    return (analogInputs[k].Value());
}

bool Dubby::EncoderFallingEdgeCustom()
{
    bool reading = encoder.Pressed(); // Read the encoder button state, assuming true is pressed

    if (reading != encoderLastState)
    {
        encoderLastDebounceTime = seed.system.GetNow();
    }

    if ((seed.system.GetNow() - encoderLastDebounceTime) > encoderDebounceDelay)
    {

        if (reading != encoderState)
        {
            encoderState = reading;

            if (reading)
            {
                std::string str = std::to_string(reading);
                UpdateStatusBar(&str[0], LEFT, 55);
            }

            if (encoderState)
            {
                std::string str = std::to_string(encoderState);
                UpdateStatusBar(&str[0], RIGHT, 55);
            }

            if (encoderState == true)
            { // Encoder button pressed

                UpdateStatusBar("TRUEEEE", MIDDLE, 127);
                return true;
            }
        }
    }

    encoderLastState = reading;

    return false;
}

void Dubby::InitAudio()
{
    // Handle Seed Audio as-is and then
    SaiHandle::Config sai_config[2];
    // Internal Codec
    if (seed.CheckBoardVersion() == DaisySeed::BoardVersion::DAISY_SEED_1_1)
    {
        sai_config[0].pin_config.sa = {DSY_GPIOE, 6};
        sai_config[0].pin_config.sb = {DSY_GPIOE, 3};
        sai_config[0].a_dir = SaiHandle::Config::Direction::RECEIVE;
        sai_config[0].b_dir = SaiHandle::Config::Direction::TRANSMIT;
    }
    else
    {
        sai_config[0].pin_config.sa = {DSY_GPIOE, 6};
        sai_config[0].pin_config.sb = {DSY_GPIOE, 3};
        sai_config[0].a_dir = SaiHandle::Config::Direction::TRANSMIT;
        sai_config[0].b_dir = SaiHandle::Config::Direction::RECEIVE;
    }
    sai_config[0].periph = SaiHandle::Config::Peripheral::SAI_1;
    sai_config[0].sr = SaiHandle::Config::SampleRate::SAI_48KHZ;
    sai_config[0].bit_depth = SaiHandle::Config::BitDepth::SAI_24BIT;
    sai_config[0].a_sync = SaiHandle::Config::Sync::MASTER;
    sai_config[0].b_sync = SaiHandle::Config::Sync::SLAVE;
    sai_config[0].pin_config.fs = {DSY_GPIOE, 4};
    sai_config[0].pin_config.mclk = {DSY_GPIOE, 2};
    sai_config[0].pin_config.sck = {DSY_GPIOE, 5};

    // External Codec

    I2CHandle::Config i2c_cfg;
    i2c_cfg.periph = I2CHandle::Config::Peripheral::I2C_1;
    i2c_cfg.mode = I2CHandle::Config::Mode::I2C_MASTER;
    i2c_cfg.speed = I2CHandle::Config::Speed::I2C_400KHZ;
    i2c_cfg.pin_config.scl = {DSY_GPIOB, 8};
    i2c_cfg.pin_config.sda = {DSY_GPIOB, 9};

    I2CHandle i2c2;
    i2c2.Init(i2c_cfg);

    // pullups must be enabled
    GPIOB->PUPDR &= ~((GPIO_PUPDR_PUPD8) | (GPIO_PUPDR_PUPD9));
    GPIOB->PUPDR |= ((GPIO_PUPDR_PUPD8_0) | (GPIO_PUPDR_PUPD9_0));

    Pcm3060 codec;

    codec.Init(i2c2);

    sai_config[1].periph = SaiHandle::Config::Peripheral::SAI_2;
    sai_config[1].sr = SaiHandle::Config::SampleRate::SAI_48KHZ;
    sai_config[1].bit_depth = SaiHandle::Config::BitDepth::SAI_24BIT;
    sai_config[1].a_sync = SaiHandle::Config::Sync::SLAVE;
    sai_config[1].b_sync = SaiHandle::Config::Sync::MASTER;
    sai_config[1].pin_config.fs = {DSY_GPIOG, 9};
    sai_config[1].pin_config.mclk = {DSY_GPIOA, 1};
    sai_config[1].pin_config.sck = {DSY_GPIOA, 2};
    sai_config[1].a_dir = SaiHandle::Config::Direction::TRANSMIT;
    sai_config[1].pin_config.sa = {DSY_GPIOD, 11};
    sai_config[1].b_dir = SaiHandle::Config::Direction::RECEIVE;
    sai_config[1].pin_config.sb = {DSY_GPIOA, 0};

    SaiHandle sai_handle[2];
    sai_handle[0].Init(sai_config[1]);
    sai_handle[1].Init(sai_config[0]);

    // Reinit Audio for _both_ codecs...
    AudioHandle::Config cfg;
    cfg.blocksize = 48;
    cfg.samplerate = SaiHandle::Config::SampleRate::SAI_48KHZ;
    cfg.postgain = 1.f;
    seed.audio_handle.Init(cfg, sai_handle[0], sai_handle[1]);
}

const char *Dubby::GetTextForEnum(EnumTypes m, int enumVal)
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