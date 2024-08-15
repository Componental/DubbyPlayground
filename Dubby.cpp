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

#define ENCODER_LONGPRESS_THRESHOLD 300

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

    // CONTROL 0
    dubbyCtrls[0][0].Init(CONTROL_NONE, 0);
    dubbyCtrls[0][1].Init(KN1, 0);
    dubbyCtrls[0][2].Init(KN2, 0);
    dubbyCtrls[0][3].Init(KN3, 0);
    dubbyCtrls[0][4].Init(KN4, 0);

    // CONTROL 1
    dubbyCtrls[1][0].Init(CONTROL_NONE, 0);
    dubbyCtrls[1][1].Init(KN1, 0);
    dubbyCtrls[1][2].Init(KN2, 0);
    dubbyCtrls[1][3].Init(KN3, 0);
    dubbyCtrls[1][4].Init(KN4, 0);

    // CONTROL 2
    dubbyCtrls[2][0].Init(CONTROL_NONE, 0);
    dubbyCtrls[2][1].Init(KN1, 0);
    dubbyCtrls[2][2].Init(KN2, 0);
    dubbyCtrls[2][3].Init(KN3, 0);
    dubbyCtrls[2][4].Init(KN4, 0);

    // CONTROL 3
    dubbyCtrls[3][0].Init(CONTROL_NONE, 0);
    dubbyCtrls[3][1].Init(KN1, 0);
    dubbyCtrls[3][2].Init(KN2, 0);
    dubbyCtrls[3][3].Init(KN3, 0);
    dubbyCtrls[3][4].Init(KN4, 0);

    // // CONTROL 1
    // dubbyCtrls[0][1].addParamValue(TIME);

    // // CONTROL 2
    // dubbyCtrls[1][1].addParamValue(FEEDBACK);
}

void Dubby::InitDubbyParameters()
{

    for (int i = 0; i < PARAMS_LAST - 1; i++)
    {
        if (i == 0)
            dubbyParameters[i].Init(PARAM_NONE, 0, 0, 1, LINEAR);
        else if (i == 1)
            dubbyParameters[i].Init(Params(i), 0, 0, 1, LOGARITHMIC);
        else if (i == 2)
            dubbyParameters[i].Init(Params(i), 0, 0, 0.5, EXPONENTIAL);
        else
            dubbyParameters[i].Init(Params(i), 0.6, 0, 1, SIGMOID);
    }
}

DubbyControls Dubby::GetParameterControl(Params p)
// For each combination of page, ctrl, and j,
// the function checks if the given parameter p matches any of the parameters associated
// with the control dubbyCtrls[page][ctrl].param[j]. If a match is found, the function returns
// the control (dubbyCtrls[page][ctrl].control) associated with that parameter.
// If no matching control is found after all iterations, the function returns CONTROL_NONE,
// indicating that the parameter p is not linked to any control.

{
    for (int page = 0; page < NUM_PAGES; page++)
    {
        for (int ctrl = 0; ctrl < CONTROLS_LAST; ctrl++)
        {
            for (int j = 0; j < PARAMS_LAST; j++)
            {
                if (p == dubbyCtrls[page][ctrl].param[j])
                    return dubbyCtrls[page][ctrl].control;
            }
        }
    }

    return CONTROL_NONE;
}

float Dubby::GetParameterValue(Parameters p)
// For each parameter associated with a control, it checks if the parameter in question (p.param)
// matches any of the parameters in the dubbyCtrls[page][ctrl].param[j].
// If a match is found, the function retrieves the control's value using dubbyCtrls[page][ctrl].value.
// It then computes the "real" value of the parameter using the method p.GetRealValue() and returns this computed value.
// If no matching parameter is found, the function returns the default value of p (p.value).

{
    for (int page = 0; page < NUM_PAGES; page++)
    {
        for (int ctrl = 0; ctrl < CONTROLS_LAST; ctrl++)
        {
            for (int j = 0; j < PARAMS_LAST; j++)
            {
                if (p.param == dubbyCtrls[page][ctrl].param[j]) // these should stay like this and not be assigned to the select param
                    return p.GetRealValue(dubbyCtrls[page][ctrl].value);
            }
        }
    }

    return p.value;
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
    // Check if the encoder has been held down longer than the long press threshold
    // and if the window selector is not already active
    if (encoder.TimeHeldMs() > ENCODER_LONGPRESS_THRESHOLD && !windowSelectorActive)
    {
        windowSelectorActive = true; // Activate the window selector
    }

    if (windowSelectorActive)
    {
        HighlightWindowItem(); // Highlight the currently selected window item

        // If the encoder has been incremented, update the window selector
        if (encoder.Increment())
            UpdateWindowSelector(encoder.Increment(), true);

        // If the encoder has a rising edge (button press), deactivate the window selector
        if (encoder.RisingEdge())
        {
            windowSelectorActive = false;
            ReleaseWindowSelector(); // Release the window selector
            UpdateWindowList();      // Update the window list
        }

        // If the encoder has a falling edge and wasn't just in the highlight menu
        if (!wasEncoderJustInHighlightMenu && encoder.FallingEdge())
            wasEncoderJustInHighlightMenu = true; // Set flag to indicate it was in highlight menu
    }

    // Handle the case where the encoder was just in the highlight menu and now has a falling edge
    if (wasEncoderJustInHighlightMenu && encoder.FallingEdge())
    {
        if (highlightMenuCounter < 2)
        {
            highlightMenuCounter++; // Increment the highlight menu counter
        }
        else
        {
            wasEncoderJustInHighlightMenu = false; // Reset flag
            highlightMenuCounter = 0;              // Reset counter
        }
    }

    // Switch case to handle different window items selected
    switch (windowItemSelected)
    {
    case WIN1:
        UpdateKnobWindow(customLabels, numDecimals);
        break;
    case WIN2:
        UpdateRenderPane(); // Update the render pane for window 1
        break;
    case WIN3:
        UpdateMixerPane(); // Update the mixer pane for window 2
        break;
    case WIN4:
        // Handle the case where the encoder has a falling edge and no submenu is active
        if (encoder.FallingEdge() && !isSubMenuActive && !wasEncoderJustInHighlightMenu)
        {
            isSubMenuActive = true;
            DisplayPreferencesMenuList(0); // Display the preferences menu
        }

        // If the window selector is active, deactivate the submenu
        if (windowSelectorActive)
        {
            isSubMenuActive = false;
            DisplayPreferencesMenuList(0); // Display the preferences menu
        }

        // Display the preferences submenu list with the encoder increment
        DisplayPreferencesSubMenuList(encoder.Increment(), preferencesMenuItemSelected);
        // If the encoder has a falling edge and the selected menu item is DFUMODE, reset to bootloader
        if (encoder.FallingEdge() && !wasEncoderJustInHighlightMenu && preferencesMenuItemSelected == DFUMODE)
            ResetToBootloader();
        // Update the preferences menu or submenu based on encoder increment
        if (encoder.Increment() && !windowSelectorActive && !isSubMenuActive)
            UpdatePreferencesMenuList(encoder.Increment());
        else if (encoder.Increment() && !windowSelectorActive && isSubMenuActive)
            UpdatePreferencesSubMenuList(encoder.Increment(), preferencesMenuItemSelected);
        break;
    case WIN5:
        // Display the parameter list with the encoder increment
        DisplayParameterList(encoder.Increment());

        // Update the parameter list if conditions are met
        if (encoder.Increment() && !isEncoderIncrementDisabled && !windowSelectorActive && !isParameterSelected)
            UpdateParameterList(encoder.Increment());

        // Handle parameter selection
        if (encoder.FallingEdge() && !wasEncoderJustInHighlightMenu && !windowSelectorActive && !isParameterSelected)
        {
            isParameterSelected = true;
            parameterOptionSelected = PARAM;
            DisplayParameterList(encoder.Increment());
        }

        // Handle control change listening
        if (isListeningControlChange)
        {

            // If the system is currently listening for control changes
            for (int i = 0; i < CONTROLS_LAST; i++)
            {
                // Loop through all controls up to the constant CONTROLS_LAST

                // Check if the control value has changed significantly
                if (abs(dubbyCtrls[currentPage][i].tempValue -
                        dubbyCtrls[dubbyParameters[parameterSelected].page][i].value) > 0.1f)
                {
                    // If the absolute difference between the temporary value and the actual value
                    // of the control at the current page and index is greater than 0.1f,
                    // it means the control value has changed significantly

                    // Remove the parameter value associated with the previously selected control
                    dubbyCtrls[dubbyParameters[parameterSelected].page][prevControl].removeParamValue(dubbyParameters[parameterSelected].param);

                    // Reset the previous control to indicate no control is currently selected
                    prevControl = CONTROL_NONE;

                    // Add the parameter value to the newly selected control
                    dubbyCtrls[dubbyParameters[parameterSelected].page][i].addParamValue(dubbyParameters[parameterSelected].param);

                    // Update the parameter to reflect the current page
                    dubbyParameters[parameterSelected].setPage(currentPage);

                    // Refresh the parameter display, starting from the first parameter
                    DisplayParameterList(0);

                    // Update the status bar with a message indicating the value is being adjusted
                    fullParameterStatusbar = parameterWindowStatusbarBase + "VALUE";
                    UpdateStatusBar(&fullParameterStatusbar[0], LEFT);

                    // Stop listening for further control changes
                    isListeningControlChange = false;

                    // Re-enable encoder increments, assuming they were disabled during this process
                    isEncoderIncrementDisabled = false;
                }
            }
        }

        // Handle page changing
        else if (isPageChanging)
        {
            if (encoder.Increment())
            {
                // Get current page and encoder increment
                int currentPage = static_cast<int>(dubbyParameters[parameterSelected].page);
                int increment = encoder.Increment();

                // Update page value considering the bounds
                currentPage += increment;
                if (currentPage >= NUM_PAGES + 1) // Wrap around if exceeding the maximum page. +1 because of ALL option
                {
                    currentPage = 0; // or any other logic you want for wrapping
                }
                else if (currentPage < 0) // Wrap around if going below the minimum page
                {
                    currentPage = NUM_PAGES; // or any other logic you want for wrapping. not -1 because of ALL option
                }

                // Set the page value
                dubbyParameters[parameterSelected].setPage(static_cast<Pages>(currentPage));
            }
            if (EncoderFallingEdgeCustom())
            {
                isPageChanging = false;
                isEncoderIncrementDisabled = false;
                fullParameterStatusbar = parameterWindowStatusbarBase + "VALUE     ";
                UpdateStatusBar(&fullParameterStatusbar[0], LEFT);
            }
        }
        // Handle curve changing
        else if (isCurveChanging)
        {
            if (encoder.Increment())
            {
                // Cycle through curve options based on encoder increment
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
                fullParameterStatusbar = parameterWindowStatusbarBase + "CURVE";
                UpdateStatusBar(&fullParameterStatusbar[0], LEFT);
            }
        }
        // Handle minimum value changing
        else if (isMinChanging)
        {
            if (encoder.Increment())
            {
                dubbyParameters[parameterSelected].min += encoder.Increment();
            }
            if (EncoderFallingEdgeCustom())
            {
                isMinChanging = false;
                isEncoderIncrementDisabled = false;
                fullParameterStatusbar = parameterWindowStatusbarBase + "MIN    ";
                UpdateStatusBar(&fullParameterStatusbar[0], LEFT);
            }
        }
        // Handle maximum value changing
        else if (isMaxChanging)
        {
            if (encoder.Increment())
            {
                dubbyParameters[parameterSelected].max += encoder.Increment();
            }
            if (EncoderFallingEdgeCustom())
            {
                isMaxChanging = false;
                isEncoderIncrementDisabled = false;
                fullParameterStatusbar = parameterWindowStatusbarBase + "MAX    ";

                UpdateStatusBar(&fullParameterStatusbar[0], LEFT);
            }
        }
        // Handle parameter value changing
        else if (isValueChanging)
        {
            if (encoder.Increment())
            {
                dubbyParameters[parameterSelected].value += encoder.Increment();
            }
            if (EncoderFallingEdgeCustom())
            {
                isValueChanging = false;
                isEncoderIncrementDisabled = false;
                fullParameterStatusbar = parameterWindowStatusbarBase + "VALUE";

                UpdateStatusBar(&fullParameterStatusbar[0], LEFT);
            }
        }

        // Handle parameter option selection
        if (isParameterSelected)
        {

            if (encoder.Increment() && !isEncoderIncrementDisabled)
            {
                // Cycle through parameter options based on encoder increment
                parameterOptionSelected = static_cast<ParameterOptions>(static_cast<int>(parameterOptionSelected) + encoder.Increment());
                if (parameterOptionSelected < PARAM || parameterOptionSelected >= POPTIONS_LAST)
                {
                    isParameterSelected = false;
                }

                // Update status bar based on selected parameter option
                switch (parameterOptionSelected)
                {
                case MIN:
                    fullParameterStatusbar = parameterWindowStatusbarBase + "MIN    ";
                    UpdateStatusBar(&fullParameterStatusbar[0], LEFT);
                    break;
                case MAX:
                    fullParameterStatusbar = parameterWindowStatusbarBase + "MAX    ";
                    UpdateStatusBar(&fullParameterStatusbar[0], LEFT);
                    break;
                case CURVE:
                    fullParameterStatusbar = parameterWindowStatusbarBase + "CURVE";
                    UpdateStatusBar(&fullParameterStatusbar[0], LEFT);
                    break;
                default:
                    fullParameterStatusbar = parameterWindowStatusbarBase + "VALUE";
                    UpdateStatusBar(&fullParameterStatusbar[0], LEFT);
                    break;
                }

                DisplayParameterList(encoder.Increment());
            }
            // Handle control selection
            else if (encoder.FallingEdge() && parameterOptionSelected == CTRL)
            {
                // If the encoder has been pressed (FallingEdge triggered) and the selected parameter option is CTRL,
                // proceed to handle control selection

                UpdateStatusBar("SELECT A CONTROL", MIDDLE, 127);

                // Set the system to listen for control changes
                isListeningControlChange = true;

                // Disable encoder increments to avoid interfering with the control selection process
                isEncoderIncrementDisabled = true;

                // Prepare for detecting control changes
                    for (int i = 0; i < CONTROLS_LAST; i++)
                    {
                        // Loop through all controls up to the constant CONTROLS_LAST

                        // Store the current value of each control into its temporary value
                        dubbyCtrls[dubbyParameters[parameterSelected].page][i].tempValue = dubbyCtrls[dubbyParameters[parameterSelected].page][i].value;

                        // Check if the current parameter is already associated with any control
                        for (int k = 0; k < PARAMS_LAST; k++)
                        {
                            // If the current parameter (parameterSelected) is found in the list of parameters for the current control
                            if (dubbyCtrls[dubbyParameters[parameterSelected].page][i].param[k] == parameterSelected)
                                // Set prevControl to the current control's ID
                                prevControl = dubbyCtrls[dubbyParameters[parameterSelected].page][i].control;
                        }
                    }
                }
            

            // Handle page selection
            else if (parameterOptionSelected == PAGE)
            {
                if (EncoderFallingEdgeCustom())
                {
                    UpdateStatusBar("SELECT A PAGE", MIDDLE, 127);
                    isEncoderIncrementDisabled = true;
                    isPageChanging = true;
                }
            }
            // Handle curve selection
            else if (parameterOptionSelected == CURVE)
            {
                if (EncoderFallingEdgeCustom())
                {
                    UpdateStatusBar("SELECT A CURVE", MIDDLE, 127);
                    isEncoderIncrementDisabled = true;
                    isCurveChanging = true;
                }
            }
            // Handle minimum value selection
            else if (parameterOptionSelected == MIN)
            {
                if (EncoderFallingEdgeCustom())
                {
                    UpdateStatusBar("SELECT MIN VALUE", MIDDLE, 127);
                    isEncoderIncrementDisabled = true;
                    isMinChanging = true;
                }
            }
            // Handle maximum value selection
            else if (parameterOptionSelected == MAX)
            {
                if (EncoderFallingEdgeCustom())
                {
                    UpdateStatusBar("SELECT MAX VALUE", MIDDLE, 127);
                    isEncoderIncrementDisabled = true;
                    isMaxChanging = true;
                }
            }
            // Handle value selection
            else if (parameterOptionSelected == VALUE && GetParameterControl(dubbyParameters[parameterSelected].param) == CONTROL_NONE)
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
    //      display.DrawRect(PANE_X_START, PANE_Y_START, PANE_X_END, PANE_Y_END, false, true);
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
        //   display.SetCursor(10, 15);
        //   UpdateStatusBar(&algorithmTitle[0], LEFT);
        UpdateKnobWindow(customLabels, numDecimals);
        break;
    case WIN2:
        statusStr = GetTextForEnum(SCOPE, scopeSelector);
        UpdateStatusBar(&statusStr[0], LEFT, 70);
        UpdateRenderPane();
        break;
    case WIN3:
        for (int i = 0; i < 4; i++)
            UpdateBar(i);
        break;
    case WIN4:
        DisplayPreferencesMenuList(0);
        break;
    case WIN5:
        fullParameterStatusbar = parameterWindowStatusbarBase + "VALUE";
        UpdateStatusBar(&fullParameterStatusbar[0], LEFT);
        display.DrawLine(6, 7, 127, 7, true);

        DisplayParameterList(0);

        break;
    case WIN6:
        display.SetCursor(10, 15);
        UpdateStatusBar("PANE 6", LEFT);
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

void Dubby::UpdateKnobWindow(const std::vector<std::string> &knobLabels, const std::vector<int> &numDecimals)
{
    display.DrawRect(PANE_X_START, PANE_Y_START, PANE_X_END, PANE_Y_END, false, true);

    // Define parameters for circular knobs
    int circle_y = 34;     // Y-coordinate of the center of the circle
    int circle_radius = 8; // Radius of the circle

    // Calculate total width occupied by circles
    int totalWidth = NUM_KNOBS * 2 * circle_radius;

    // Calculate space between circles
    int circleSpacing = (OLED_WIDTH - totalWidth) / (NUM_KNOBS + 1);

    // Loop through each knob value
    for (int i = 0; i < NUM_KNOBS; ++i)
    {
        // Calculate knob x-coordinate
        int circle_x_offset = circleSpacing * (i + 1) + circle_radius + i * 2 * circle_radius;

        // Get knob value
        float knobValue = savedKnobValuesForVisuals[i];

        float knobValueLive = GetKnobValue(static_cast<Ctrl>(i));
        // Calculate angle for the current knob
        float angle = (knobValue * 0.8f * 2 * PI_F) - (PI_F * 1.5f) + 0.2 * PI_F; // Convert knob value to angle

        // Calculate angle for knobValueLive
        float liveAngle = (knobValueLive * 0.8f * 2 * PI_F) - (PI_F * 1.5f) + 0.2 * PI_F;

        // Calculate line end position based on knob value
        int line_end_x = circle_x_offset + static_cast<int>(circle_radius * cos(angle));
        int line_end_y = circle_y + static_cast<int>(circle_radius * sin(angle));

        int arc_radius = circle_radius - 1; // Define a smaller radius for the arc

        // Calculate start and end angles for the arc
        float start_arc_angle = -angle;   // Start angle is the same as angle
        float end_arc_angle = -liveAngle; // End angle is the same as liveAngle

        // Draw arc indicating knob value difference
        display.DrawArc(circle_x_offset, circle_y, arc_radius,
                        static_cast<int>((start_arc_angle) * 180.0f / PI_F),                 // Convert to degrees
                        static_cast<int>((end_arc_angle - start_arc_angle) * 180.0f / PI_F), // Sweep angle
                        true);

        // Draw circular knob
        display.DrawCircle(circle_x_offset, circle_y, circle_radius, true);

        // Draw line indicating knob value
        display.DrawLine(circle_x_offset, circle_y, line_end_x, line_end_y, true);

        // Calculate the position for the label to be centered above the circle
        int label_x = circle_x_offset - (knobLabels[i].size() * 4) / 2; // Assuming each character is 4 pixels wide in the selected font
        int label_y = circle_y - 20;                                    // Adjust this value to position the label properly above the circle

        // Draw custom label above each circle
        display.SetCursor(label_x, label_y);
        display.WriteString(knobLabels[i].c_str(), Font_4x5, true);

        // Format knob value as string
        char formattedValue[10];
        snprintf(formattedValue, 10, "%.*f", numDecimals[i], knobValuesForPrint[i]);

        // Calculate the position for the value to be centered under the circle
        int value_width = strlen(formattedValue) * 4; // Assuming each character is 4 pixels wide in the selected font
        int value_x = circle_x_offset - value_width / 2;

        // Draw knob value below the label
        display.SetCursor(value_x, circle_y + 15);
        display.WriteString(formattedValue, Font_4x5, true);
    }
}

void Dubby::UpdateAlgorithmTitle()
{
    UpdateStatusBar(&algorithmTitle[0], LEFT);
}

void Dubby::UpdateKnobValues(const std::vector<float> &values)
{
    knobValuesForPrint.clear(); // Clear the existing values
    knobValuesForPrint.insert(knobValuesForPrint.end(), values.begin(), values.end());
}

void Dubby::SetCurrentPage(int page)
{
    currentPage = page + 1;
}

void Dubby::DisplayParameterList(int increment)
{
    // clear bounding box
    // display.DrawRect(PANE_X_START - 1, 1, PANE_X_END, PANE_Y_END, false, true);
    int ctrlColumnPos = 43;
    int valMinMaxCurveColumnPos = 99;
    int pageColumnPos = 71;

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
                    x = ctrlColumnPos;
                    break;
                case PAGE: // handle PAGE selection
                    x = pageColumnPos;
                    break;
                case VALUE:
                case MIN:
                case MAX:
                case CURVE:
                    x = valMinMaxCurveColumnPos;
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

        display.SetCursor(ctrlColumnPos + 2, PARAMLIST_Y_START + 1 + (j * PARAMLIST_SPACING));
        display.WriteString(ControlsStrings[GetParameterControl(dubbyParameters[i].param)], Font_4x5, !(parameterSelected == i && isParameterSelected));

        std::string pageStr = PagesStrings[dubbyParameters[i].page];
        display.SetCursor(pageColumnPos + 2, PARAMLIST_Y_START + 1 + (j * PARAMLIST_SPACING));
        display.WriteString(pageStr.c_str(), Font_4x5, !(parameterSelected == i && isParameterSelected));

        std::string str = std::to_string(GetParameterValue(dubbyParameters[i])).substr(0, std::to_string(GetParameterValue(dubbyParameters[i])).find(".") + 3);
        switch (parameterOptionSelected)
        {
            break;
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
            str = std::to_string(GetParameterValue(dubbyParameters[i])).substr(0, std::to_string(GetParameterValue(dubbyParameters[i])).find(".") + 3);
            // UpdateStatusBar(&fullParameterStatusbar[0], LEFT); %VALUE   ", LEFT);
            break;
        }

        display.SetCursor(valMinMaxCurveColumnPos + 2, PARAMLIST_Y_START + 1 + (j * PARAMLIST_SPACING));
        display.WriteString(&str[0], Font_4x5, !(parameterSelected == i && isParameterSelected));
    }

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

    // dubbyCtrls[0][1].value = GetKnobValue(CTRL_1);
    // dubbyCtrls[0][2].value = GetKnobValue(CTRL_2);
    // dubbyCtrls[0][3].value = GetKnobValue(CTRL_3);
    // dubbyCtrls[0][4].value = GetKnobValue(CTRL_4);
    // page0

    dubbyCtrls[0][1].value = getKnobValueMatrix[0][0];
    dubbyCtrls[0][2].value = getKnobValueMatrix[0][1];
    dubbyCtrls[0][3].value = getKnobValueMatrix[0][2];
    dubbyCtrls[0][4].value = getKnobValueMatrix[0][3];

    // page1

    dubbyCtrls[1][1].value = getKnobValueMatrix[1][0];
    dubbyCtrls[1][2].value = getKnobValueMatrix[1][1];
    dubbyCtrls[1][3].value = getKnobValueMatrix[1][2];
    dubbyCtrls[1][4].value = getKnobValueMatrix[1][3];

    // page2
    dubbyCtrls[2][1].value = getKnobValueMatrix[2][0];
    dubbyCtrls[2][2].value = getKnobValueMatrix[2][1];
    dubbyCtrls[2][3].value = getKnobValueMatrix[2][2];
    dubbyCtrls[2][4].value = getKnobValueMatrix[2][3];

    // page3
    dubbyCtrls[3][1].value = getKnobValueMatrix[3][0];
    dubbyCtrls[3][2].value = getKnobValueMatrix[3][1];
    dubbyCtrls[3][3].value = getKnobValueMatrix[3][2];
    dubbyCtrls[3][4].value = getKnobValueMatrix[3][3];

    // for loop for shorter code, later

    // for (int page = 0; page < NUM_PAGES; ++page) {
    //     for (int controlIndex = 1; controlIndex <= 4; ++controlIndex) {
    //         dubbyCtrls[page][controlIndex].value = getKnobValueMatrix[page][controlIndex - 1];
    //     }
    // }
    dubbyCtrls[0][5].value = buttons[0].Pressed();
    dubbyCtrls[0][6].value = buttons[1].Pressed();
    dubbyCtrls[0][7].value = buttons[2].Pressed();
    dubbyCtrls[0][8].value = buttons[3].Pressed();
    dubbyCtrls[0][9].value = GetKnobValue(CTRL_5);
    dubbyCtrls[0][10].value = GetKnobValue(CTRL_6);
    dubbyCtrls[0][11].value = joystickButton.Pressed();
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