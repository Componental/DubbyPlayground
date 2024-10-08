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
#define MENULIST_Y_START 0
#define MENULIST_Y_END 8
#define MENULIST_SPACING 8
#define MENULIST_SUBMENU_SPACING 63
#define MENULIST_ROWS_ON_SCREEN 7

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

#define MODAL_X_START 10
#define MODAL_X_END 117
#define MODAL_Y_START 5
#define MODAL_Y_END 54

#define MODAL_LEFT_OPTION_X_START 25
#define MODAL_LEFT_OPTION_Y_START 35
#define MODAL_RIGHT_OPTION_X_START 70
#define MODAL_RIGHT_OPTION_Y_START 35
#define MODAL_OPTION_WIDTH 33
#define MODAL_OPTION_HEIGHT 12

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

        if (i == PARAMLIST_ROWS_ON_SCREEN - 1)
            paramListBoxBounding[i][3] = (PARAMLIST_Y_END + i * PARAMLIST_SPACING) - 2;
    }

    for (int i = 0; i < MIDILIST_ROWS_ON_SCREEN; i++)
    {
        midiListBoxBounding[i][0] = MIDILIST_X_START;
        midiListBoxBounding[i][1] = MIDILIST_Y_START + i * MIDILIST_SPACING;
        midiListBoxBounding[i][2] = MIDILIST_X_END;
        midiListBoxBounding[i][3] = MIDILIST_Y_END + i * MIDILIST_SPACING;
    }

    scrollbarWidth = int(128 / WIN_LAST);

    InitLEDs();
    InitDisplay();
    InitLEDs();
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

void Dubby::InitLEDs()
{
    initLED();
    setLED(1, NO_COLOR, 0);
    setLED(0, NO_COLOR, 0);
    updateLED();
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

    dubbyParameters[TIME].Init(Params(TIME), KN1, 110.f, 0, 440, LINEAR, true, 0, true, 2000);
    dubbyParameters[FEEDBACK].Init(Params(FEEDBACK), KN2, 0.6f, 0, 1, LINEAR, true, 0, true, 1);
    dubbyParameters[MIX].Init(Params(MIX), KN3, 0.5f, 0, 1, EXPONENTIAL, true, 0, true, 100);
    dubbyParameters[CUTOFF].Init(Params(CUTOFF), JSY, 5.f, 0, 10, EXPONENTIAL, true, 0, true, 10);
    dubbyParameters[IN_GAIN].Init(Params(IN_GAIN), BTN1, 0.5f, 0, 5, LINEAR, true, 0, true, 5);
    dubbyParameters[OUT_GAIN].Init(Params(OUT_GAIN), BTN2, 0.5f, 0, 5, LINEAR, true, 0, true, 5);
    dubbyParameters[FREEZE].Init(Params(FREEZE), BTN3, 0.5f, 0, 5, LINEAR, true, 0, true, 5);
    dubbyParameters[MUTE].Init(Params(MUTE), BTN4, 0.5f, 0, 5, LINEAR, true, 0, true, 5);
    dubbyParameters[LOOP].Init(Params(LOOP), JSX, 0.5f, 0, 5, LINEAR, true, 0, true, 5);
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
    if (!isModalActive)
    {
        if (encoder.TimeHeldMs() > ENCODER_LONGPRESS_THRESHOLD && !windowSelectorActive)
        {
            windowSelectorActive = true;

            isParameterSelected = false;
            isListeningControlChange = false;
            isValueChanging = false;
            isMinChanging = false;
            isMaxChanging = false;
            isCurveChanging = false;
            parameterOptionSelected = PARAM;

            isEncoderIncrementDisabled = false;

            encoder.EnableAcceleration(false);
        }

        if (windowSelectorActive)
        {
            HighlightWindowItem();
            if (encoder.Increment())
                UpdateWindowSelector(encoder.Increment(), true);

            if (encoder.FallingEdgeCustom())
            {
                windowSelectorActive = false;
                ReleaseWindowSelector();
                UpdateWindowList();

                wasEncoderJustInHighlightMenu = true;
                encoderLastDebounceTime2 = seed.system.GetNow();
            }

            if (!wasEncoderJustInHighlightMenu && encoder.FallingEdgeCustom())
            {
                wasEncoderJustInHighlightMenu = true;
                encoderLastDebounceTime2 = seed.system.GetNow();
            }
        }

        if (wasEncoderJustInHighlightMenu && (encoder.FallingEdgeCustom() || encoder.FallingEdge()))
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
            UpdateCurrentMappingWindow();
            break;
        case WIN2:
            UpdateLFOWindow();
            break;
        case WIN3:
            UpdateParameterPane();
            break;
        case WIN4:
            UpdateRenderPane();
            break;
        case WIN5:
            UpdateChannelMappingPane();
            break;
        case WIN6:
            UpdateMidiSettingsPane();
            break;
        case WIN7:
            UpdateGlobalSettingsPane();
            break;
        default:
            UpdateCurrentMappingWindow();
            break;
        }
    }
    else
    {
        if ((encoder.Increment() == 1 && modalOptionSelected == 0) || (encoder.Increment() == -1 && modalOptionSelected == 1))
        {
            ChangeModalOption();
        }
        else if (encoder.RisingEdgeCustom())
        {
            if (modalOptionSelected == YES)
            {
                switch (preferencesMenuItemSelected)
                {
                case DFUMODE:
                    ResetToBootloader();
                    break;
                case SAVEMEMORY:
                    trigger_save_parameters_qspi = true;
                    break;
                case RESETMEMORY:
                    trigger_reset_parameters_qspi = true;
                    break;
                default:
                    break;
                }
            }

            CloseModal();
        }
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

    // display.DrawLine(PANE_X_START - 1, PANE_Y_START + 1, PANE_X_START - 1, PANE_Y_END + 1, true);

    display.DrawLine(PANE_X_START - 1, PANE_Y_END + 1, PANE_X_END - 1, PANE_Y_END + 1, true);

    display.DrawLine(windowItemSelected * scrollbarWidth, 63, (windowItemSelected * scrollbarWidth) + scrollbarWidth, 63, true);

    display.Update();
}

void Dubby::ReleaseWindowSelector()
{
    //  ClearPane();

    display.DrawRect(PANE_X_START - 1, PANE_Y_END, PANE_X_END, PANE_Y_END + 13, false, true);

    display.DrawRect(windowBoxBounding[0][0], windowBoxBounding[0][1], windowBoxBounding[0][2], windowBoxBounding[0][3], false, false);

    display.SetCursor(windowTextCursors[0][0], windowTextCursors[0][1]);

    display.WriteStringAligned(GetTextForEnum(WINDOWS, windowItemSelected), Font_4x5, daisy::Rectangle(windowBoxBounding[0][0], windowBoxBounding[0][1] + 1, 43, 7), daisy::Alignment::centeredLeft, true);

    display.Update();
}

void Dubby::ClearPane()
{
    display.DrawRect(PANE_X_START, PANE_Y_START, PANE_X_END, PANE_Y_END, false, true);
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
            encoder.EnableAcceleration(true);
            std::string str = (mixerPageSelected == INPUTS ? "in" : "out") + std::to_string(barSelector % 4 + 1) + ":" + std::to_string(audioGains[mixerPageSelected][barSelector % 4]).substr(0, std::to_string(audioGains[mixerPageSelected][barSelector % 4]).find(".") + 3);
            UpdateStatusBar(&str[0], RIGHT);
        }
        else
        {
            encoder.EnableAcceleration(false);
            UpdateStatusBar(" ", RIGHT);
        }
    }

    if (isBarSelected)
    {
        if ((increment > 0 && audioGains[mixerPageSelected][barSelector % 4] < 1.0f) || (increment < 0 && audioGains[mixerPageSelected][barSelector % 4] > 0.0001f))
        {
            audioGains[mixerPageSelected][barSelector % 4] += increment / 100.f;

            if (audioGains[mixerPageSelected][barSelector % 4] > 1.0f)
                audioGains[mixerPageSelected][barSelector % 4] = 1.0f;
            else if (audioGains[mixerPageSelected][barSelector % 4] < 0.0f)
                audioGains[mixerPageSelected][barSelector % 4] = 0.0f;

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
        UpdateCurrentMappingWindow();
        break;
    case WIN2:
        UpdateLFOWindow();
        break;
    case WIN3:
        UpdateStatusBar(" PARAM       CTRL      VALUE  >", LEFT);
        display.DrawLine(6, 7, 127, 7, true);
        DisplayParameterList(0);
        break;
    case WIN4:
        statusStr = GetTextForEnum(SCOPE, scopeSelector);
        UpdateStatusBar(&statusStr[0], LEFT, 70);
        UpdateRenderPane();
        break;
    case WIN5:
        UpdateChannelMappingPane();
        break;
    case WIN6:
        UpdateStatusBar(" SETTING              VALUE    ", LEFT);
        display.DrawLine(6, 10, 121, 10, true);
        DisplayMidiSettingsList(0);
        break;
    case WIN7:
        DisplayPreferencesMenuList(preferencesMenuItemSelected);
        break;
    default:
        break;
    }

    display.Update();
}

void Dubby::UpdateChannelMappingPane()
{

    // Define dimensions for each cell in the grid
    const int cellWidth = 23; // Width of each cell in the grid
    const int cellHeight = 8; // Height of each cell in the grid

    // Calculate the overall width and height of the grid
    const int gridWidth = numCols * cellWidth;   // Total width of the grid
    const int gridHeight = numRows * cellHeight; // Total height of the grid

    // Calculate the starting coordinates to center the grid in a 128x64 display
    const int startX = ((128 - gridWidth) / 2) + 4; // X-coordinate of the top-left corner of the grid
    const int startY = ((64 - gridHeight) / 2) + 2; // Y-coordinate of the top-left corner of the grid

    // Define labels for rows and columns
    const char *rowLabels[numRows] = {"IN1", "IN2", "IN3", "IN4"};     // Row labels displayed above each row
    const char *colLabels[numCols] = {"OUT1", "OUT2", "OUT3", "OUT4"}; // Column labels displayed to the left of each column

    // Get the current increment value from the encoder
    int increment = encoder.Increment(); // Encoder increment value

    // Static variables to keep track of the current position and mode
    static int currentRow = 0;             // Current selected row
    static int currentCol = 0;             // Current selected column
    static bool selectJunctionMode = true; // Flag to toggle between index mode and grid mode

    // Toggle mode when the encoder is pressed
    if (encoder.FallingEdge() && !wasEncoderJustInHighlightMenu && !windowSelectorActive)
    {
        selectJunctionMode = !selectJunctionMode; // Toggle between selectJunctionMode and grid navigation mode
    }

    if (selectJunctionMode)
    {
        // Display status bar message for select index mode
        UpdateStatusBar("SELECT AUDIO JUNCTION   ", LEFT);

        // Update row and column based on the encoder increment
        if (increment != 0 && !windowSelectorActive)
        {
            // Adjust the increment direction for smooth navigation
            if (increment > 0)
            {
                currentCol++;
                if (currentCol >= numCols)
                {
                    currentCol = 0;
                    currentRow++;
                    if (currentRow >= numRows)
                    {
                        currentRow = 0;
                    }
                }
            }
            else
            {
                currentCol--;
                if (currentCol < 0)
                {
                    currentCol = numCols - 1;
                    currentRow--;
                    if (currentRow < 0)
                    {
                        currentRow = numRows - 1;
                    }
                }
            }
        }
    }
    else
    {
        // Display status bar message for grid navigation mode
        UpdateStatusBar("* ASSIGN AUDIO ROUTING *", LEFT);

        // Update the channel mapping value at the selected row and column
        if (increment != 0 && !windowSelectorActive)
        {
            // Determine the valid range for channel mappings

            // Determine the direction of navigation (forward or backward)
            int direction = (increment > 0) ? 1 : -1;

            // Initialize the current mapping to the existing value
            int currentMapping = channelMapping[currentRow][currentCol];

            // Calculate the next potential mapping in the desired direction
            int nextMapping = (currentMapping + direction + CHANNELMAPPINGS_LAST) % CHANNELMAPPINGS_LAST;

            // Loop until a valid mapping is found or we return to the original position
            while (nextMapping != currentMapping)
            {
                currentMapping = nextMapping;
                nextMapping = (currentMapping + direction + CHANNELMAPPINGS_LAST) % CHANNELMAPPINGS_LAST;

                // Check if the current mapping is valid (exists and has corresponding bool)
                if ((currentMapping == NONE && dubbyChannelMapping->hasNone) ||
                    (currentMapping == PASS && dubbyChannelMapping->hasPass) ||
                    (currentMapping == EFCT && dubbyChannelMapping->hasEfct) ||
                    (currentMapping == SNTH && dubbyChannelMapping->hasSynth))
                {
                    break; // Valid mapping found, exit loop
                }
            }

            // Update the channel mapping with the validated value
            channelMapping[currentRow][currentCol] = currentMapping;
        }
    }

    // Display row labels above the grid
    for (int row = 0; row < numRows; row++)
    {
        int labelX = startX + 5 + (cellWidth * row); // X-coordinate for the row label
        int labelY = startY - 6;                     // Y-coordinate for the row label
        display.SetCursor(labelX, labelY);
        display.WriteString(rowLabels[row], Font_4x5, true); // Display row label

        // Draw horizontal line under the row labels
        int lineStartX = startX + 2;
        int lineEndX = lineStartX + (cellWidth * 4) - 5;
        int lineY = startY;
        display.DrawLine(lineStartX, lineY, lineEndX, lineY, true); // Draw a horizontal line
    }

    // Display column labels to the left of the grid
    for (int col = 0; col < numCols; col++)
    {
        int labelX = startX - 16;                     // X-coordinate for the column label
        int labelY = startY + (cellHeight * col) + 3; // Y-coordinate for the column label
        display.SetCursor(labelX, labelY);
        display.WriteString(colLabels[col], Font_4x5, true); // Display column label

        // Draw vertical line next to the column labels
        int lineX = startX + 2;
        int lineStartY = startY;
        int lineEndY = lineStartY + (cellHeight * 4) - 1;
        display.DrawLine(lineX, lineStartY, lineX, lineEndY, true); // Draw a vertical line
    }

    // Display options in the grid based on channelMapping
    for (int row = 0; row < numRows; row++)
    {
        for (int col = 0; col < numCols; col++)
        {
            int x = startX + col * cellWidth;  // X-coordinate for the cell
            int y = startY + row * cellHeight; // Y-coordinate for the cell

            // Determine whether to fill the cell or just draw the border
            bool fillCell = (row == currentRow && col == currentCol); // Fill the selected cell with white only in grid mode

            // Draw the cell with or without filling
            display.DrawRect(x + 4, y + 2, x + cellWidth - 1, y + cellHeight, fillCell, fillCell);

            // Look up the string based on the channelMapping value
            int mappingValue = channelMapping[row][col];

            // Display the mapping string in the cell
            const char *mappingString = dubbyChannelMapping->ChannelMappingsStrings[mappingValue];
            bool negativeFill = (row == currentRow && col == currentCol && !selectJunctionMode);

            display.SetCursor(x + 5, y + 3);                             // Adjust text positioning for centering
            display.WriteString(mappingString, Font_4x5, !negativeFill); // Display mapping text

            // Draw a vertical line 6 pixels to the left of the right border of the rectangle
            int lineX = x + cellWidth - 2;
            display.DrawLine(lineX, y + 3, lineX, y + cellHeight - 1, negativeFill); // Draw line
        }
    }

    // Update the display after drawing all elements
    display.Update();
}

void Dubby::UpdateLFOWindow()
{
    float yOffset = 2;
    UpdateStatusBar("LFO 1           LFO 2 ", LEFT);

    int16_t displayWidth = display.Width();
    int16_t displayHeight = display.Height();
    int16_t yStart = displayHeight / 5 - 2 - yOffset;
    int16_t halfWidth = displayWidth / 2;
    int16_t rectHeight = 8;
    // Define the bounding box dimensions for LFO1 and LFO2
    int16_t lfo1BoundingBoxStartX = 0;
    int16_t lfo1BoundingBoxEndX = halfWidth / 2;

    int16_t lfo2BoundingBoxStartX = halfWidth;
    int16_t lfo2BoundingBoxEndX = halfWidth + halfWidth / 2;

    static bool isSelected[] = {true, false, false, false, false, false, false, false}; // 0: LFO1, 1: LFO2, 2: WaveShapeLFO1, 3: WaveShapeLFO2
    static bool selectIndexMode = false;
    float maxRateLFO = 10000.f;

    // Define box dimensions for LFO and WaveShape parameters
    int paramBoxLFOWidth = 34;
    int paramBoxLFOHeight = 8;
    int paramBoxWaveShapeWidth = 26;
    int paramBoxWaveShapeHeight = 8;

    // Define positions for parameter boxes
    int paramBoxLFO1X = 2;
    int paramBoxLFO1Y = 50 - yOffset;
    int paramBoxLFO2X = displayWidth / 2 + 2;
    int paramBoxLFO2Y = 50 - yOffset;

    int paramBoxWaveShapeLFO1X = halfWidth / 2 + 3;
    int paramBoxWaveShapeLFO1Y = 10 - yOffset;
    int paramBoxWaveShapeLFO2X = displayWidth - halfWidth / 2 + 3;
    int paramBoxWaveShapeLFO2Y = 10 - yOffset;

    const char *paramLFO1 = ParamsStrings[currentParamIndexLFO1];
    const char *paramLFO2 = ParamsStrings[currentParamIndexLFO2];
    const char *paramWaveShapeLFO1 = LFOWaveFormsStrings[currentParamIndexLFO1WaveShape];
    const char *paramWaveShapeLFO2 = LFOWaveFormsStrings[currentParamIndexLFO2WaveShape];

    // Draw parameter boxes and strings
    auto drawParamBox = [&](const char *param, int16_t x, int16_t y, int width, int height, bool selected, bool selectIndexMode)
    {
        bool fill = selected && selectIndexMode;
        display.DrawRect(x, y, x + width, y + height, selected, fill);
        display.SetCursor(x + 1, y + 2);
        display.WriteString(param, Font_4x5, !fill);
    };

    // Define parameters for circular knobs and bounding circles
    int circle_y = 34 - yOffset;    // Y-coordinate of the center of the circle
    int circle_radius = 6;          // Radius of the circle
    int bounding_circle_radius = 7; // Radius of the bounding circle, slightly larger than the knob circle
    int selectedIndices[NUM_KNOBS] = {1, 2, 5, 6};
    // Calculate total width occupied by circles
    int totalWidth = NUM_KNOBS * 2 * bounding_circle_radius;

    // Calculate space between circles
    int circleSpacing = (OLED_WIDTH - totalWidth) / (NUM_KNOBS + 1);

    // Define offsets for knobs
    const int offsetKnob1And2 = -6;
    const int offsetKnob3And4 = 6;

    // display.Fill(false);
    // ClearPane();
    display.DrawRect(0, 6, PANE_X_END + 1, PANE_Y_END, false, true);

    // // Draw the vertical line in the center of the display
    // display.DrawLine(halfWidth, PANE_Y_START, halfWidth, PANE_Y_END, true);

    // // Draw bounding box for LFO1
    display.DrawRect(lfo1BoundingBoxStartX, yStart, lfo1BoundingBoxEndX, yStart + rectHeight, true, false);

    // Draw rectangles for lfo1Value (left half)
    if (lfo1Value != 0)
    {
        int16_t x1_lfo1 = (lfo1Value < 0) ? halfWidth / 4 + (lfo1Value * (halfWidth / 2)) + 1 : halfWidth / 4 + 1;
        int16_t x2_lfo1 = (lfo1Value > 0) ? halfWidth / 4 + (lfo1Value * (halfWidth / 2)) : halfWidth / 4; // Subtract 2 pixels for width and move right by 2 pixels
        display.DrawRect(x1_lfo1, yStart, x2_lfo1, yStart + rectHeight, true, true);
    }

    // Draw bounding box for LFO2
    display.DrawRect(lfo2BoundingBoxStartX, yStart, lfo2BoundingBoxEndX, yStart + rectHeight, true, false);

    // Draw rectangles for lfo2Value (right half)
    if (lfo2Value != 0)
    {
        int16_t x1_lfo2 = (lfo2Value < 0) ? halfWidth + halfWidth / 4 + (lfo2Value * (halfWidth / 2)) + 1 : halfWidth + halfWidth / 4 + 1;
        int16_t x2_lfo2 = (lfo2Value > 0) ? halfWidth + halfWidth / 4 + (lfo2Value * (halfWidth / 2)) : halfWidth + halfWidth / 4; // Subtract 2 pixels for width and move right by 2 pixels
        display.DrawRect(x1_lfo2, yStart, x2_lfo2, yStart + rectHeight, true, true);
    }

    int increment = encoder.Increment();

    if (encoder.FallingEdge() && !windowSelectorActive && !wasEncoderJustInHighlightMenu)
        selectIndexMode = !selectIndexMode;

    // Determine which parameter box is selected
    int selectedIndex = -1;
    for (int i = 0; i < 8; ++i)
    {
        if (isSelected[i])
        {
            selectedIndex = i;
            break;
        }
    }

    // Update the parameter index based on selection mode
    if (increment != 0 && !windowSelectorActive)
    {
        if (selectIndexMode && selectedIndex != -1)
        {
            int paramCount = PARAMS_LAST; // Assuming PARAMS_LAST is the total number of parameters
            int waveformCount = daisysp::Oscillator::WAVE_LAST - 3;
            switch (selectedIndex)
            {
            case 0: // WaveShapeLFO1
                encoder.EnableAcceleration(false);

                currentParamIndexLFO1WaveShape = (currentParamIndexLFO1WaveShape + increment + waveformCount) % waveformCount;

                break;

            case 1:

                encoder.EnableAcceleration(true);
                // Update knobValues[1] with encoder increment
                knobValues[0] += encoder.Increment() * 1.;

                // Clamp knobValues[1] between 0 and 20
                if (knobValues[0] < 0.0f)
                {
                    knobValues[0] = 0.0f;
                }
                else if (knobValues[0] > maxRateLFO)
                {
                    knobValues[0] = maxRateLFO;
                }
                break;
            case 2:
                encoder.EnableAcceleration(false);

                // Update knobValues[1] with encoder increment
                knobValues[1] += encoder.Increment() * 0.05f;

                // Clamp knobValues[1] between 0 and 20
                if (knobValues[1] < 0.0f)
                {
                    knobValues[1] = 0.0f;
                }
                else if (knobValues[1] > 1.0f)
                {
                    knobValues[1] = 1.0f;
                }
                break;
            case 3: // PARAM LFO1
                encoder.EnableAcceleration(false);

                currentParamIndexLFO1 = (currentParamIndexLFO1 + increment + paramCount) % paramCount;

                break;
            case 4: // WaveShapeLFO2
                encoder.EnableAcceleration(false);

                currentParamIndexLFO2WaveShape = (currentParamIndexLFO2WaveShape + increment + waveformCount) % waveformCount;

                break;
            case 5:
                encoder.EnableAcceleration(true);

                // Update knobValues[1] with encoder increment
                knobValues[2] += encoder.Increment() * 1.f;

                // Clamp knobValues[1] between 0 and 20
                if (knobValues[2] < 0.0f)
                {
                    knobValues[2] = 0.0f;
                }
                else if (knobValues[2] > maxRateLFO)
                {
                    knobValues[2] = maxRateLFO;
                }
                break;
            case 6:
                encoder.EnableAcceleration(false);

                knobValues[3] += encoder.Increment() * 0.05f;

                // Clamp knobValues[1] between 0 and 20
                if (knobValues[3] < 0.0f)
                {
                    knobValues[3] = 0.0f;
                }
                else if (knobValues[3] > 1.0f)
                {
                    knobValues[3] = 1.0f;
                }
                break;
            case 7: // WaveShapeLFO2
                encoder.EnableAcceleration(false);

                currentParamIndexLFO2 = (currentParamIndexLFO2 + increment + paramCount) % paramCount;
                break;
            }
        }
        else
        {
            // Switch selection mode
            if (selectIndexMode)
            {
                // Switch selection based on increment
                int newIndex = (selectedIndex + increment + 8) % 8;
                // Deselect all other boxes
                for (int i = 0; i < 8; ++i)
                {
                    isSelected[i] = false;
                }
                // Select the new index
                isSelected[newIndex] = true;
            }
            else
            {
                // Toggle selection between parameters
                int nextIndex = (selectedIndex + increment + 8) % 8;
                // Ensure the nextIndex stays within bounds
                nextIndex = (nextIndex < 0) ? (nextIndex + 8) % 8 : nextIndex;
                // Deselect all other boxes
                for (int i = 0; i < 8; ++i)
                {
                    isSelected[i] = false;
                }
                // Select the next box
                isSelected[nextIndex] = true;
            }
        }
    }

    // Draw boxes for LFO1, LFO2, WaveShapeLFO1, and WaveShapeLFO2
    drawParamBox(paramWaveShapeLFO1, paramBoxWaveShapeLFO1X, paramBoxWaveShapeLFO1Y, paramBoxWaveShapeWidth, paramBoxWaveShapeHeight, isSelected[0], selectIndexMode);
    drawParamBox(paramLFO1, paramBoxLFO1X, paramBoxLFO1Y - 1, paramBoxLFOWidth, paramBoxLFOHeight, isSelected[3], selectIndexMode);
    drawParamBox(paramWaveShapeLFO2, paramBoxWaveShapeLFO2X, paramBoxWaveShapeLFO2Y, paramBoxWaveShapeWidth, paramBoxWaveShapeHeight, isSelected[4], selectIndexMode);
    drawParamBox(paramLFO2, paramBoxLFO2X, paramBoxLFO2Y - 1, paramBoxLFOWidth, paramBoxLFOHeight, isSelected[7], selectIndexMode);

    // visualizeKnobValuesCircle(customLabels, numDecimals);

    // Loop through each knob value
    for (int i = 0; i < NUM_KNOBS; ++i)
    {
        bool selected = isSelected[selectedIndices[i]];
        bool pressed = isSelected[selectedIndices[i]] & selectIndexMode;

        // Calculate knob x-coordinate with the applied offsets
        int circle_x_offset = circleSpacing * (i + 1) + bounding_circle_radius + i * 2 * bounding_circle_radius;

        // Apply offsets based on knob ƒindex
        if (i == 0 || i == 1)
        { // Knobs 1 and 2
            circle_x_offset += offsetKnob1And2;
        }
        else if (i == 2 || i == 3)
        { // Knobs 3 and 4
            circle_x_offset += offsetKnob3And4;
        }

        // Draw circular knob
        display.DrawCircle(circle_x_offset, circle_y, bounding_circle_radius, selected); // Draw filled knob circle
        display.DrawCircle(circle_x_offset, circle_y, circle_radius, true);              // Draw filled knob circle
        display.DrawCircle(circle_x_offset, circle_y, circle_radius - 1, pressed);       // Draw filled knob circle

        // Normalize the knob value for the first and third knobs
        float normalizedValue = knobValues[i];
        if (i == 0 || i == 2) // Knobs 1 and 3
        {
            normalizedValue = knobValues[i] / maxRateLFO; // Normalize to 0-1 range
        }

        // Calculate angle for the current knob
        float angle = (normalizedValue * 0.8f * 2 * PI_F) - (PI_F * 1.5f) + 0.2 * PI_F; // Convert normalized value to angle
        // Calculate line end position based on knob value
        int line_end_x = circle_x_offset + static_cast<int>(circle_radius * cos(angle));
        int line_end_y = circle_y + static_cast<int>(circle_radius * sin(angle));

        // Draw line indicating knob value
        display.DrawLine(circle_x_offset, circle_y, line_end_x, line_end_y, true);

        // Calculate the position for the label to be centered above the circle
        int label_x = circle_x_offset - (customLabels[i].size() * 4) / 2; // Assuming each character is 4 pixels wide in the selected font
        int label_y = circle_y - 13;                                      // Adjust this value to position the label properly above the circle

        // Draw custom label above each circle
        display.SetCursor(label_x, label_y);
        display.WriteString(customLabels[i].c_str(), Font_4x5, true);

        // Format knob value as string
        char formattedValue[10];
        snprintf(formattedValue, 10, "%.*f", numDecimals[i], knobValues[i]);

        // Calculate the position for the value to be centered under the circle
        int value_width = strlen(formattedValue) * 4; // Assuming each character is 4 pixels wide in the selected font
        int value_x = circle_x_offset - value_width / 2;

        // Draw knob value below the label
        display.SetCursor(value_x, circle_y + 9);
        display.WriteString(formattedValue, Font_4x5, true);
    }

    // Update the display to show the changes
    // display.Update();
}

void Dubby::UpdateLFO()
{
    lfo1.SetWaveform(currentParamIndexLFO1WaveShape);
    lfo1.SetFreq(knobValues[0]);
    lfo2.SetWaveform(currentParamIndexLFO2WaveShape);
    lfo2.SetFreq(knobValues[2]);
}

void Dubby::ProcessLFO()
{
    lfo1Value = lfo1.Process() * knobValues[1];
    lfo2Value = lfo2.Process() * knobValues[3];

    if (currentParamIndexLFO1)
    {
        lfo1Values[currentParamIndexLFO1] = lfo1Value;
    }
    else
    {
        lfo1Values[currentParamIndexLFO1] = 0;
    }

    if (currentParamIndexLFO2)
    {
        lfo2Values[currentParamIndexLFO2] = lfo2Value;
    }
    else
    {
        lfo2Values[currentParamIndexLFO2] = 0;
    }
}

void Dubby::UpdateCurrentMappingWindow()
{
    // display.DrawRect(0, 0, PANE_X_END + 1, PANE_Y_END + 12, false, true);
    ClearPane();
    // Define constants
    const int numControls = 10;                             // Number of possible controls (e.g., KN1, KN2, ..., JSX, JSY)
    const int macroLabelCount = 12;                         // Total number of macro labels
    const int controlMappingCount = PARAMS_LAST;            // Total number of parameter mappings
    const int charWidth = 4, charHeight = 5;                // Height & width of each character in the font
    const float joystickMinX = 0.16f, joystickMaxX = 0.77f; // Minimum & maximum joystick value
    const float joystickMinY = 0.14f, joystickMaxY = 0.86f; // Minimum & maximum joystick value
    // const float joystickIdleX = 0.49f, joystickIdleY = 0.45f;    // Joystick X/Y idle value
    const int movementRangeWidth = 14, movementRangeHeight = 14; // Minimum & maximum range height for rectangle movement
    int rectWidthJoystick = 3, rectHeightJoystick = 3;           // Height & width of the joystick rectangle
    const int labelOffset = 3;                                   // Offset for labels from axis lines
    const int circleRadius = 5;                                  // Radius of circular knobs
    const int circleY = 12;                                      // Y-coordinate of the center of the circular knobs
    const int adjustedSpacing = 13;                              // Spacing between circular knobs
    const int buttonRectWidth = 4, buttonRectHeight = 8;         // Height & width of button rectangles
    const int offset = 26;                                       // Offset for positioning button rectangles

    int controlCount[CONTROLS_LAST] = {0}; // Assuming CONTROLS_LAST is the number of possible controls

    // First pass: count how many times each control appears
    for (int i = 0; i < controlMappingCount; i++)
    {
        switch (dubbyParameters[i].control)
        {
        case KN1:
            controlCount[0]++;
            break;
        case KN2:
            controlCount[1]++;
            break;
        case KN3:
            controlCount[2]++;
            break;
        case KN4:
            controlCount[3]++;
            break;
        case BTN1:
            controlCount[4]++;
            break;
        case BTN2:
            controlCount[5]++;
            break;
        case BTN3:
            controlCount[6]++;
            break;
        case BTN4:
            controlCount[7]++;
            break;
        case JSX:
            controlCount[8]++;
            break;
        case JSY:
            controlCount[9]++;
            break;
        default:
            // You can leave these as is or add some handling if necessary.
            break;
        }
    }
    // Initialize macroLabels with default value
    for (int i = 0; i < macroLabelCount; i++)
    {
        macroLabels[i] = "-";
    }
    for (int i = 0; i < controlMappingCount; i++)
    {
        switch (dubbyParameters[i].control)
        {
        case KN1:
            macroLabels[0] = (controlCount[0] > 1) ? "MACRO" : ParamsStrings[i];
            break;
        case KN2:
            macroLabels[1] = (controlCount[1] > 1) ? "MACRO" : ParamsStrings[i];
            break;
        case KN3:
            macroLabels[2] = (controlCount[2] > 1) ? "MACRO" : ParamsStrings[i];
            break;
        case KN4:
            macroLabels[3] = (controlCount[3] > 1) ? "MACRO" : ParamsStrings[i];
            break;
        case BTN1:
            macroLabels[4] = (controlCount[4] > 1) ? "MACRO" : ParamsStrings[i];
            break;
        case BTN2:
            macroLabels[5] = (controlCount[5] > 1) ? "MACRO" : ParamsStrings[i];
            break;
        case BTN3:
            macroLabels[6] = (controlCount[6] > 1) ? "MACRO" : ParamsStrings[i];
            break;
        case BTN4:
            macroLabels[7] = (controlCount[7] > 1) ? "MACRO" : ParamsStrings[i];
            break;
        case JSX:
            macroLabels[8] = (controlCount[8] > 1) ? "MACRO" : ParamsStrings[i];
            break;
        case JSY:
            macroLabels[9] = (controlCount[9] > 1) ? "MACRO" : ParamsStrings[i];
            break;
        default:
            // This should never happen, but it's good practice to handle unexpected cases.
            break;
        }
    }
    // Update joystick X and Y labels and create new labels with '-'
    for (int i = 0; i < numControls; i++)
    {
        int labelLength = (i == 8 || i == 9) ? 3 : 4;

        // Cast labelLength to std::string::size_type to match macroLabels[i].length() type
        if (macroLabels[i].length() > static_cast<std::string::size_type>(labelLength))
        {
            macroLabels[i] = macroLabels[i].substr(0, labelLength);

            if (i == 8 || i == 9)
            {
                macroLabels[i] += "+";
                macroLabels[i + 2] = macroLabels[i].substr(0, 3) + "-";
            }
        }
    }

    display.DrawRect(0, 0, PANE_X_END + 1, PANE_Y_END, false, true);

    // Change size when joystick button is pressed
    rectHeightJoystick = rectWidthJoystick = joystickButton.Pressed() ? 5 : 3;

    // Mapping joystick values to screen coordinates
    float joystickX = GetKnobValue(CTRL_5); // Get joystick X value (joystickMin to joystickMax)
    float joystickY = GetKnobValue(CTRL_6); // Get joystick Y value (joystickMin to joystickMax)

    // Calculate the center position, so the movement range is centered on the screen
    int centerX = OLED_WIDTH / 2;
    int centerY = OLED_HEIGHT / 1.6;

    // Define the boundary rectangle encompassing the possible movement area
    int boundaryX1 = centerX - movementRangeWidth / 2;
    int boundaryY1 = centerY - movementRangeHeight / 2;
    int boundaryX2 = boundaryX1 + movementRangeWidth;
    int boundaryY2 = boundaryY1 + movementRangeHeight;

    // Draw the coordinate system lines inside the boundary rectangle
    // Vertical line (y-axis)
    int axisX = (boundaryX1 + boundaryX2) / 2;
    display.DrawLine(axisX, boundaryY1, axisX, boundaryY2, true);

    // Horizontal line (x-axis)
    int axisY = (boundaryY1 + boundaryY2) / 2;
    display.DrawLine(boundaryX1, axisY, boundaryX2, axisY, true);
    // Define the percentage threshold for snapping to idle
    float snapThreshold = 0.10; // 10%

    // Check if the joystick X is within 10% of the idle value
    if (fabs(joystickX - joystickIdleX) / (joystickMaxX - joystickMinX) < snapThreshold)
    {
        joystickX = joystickIdleX;
    }

    // Check if the joystick Y is within 10% of the idle value
    if (fabs(joystickY - joystickIdleY) / (joystickMaxY - joystickMinY) < snapThreshold)
    {
        joystickY = joystickIdleY;
    }

    // Normalize joystick values with joystickIdleX and joystickIdleY as reference points
    float normalizedX = (joystickX - joystickIdleX) / (joystickMaxX - joystickMinX);
    float normalizedY = (joystickY - joystickIdleY) / (joystickMaxY - joystickMinY);

    // Flip the X and Y axes and map the normalized values to the screen coordinate range
    int mappedX = centerX - normalizedX * movementRangeWidth;  // Flip X-axis
    int mappedY = centerY + normalizedY * movementRangeHeight; // Flip Y-axis (positive movementY moves down)

    // Rectangle size
    int rectX1 = mappedX - (rectWidthJoystick / 2);
    int rectY1 = mappedY - (rectHeightJoystick / 2);
    int rectX2 = rectX1 + rectWidthJoystick - 1;
    int rectY2 = rectY1 + rectHeightJoystick - 1;

    // Draw the rectangle at the new position
    display.DrawRect(rectX1, rectY1, rectX2, rectY2, true, true);

    // Calculate the position for the joystick labels
    // Joystick X label (to the right of the end of the X-axis line)
    int joystickXLabelX = axisX + 7 + labelOffset;
    int joystickXLabelY = axisY - charHeight / 2; // Center vertically based on font height

    // Joystick Y label (centered under the end of the Y-axis line)
    int joystickYLabelWidth = macroLabels[9].size() * charWidth; // Width of the joystick Y label
    int joystickYLabelX = axisX - (joystickYLabelWidth / 2);     // Center horizontally
    int joystickYLabelY = boundaryY2 + labelOffset;              // Offset below the end of the Y-axis line

    // New joystick X "-" label (to the left of the X-axis line)
    int joystickXNegLabelX = axisX - 7 - (macroLabels[10].size() * charWidth) - labelOffset;
    int joystickXNegLabelY = axisY - charHeight / 2; // Center vertically based on font height

    // New joystick Y "-" label (centered above the Y-axis line)
    int joystickYNegLabelWidth = macroLabels[11].size() * charWidth;    // Width of the joystick Y "-" label
    int joystickYNegLabelX = axisX - (joystickYNegLabelWidth / 2);      // Center horizontally
    int joystickYNegLabelY = boundaryY1 + 1 - labelOffset - charHeight; // Offset above the end of the Y-axis line

    // Draw the joystick parameter labels
    display.SetCursor(joystickXLabelX, joystickXLabelY);
    display.WriteString(macroLabels[8].c_str(), Font_4x5, true); // Label for joystick X "+"

    display.SetCursor(joystickYLabelX, joystickYLabelY);
    display.WriteString(macroLabels[9].c_str(), Font_4x5, true); // Label for joystick Y "+"

    // Draw the new joystick labels
    display.SetCursor(joystickXNegLabelX, joystickXNegLabelY);
    display.WriteString(macroLabels[10].c_str(), Font_4x5, true); // Label for joystick X "-"

    display.SetCursor(joystickYNegLabelX, joystickYNegLabelY);
    display.WriteString(macroLabels[11].c_str(), Font_4x5, true); // Label for joystick Y "-"

    // Calculate total width occupied by circles
    int totalWidth = NUM_KNOBS * 2 * circleRadius;

    // Calculate space between circles
    int circleSpacing = (OLED_WIDTH - totalWidth - (NUM_KNOBS - 1) * adjustedSpacing) / 2;

    circleSpacing = std::max(circleSpacing, adjustedSpacing); // Ensure spacing is not negative

    // Loop through each knob value
    for (int i = 0; i < NUM_KNOBS; ++i)
    {
        // Calculate knob x-coordinate
        int circleXOffset = circleSpacing + (i * (2 * circleRadius + adjustedSpacing)) + circleRadius;

        float knobValueLive = GetKnobValue(static_cast<Ctrl>(i));
        // Calculate angle for the current knob
        float angle = (knobValueLive * 0.8f * 2 * PI_F) - (PI_F * 1.5f) + 0.2 * PI_F; // Convert knob value to angle

        // Calculate line end p2osition based on knob value
        int lineEndX = circleXOffset + static_cast<int>(circleRadius * cos(angle));
        int lineEndY = circleY + static_cast<int>(circleRadius * sin(angle));

        // Draw circular knob
        display.DrawCircle(circleXOffset, circleY, circleRadius, true);

        // Draw line indicating knob value
        display.DrawLine(circleXOffset, circleY, lineEndX, lineEndY, true);

        // Calculate the position for the label to be centered above the circle
        int labelX = circleXOffset - (macroLabels[i].size() * charWidth) / 2; // Assuming each character is charWidth pixels wide in the selected font
        int labelY = 0;                                                       // Adjust this value to position the label properly above the circle

        // Draw custom label above each circle
        display.SetCursor(labelX, labelY);
        display.WriteString(macroLabels[i].c_str(), Font_4x5, true);
    }

    // Top-left corner
    display.DrawRect(0, PANE_Y_START + offset - 4, buttonRectWidth, buttonRectHeight + PANE_Y_START + offset - 4, true, buttons[0].Pressed());
    display.SetCursor(buttonRectWidth + 2, PANE_Y_START + offset - 4 + 2);
    display.WriteString(macroLabels[4].c_str(), Font_4x5, true);

    // Top-right corner
    display.DrawRect(OLED_WIDTH - buttonRectWidth - 1, PANE_Y_START + offset - 4, OLED_WIDTH - 1, buttonRectHeight + PANE_Y_START + offset - 4, true, buttons[2].Pressed());
    int textWidth = macroLabels[5].size() * charWidth;
    display.SetCursor(OLED_WIDTH - buttonRectWidth - textWidth - 3, PANE_Y_START + offset - 4 + 2);
    display.WriteString(macroLabels[5].c_str(), Font_4x5, true);

    // Bottom-left corner
    display.DrawRect(0, PANE_Y_END - buttonRectHeight - 4, buttonRectWidth, PANE_Y_END - 4, true, buttons[1].Pressed());
    display.SetCursor(buttonRectWidth + 2, PANE_Y_END - buttonRectHeight - 4 + 2);
    display.WriteString(macroLabels[6].c_str(), Font_4x5, true);

    // Bottom-right corner
    display.DrawRect(OLED_WIDTH - buttonRectWidth - 1, PANE_Y_END - buttonRectHeight - 4, OLED_WIDTH - 1, PANE_Y_END - 4, true, buttons[3].Pressed());
    textWidth = macroLabels[7].size() * charWidth;
    display.SetCursor(OLED_WIDTH - buttonRectWidth - textWidth - 3, PANE_Y_END - buttonRectHeight - 4 + 2);
    display.WriteString(macroLabels[7].c_str(), Font_4x5, true);

    // Update the display after drawing all elements

    if (!windowSelectorActive)
    {
        display.Update();
    }
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

void Dubby::UpdateGlobalSettingsPane()
{
    if (encoder.FallingEdge() && !wasEncoderJustInHighlightMenu && !isSubMenuActive)
    {
        isSubMenuActive = true;
        DisplayPreferencesMenuList(0);
    }

    if (encoder.FallingEdge() && !wasEncoderJustInHighlightMenu && windowSelectorActive)
    {
        isSubMenuActive = false;
        DisplayPreferencesMenuList(0);
    }

    DisplayPreferencesSubMenuList(encoder.Increment(), preferencesMenuItemSelected);

    if (encoder.RisingEdgeCustom() && !windowSelectorActive && ((seed.system.GetNow() - encoderLastDebounceTime2) > encoderDebounceDelay2))
    {
        if (!isSubMenuActive)
        {
            isSubMenuActive = true;
            // UpdateStatusBar("UPDATED", MIDDLE, 128);
            DisplayPreferencesMenuList(0);
        }

        switch (preferencesMenuItemSelected)
        {
        case LEDS:
            break;
        case SAVEMEMORY:
            OpenModal("ARE YOU SURE?");
            break;
        case RESETMEMORY:
            OpenModal("ARE YOU SURE?");
            break;
        case DFUMODE:
            OpenModal("ARE YOU SURE?");
            break;
        default:
            break;
        }
    }

    if (encoder.Increment() && !windowSelectorActive && !isSubMenuActive)
        UpdatePreferencesMenuList(encoder.Increment());
    else if (encoder.Increment() && !windowSelectorActive && isSubMenuActive)
        UpdatePreferencesSubMenuList(encoder.Increment(), preferencesMenuItemSelected);
}

void Dubby::UpdateParameterPane()
{
    // std::string hlmode = std::to_string(wasEncoderJustInHighlightMenu);
    // UpdateStatusBar(&hlmode[0], LEFT);
    DisplayParameterList(encoder.Increment());

    if (encoder.Increment() && !isEncoderIncrementDisabled && !windowSelectorActive && !isParameterSelected)
        UpdateParameterList(encoder.Increment());

    if (encoder.FallingEdge() && !windowSelectorActive && !isParameterSelected && !wasEncoderJustInHighlightMenu)
    {
        if ((seed.system.GetNow() - encoderLastDebounceTime2) > encoderDebounceDelay2)
        {
            isParameterSelected = true;
            parameterOptionSelected = PARAM;
            DisplayParameterList(encoder.Increment());
        }
    }
    // if (encoder.FallingEdge() && wasEncoderJustInHighlightMenu) wasEncoderJustInHighlightMenu = !wasEncoderJustInHighlightMenu;

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

        if (encoder.RisingEdgeCustom())
        {
            isListeningControlChange = false;
            isEncoderIncrementDisabled = false;
            UpdateStatusBar(" PARAM       CTRL      VALUE  >", LEFT);
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
        if (encoder.RisingEdgeCustom())
        {
            isCurveChanging = false;
            isEncoderIncrementDisabled = false;
            UpdateStatusBar(" PARAM       CTRL   <  CURVE   ", LEFT);
        }
    }
    else if (isMinChanging)
    {
        if (encoder.Increment())
        {
            float incrementValue = encoder.Increment() / 100.0f;
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
        if (encoder.RisingEdgeCustom())
        {
            isMinChanging = false;
            isEncoderIncrementDisabled = false;
            encoder.EnableAcceleration(false);
            UpdateStatusBar(" PARAM       CTRL   <  MIN    >", LEFT);
        }
    }
    else if (isMaxChanging)
    {
        if (encoder.Increment())
        {
            float incrementValue = encoder.Increment() / 100.0f;
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

        if (encoder.RisingEdgeCustom())
        {
            isMaxChanging = false;
            isEncoderIncrementDisabled = false;
            encoder.EnableAcceleration(false);
            UpdateStatusBar(" PARAM       CTRL   <  MAX    >", LEFT);
        }
    }
    else if (isValueChanging)
    {
        float incrementValue = encoder.Increment() / 100.0f;
        if (incrementValue)
        {
            if (incrementValue < 0)
            {
                if (dubbyParameters[parameterSelected].value > dubbyParameters[parameterSelected].min)
                {
                    if ((dubbyParameters[parameterSelected].value + incrementValue) < dubbyParameters[parameterSelected].min)
                        dubbyParameters[parameterSelected].baseValue = ceil(dubbyParameters[parameterSelected].value + incrementValue);
                    else
                        dubbyParameters[parameterSelected].baseValue += incrementValue;

                    if (dubbyParameters[parameterSelected].value < dubbyParameters[parameterSelected].min)
                        dubbyParameters[parameterSelected].baseValue = dubbyParameters[parameterSelected].min;
                }
            }
            else if (incrementValue > 0)
            {
                if (dubbyParameters[parameterSelected].value < dubbyParameters[parameterSelected].max)
                {
                    if ((dubbyParameters[parameterSelected].value + incrementValue) > dubbyParameters[parameterSelected].max)
                        dubbyParameters[parameterSelected].baseValue = floor(dubbyParameters[parameterSelected].value + incrementValue);
                    else
                        dubbyParameters[parameterSelected].baseValue += incrementValue;

                    if (dubbyParameters[parameterSelected].value > dubbyParameters[parameterSelected].max)
                        dubbyParameters[parameterSelected].baseValue = dubbyParameters[parameterSelected].max;
                }
            }
        }
        if (encoder.RisingEdgeCustom())
        {
            isValueChanging = false;
            isEncoderIncrementDisabled = false;
            encoder.EnableAcceleration(false);
            UpdateStatusBar(" PARAM       CTRL      VALUE  >", LEFT);
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

        if (encoder.RisingEdgeCustom())
        {
            if (parameterOptionSelected == PARAM)
            {
                isParameterSelected = false;
            }
            else if (parameterOptionSelected == CTRL)
            {
                UpdateStatusBar("SELECT A CONTROL", MIDDLE, 127);
                isListeningControlChange = true;
                isEncoderIncrementDisabled = true;

                for (int i = 0; i < CONTROLS_LAST; i++)
                    dubbyCtrls[i].tempValue = dubbyCtrls[i].value;
            }
            else if (parameterOptionSelected == CURVE)
            {
                UpdateStatusBar("SELECT A CURVE", MIDDLE, 127);
                isEncoderIncrementDisabled = true;
                isCurveChanging = true;
            }
            else if (parameterOptionSelected == MIN)
            {
                UpdateStatusBar("SELECT MIN VALUE", MIDDLE, 127);
                isEncoderIncrementDisabled = true;
                isMinChanging = true;
                encoder.EnableAcceleration(true);
            }
            else if (parameterOptionSelected == MAX)
            {
                UpdateStatusBar("SELECT MAX VALUE", MIDDLE, 127);
                isEncoderIncrementDisabled = true;
                isMaxChanging = true;
                encoder.EnableAcceleration(true);
            }
            else if (parameterOptionSelected == VALUE && dubbyParameters[parameterSelected].control == CONTROL_NONE)
            {
                UpdateStatusBar("SELECT A VALUE", MIDDLE, 127);
                isEncoderIncrementDisabled = true;
                isValueChanging = true;
                encoder.EnableAcceleration(true);
            }
        }
    }
}

void Dubby::UpdateMidiSettingsPane()
{
    DisplayMidiSettingsList(encoder.Increment());

    if (encoder.Increment() && !windowSelectorActive && !isMidiSettingSelected)
        UpdateMidiSettingsList(encoder.Increment());

    if (encoder.FallingEdge() && !wasEncoderJustInHighlightMenu && !windowSelectorActive)
    {
        isMidiSettingSelected = !isMidiSettingSelected;

        UpdateMidiSettingsList(encoder.Increment());
    }
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
    int numItems = 0;

    switch (prefMenuItemSelected)
    {
    case LEDS:
        type = PREFERENCESLEDSMENULIST;
        numItems = PREFERENCESLEDMENU_LAST;
        break;
    case ROUTING:
        type = PREFERENCESROUTINGMENULIST;
        numItems = PREFERENCESROUTINGMENU_LAST;
        break;
    default:
        type = PREFERENCESLEDSMENULIST;
        break;
    }

    int optionStart = 0;
    if (subMenuSelector > (numItems - 1))
    {
        optionStart = subMenuSelector - (numItems - 1);
    }

    // display each item, j for text cursor
    for (int i = optionStart, j = 0; i < numItems; i++, j++)
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

    display.DrawRect(PANE_X_START + MENULIST_SUBMENU_SPACING - 1, 0, PANE_X_END, PANE_Y_END + 1, true, false);

    display.Update();
}

void Dubby::UpdatePreferencesSubMenuList(int increment, PreferencesMenuItems prefMenuItemSelected)
{
    int endSelector = 0;

    switch (prefMenuItemSelected)
    {
    case LEDS:
        endSelector = sizeof(PreferencesLedsMenuItemsStrings);
        break;
    case ROUTING:
        endSelector = sizeof(PreferencesRoutingMenuItemsStrings);
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

void Dubby::UpdateStatusBar(const char *text, StatusBarSide side = LEFT, int width)
{
    Rectangle barRec = daisy::Rectangle(STATUSBAR_X_START, STATUSBAR_Y_START, STATUSBAR_X_END, STATUSBAR_Y_END);

    if (side == LEFT)
    {
        display.DrawRect(STATUSBAR_X_START, STATUSBAR_Y_START, width, STATUSBAR_Y_END - 1, false, true);
        display.WriteStringAligned(text, Font_4x5, barRec, daisy::Alignment::centeredLeft, true);
    }
    else if (side == MIDDLE)
    {
        display.DrawRect(64 - (width / 2), STATUSBAR_Y_START, 64 + (width / 2), STATUSBAR_Y_END - 1, false, true);
        display.WriteStringAligned(text, Font_4x5, barRec, daisy::Alignment::centered, true);
    }
    else if (side == RIGHT)
    {
        display.DrawRect(STATUSBAR_X_END - width, STATUSBAR_Y_START, STATUSBAR_X_END, STATUSBAR_Y_END - 1, false, true);
        display.WriteStringAligned(text, Font_4x5, barRec, daisy::Alignment::centeredRight, true);
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

        // ALIGN TO RIGHT
        // Rectangle strArea = Rectangle(93, PARAMLIST_Y_START + 1 + (j * PARAMLIST_SPACING), 30, 5);
        // display.WriteStringAligned(&str[0], Font_4x5, strArea, daisy::Alignment::centeredRight, !(parameterSelected == i && isParameterSelected));

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
    {
        if (dubbyParameters[i].control != CONTROL_NONE)
        {
            dubbyParameters[i].CalculateRealValue(dubbyCtrls[dubbyParameters[i].control].value);
            dubbyParameters[i].value = daisysp::fclamp(dubbyParameters[i].value + (lfo1Values[i] * (dubbyParameters[i].max - dubbyParameters[i].min)) + (lfo2Values[i] * (dubbyParameters[i].max - dubbyParameters[i].min)), dubbyParameters[i].min, dubbyParameters[i].max);
        }
        else
        {
            dubbyParameters[i].value = daisysp::fclamp(dubbyParameters[i].baseValue + (lfo1Values[i] * (dubbyParameters[i].max - dubbyParameters[i].min)) + (lfo2Values[i] * (dubbyParameters[i].max - dubbyParameters[i].min)), dubbyParameters[i].min, dubbyParameters[i].max);
        }
    }
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

void Dubby::OpenModal(const char *text)
{
    isModalActive = true;

    display.DrawRect(MODAL_X_START - 2, MODAL_Y_START - 2, MODAL_X_END + 2, MODAL_Y_END + 2, false, true);
    display.DrawRect(MODAL_X_START, MODAL_Y_START, MODAL_X_END, MODAL_Y_END, true);

    display.WriteStringAligned(text, Font_6x8, Rectangle(MODAL_X_START + 5, MODAL_Y_START + 5, 100, 12), Alignment::centered, true);

    ChangeModalOption();
}

void Dubby::ChangeModalOption()
{
    modalOptionSelected = (ModalOptions)!modalOptionSelected;

    display.DrawRect(MODAL_X_START + 2, MODAL_LEFT_OPTION_Y_START - 2, MODAL_X_END - 2, MODAL_LEFT_OPTION_Y_START + MODAL_OPTION_HEIGHT + 2, false, true);

    display.DrawRect(MODAL_LEFT_OPTION_X_START, MODAL_LEFT_OPTION_Y_START, MODAL_LEFT_OPTION_X_START + MODAL_OPTION_WIDTH, MODAL_LEFT_OPTION_Y_START + MODAL_OPTION_HEIGHT, true, !modalOptionSelected);
    display.WriteStringAligned("YES", Font_6x8, Rectangle(MODAL_LEFT_OPTION_X_START + 1, MODAL_LEFT_OPTION_Y_START + 1, MODAL_OPTION_WIDTH, MODAL_OPTION_HEIGHT), Alignment::centered, modalOptionSelected);

    display.DrawRect(MODAL_RIGHT_OPTION_X_START, MODAL_RIGHT_OPTION_Y_START, MODAL_RIGHT_OPTION_X_START + MODAL_OPTION_WIDTH, MODAL_RIGHT_OPTION_Y_START + MODAL_OPTION_HEIGHT, true, modalOptionSelected);
    display.WriteStringAligned("NO", Font_6x8, Rectangle(MODAL_RIGHT_OPTION_X_START + 1, MODAL_RIGHT_OPTION_Y_START + 1, MODAL_OPTION_WIDTH, MODAL_OPTION_HEIGHT), Alignment::centered, !modalOptionSelected);

    display.Update();
}

void Dubby::CloseModal()
{
    isModalActive = false;
    modalOptionSelected = YES;

    display.DrawRect(0, 0, OLED_WIDTH, OLED_WIDTH, false, true);

    ReleaseWindowSelector();
    UpdateWindowList();
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
    cfg.blocksize = AUDIO_BLOCK_SIZE;
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
    case PREFERENCESLEDSMENULIST:
        return PreferencesLedsMenuItemsStrings[enumVal];
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