
#pragma once
#include "daisy_seed.h"
#include "dev/oled_ssd130x.h"
#include "fonts/dubby_oled_fonts.h"

#include <iostream>
#include <string>
#include <vector>
#include <cstdlib>

#include "daisysp.h"
#include "ui/DubbyEncoder.h"
#include "led.h"
#include "libDubby/Parameters.h"

#include "./bitmaps/bmps.h"

#define AUDIO_BLOCK_SIZE 128
#define NUM_AUDIO_CHANNELS 4
#define PI_F 3.1415927410125732421875f
#define NUM_KNOBS 4

namespace daisy
{

    // Setting Struct containing parameters we want to save to flash
    struct PersistantMemoryParameterSettings
    {

        Parameters savedParams[PARAMS_LAST];
        bool isSaved[PARAMS_LAST];

        // Overloading the != operator
        // This is necessary as this operator is used in the PersistentStorage source code
        bool operator!=(const PersistantMemoryParameterSettings &a) const
        {
            for (int i = 0; i < PARAMS_LAST; ++i)
            {
                if (savedParams[i].param != a.savedParams[i].param ||
                    savedParams[i].control != a.savedParams[i].control ||
                    savedParams[i].value != a.savedParams[i].value ||
                    savedParams[i].min != a.savedParams[i].min ||
                    savedParams[i].max != a.savedParams[i].max ||
                    savedParams[i].minLimit != a.savedParams[i].minLimit ||
                    savedParams[i].maxLimit != a.savedParams[i].maxLimit ||
                    savedParams[i].hasMinLimit != a.savedParams[i].hasMinLimit ||
                    savedParams[i].hasMaxLimit != a.savedParams[i].hasMaxLimit ||
                    savedParams[i].curve != a.savedParams[i].curve ||
                    isSaved[i] != a.isSaved[i])
                {
                    return true;
                }
            }
            return false;
        }
    };

    class Dubby
    {

    public:
        enum AudioIns
        {
            IN1,
            IN2,
            IN3,
            IN4
        };

        enum AudioOuts
        {
            OUT1,
            OUT2,
            OUT3,
            OUT4
        };

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

        const char *WindowItemsStrings[WIN_LAST] =
            {
                "SCOPE",
                "MIXER",
                "PREFS",
                "PARAMETERS",
                "MIDI CONF",
                "ROUTING",
                "LFO",
                "PERFORM",
        };

        enum PreferencesMenuItems
        {
            LEDS,
            ROUTING,
            DFUMODE,
            PARAMS,
            SAVEMEMORY,
            RESETMEMORY,
            RESET,
            PREFERENCESMENU_LAST // used to know the size of enum
        };

        const char *PreferencesMenuItemsStrings[PREFERENCESMENU_LAST] =
            {
                "LEDS",
                "MEMORY",
                "FIRMWARE",
                "CALLIBRATION",
                "SAVE MEMORY",
                "RESET MEMORY",
                "RESET"
            };

        enum Ctrl
        {
            CTRL_1, // knob 1
            CTRL_2, // knob 2
            CTRL_3, // knob 3
            CTRL_4, // knob 4
            CTRL_5, // joystick horizontal
            CTRL_6, // joystick vertical
            CTRL_LAST
        };

        enum GateInput
        {
            GATE_IN_1, // button 1
            GATE_IN_2, // button 2
            GATE_IN_3, // button 3
            GATE_IN_4, // button 4
            GATE_IN_5, // joystick button
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

        const char *ScopePagesStrings[SCOPE_PAGES_LAST] =
            {
                "IN 1, IN 2",
                "IN 3, IN 4",
                "OUT 1, OUT 2",
                "OUT 3, OUT 4",
                "IN 1",
                "IN 2",
                "IN 3",
                "IN 4",
                "OUT 1",
                "OUT 2",
                "OUT 3",
                "OUT 4",
        };

        enum MixerPages
        {
            INPUTS,
            OUTPUTS,
            MIXER_PAGES_LAST
        };

        const char *MixerPagesStrings[MIXER_PAGES_LAST] =
            {
                "INPUTS",
                "OUTPUTS",
        };

        enum PreferencesLedsMenuItems
        {
            MAXBRIGHTNESS1,
            MAXBRIGHTNESS2,
            PREFERENCESLEDMENU_LAST // used to know the size of enum
        };

        const char *PreferencesLedsMenuItemsStrings[2] =
            {
                "MAX BRIGHTNESS 1",
                "MAX BRIGHTNESS 2",
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

        const char *PreferencesRoutingMenuItemsStrings[PREFERENCESROUTINGMENU_LAST] =
            {
                "SAVE MEMORY",
                "RESET MEMORY",
                "SAVE MEMORY1",
                "RESET MEMORY1",
                "RESET MEMORY2",
        };

        enum EnumTypes
        {
            PREFERENCESLEDSMENULIST,
            PREFERENCESROUTINGMENULIST,
            PREFERENCESMENU,
            WINDOWS,
            SCOPE,
            MIXERPAGES,
        };

        const char *ControlsStrings[CONTROLS_LAST] =
            {
                "-",
                "KNOB1",
                "KNOB2",
                "KNOB3",
                "KNOB4",
                "BTN1",
                "BTN2",
                "BTN3",
                "BTN4",
                "JSX",
                "JSY",
                "JSSW",
        };

        const char *ParamsStrings[PARAMS_LAST] =
            {
                "-",
                "TIME",
                "FEEDBACK",
                "MIX",
                "CUTOFF",
                "IN GAIN",
                "OUT GAIN",
                "FREEZE",
                "MUTE",
                "LOOP",
                "2ND_LAST" // because of a bug
        };

        const char *CurvesStrings[CURVES_LAST]{
            "LINEAR",
            "LOGARITH",
            "EXPONENT",
            "SIGMOID",
        };

        const char *LFOWaveFormsStrings[daisysp::Oscillator::WAVE_LAST - 3] =
            {
                "SIN",
                "TRI",
                "SAW",
                "RAMP",
                "SQUARE"};
        int currentParamIndexLFO1WaveShape, currentParamIndexLFO2WaveShape;
        int currentParamIndexLFO1 = 0, currentParamIndexLFO2 = 0;

        enum ModalOptions
        {
            YES,
            NO
        };

        const int numRows = 4;
        const int numCols = 4;

        int channelMapping[NUM_AUDIO_CHANNELS][NUM_AUDIO_CHANNELS] = {
            // Input channels:       0     1     2     3
            /* Output channel 0 */ {0, 0, 0, 0},
            /* Output channel 1 */ {0, 0, 0, 0},
            /* Output channel 2 */ {0, 0, 0, 0},
            /* Output channel 3 */ {0, 0, 0, 0}};

        Dubby() {}

        ~Dubby() {}

        void Init();

        void SetAudioInGain(AudioIns in, float gain);

        float GetAudioInGain(AudioIns in);

        void SetAudioOutGain(AudioOuts out, float gain);

        float GetAudioOutGain(AudioOuts out);

        void UpdateDisplay();

        void DrawLogo();

        void DrawBitmap(int bitmapIndex);

        void UpdateWindowSelector(int increment, bool higlight = true);

        void HighlightWindowItem();

        void ReleaseWindowSelector();

        void UpdateWindowList();

        void UpdateMixerPane();

        void UpdateBar(int i);

        void UpdateLFO();
        void DrawLFOValues(int16_t lfoValue, int16_t xStart, int16_t xEnd, int16_t yStart, int16_t rectHeight);
        void DrawParamBox(const char *param, int16_t x, int16_t y, int16_t width, int16_t height, bool selected, bool invert);

        void ProcessLFO();

        void UpdateRenderPane();

        void UpdateGlobalSettingsPane();

        void UpdateParameterPane();

        void UpdateMidiSettingsPane();

        void RenderScope();

        void DisplayPreferencesMenuList(int increment);

        void UpdatePreferencesMenuList(int increment);

        void DisplayPreferencesSubMenuList(int increment, PreferencesMenuItems preferencesMenuItemSelected);

        void UpdatePreferencesSubMenuList(int increment, PreferencesMenuItems preferencesMenuItemSelected);

        void DisplayParameterList(int increment);

        void DisplayMidiSettingsList(int increment);

        void UpdateParameterList(int increment);

        void UpdateMidiSettingsList(int increment);

        void UpdateLFOWindow();

        void UpdateCurrentMappingWindow();

        void ProcessAllControls();

        void ProcessAnalogControls();

        void ProcessDigitalControls();

        float GetKnobValue(Ctrl k);

        const char *GetTextForEnum(EnumTypes m, int enumVal);

        void ResetToBootloader();

        void SwitchMIDIOutThru(bool state);

        void ClearPane();

        void UpdateStatusBar(const char *text, StatusBarSide side, int width = 40); // side = 0 => left, side = 1 => right

        void updateKnobValues(const std::vector<float> &values);

        void visualizeKnobValues(const std::vector<std::string> &knobLabels, const std::vector<int> &numDecimals);

        void visualizeKnobValuesCircle(const std::vector<std::string> &knobLabels, const std::vector<int> &numDecimals);

        void UpdateAlgorithmTitle();

        DubbyControls GetParameterControl(Params p);

        float GetParameterValue(Parameters p);

        void UpdateChannelMappingPane();

        void OpenModal(const char *text);

        void ChangeModalOption();

        void CloseModal();

        DaisySeed seed;

        WindowItems windowItemSelected = (WindowItems)0;

        PreferencesMenuItems preferencesMenuItemSelected = (PreferencesMenuItems)0;
        PreferencesLedsMenuItems  preferencesLedsMenuItemSelected = (PreferencesLedsMenuItems)0;
        PreferencesRoutingMenuItems preferencesRoutingMenuItemSelected = (PreferencesRoutingMenuItems)0;
        int subMenuSelector = 0;

        DubbyControls prevControl = CONTROL_NONE;

        Params parameterSelected = (Params)1;
        bool isParameterSelected = false;
        ParameterOptions parameterOptionSelected = PARAM;

        bool isListeningControlChange = false;
        bool isCurveChanging = false;
        bool isMinChanging = false;
        bool isMaxChanging = false;
        bool isValueChanging = false;

        float lfo1Values[PARAMS_LAST] = {0}; // Store LFO1 values for each parameter
        float lfo2Values[PARAMS_LAST] = {0}; // Store LFO2 values for each parameter
        float lfo1Value = 0, lfo2Value = 0;
        bool isEncoderIncrementDisabled = false;

        bool isSubMenuActive = false;


        MidiSettings midiSettingSelected = (MidiSettings)0;
        bool isMidiSettingSelected = false;
        bool testBool = false;

        ChannelMappings channelMappingSelected = (ChannelMappings)0;
        bool isChannelMappingSelected = false;
        daisysp::Oscillator lfo1, lfo2;
         float joystickIdleX, joystickIdleY;

        // const int menuTextCursors[3][2] = { {8, 55}, {50, 55}, {92, 55} }; OLD
        const int windowTextCursors[3][2] = {{3, 52}, {46, 52}, {88, 52}};
        const int windowBoxBounding[3][4] = {{0, 56, 43, 61}, {43, 56, 85, 61}, {85, 56, 127, 61}};
        int menuListBoxBounding[7][4];
        int paramListBoxBounding[8][4];
        int midiListBoxBounding[5][4];

        int scrollbarWidth = 0;
        int barSelector = 0;
        bool isBarSelected = false;

        int scopeSelector = INPUT_1_2;

        DubbyEncoder encoder;
        AnalogControl analogInputs[CTRL_LAST];
        GateIn gateInputs[GATE_IN_LAST];
        Switch buttons[4];
        Switch joystickButton;

        float scope_buffer[AUDIO_BLOCK_SIZE] = {0.f};

        float currentLevels[2][4] = {{0.f}, {0.f}}; // 0 => INPUTS, 1 => OUTPUTS

        OledDisplay<SSD130x4WireSpi128x64Driver> display;

        MidiUartHandler midi_uart;
        dsy_gpio midi_sw_output;

        int mixerPageSelected = INPUTS;

        double sumSquaredIns[4] = {0.0f};
        double sumSquaredOuts[4] = {0.0f};

        MidiUsbHandler midi_usb;

        int globalBPM = 120;
        int receivedBPM;
        // uint32_t bpm = 120;
        std::vector<std::string> customLabels = {"RATE", "AMT", "RATE", "AMT"};
        std::vector<std::string> macroLabels = {"-", "-", "-", "-", "-", "-", "-", "-", "-", "-", "-", "-"};

        std::vector<float> knobValues = {100.f, 0.f, 25.f, 0.f};
        std::vector<int> numDecimals = {0, 1, 0, 1}; // Assuming you have three knobs with different decimal places

        std::vector<float> savedKnobValuesForVisuals;
        std::string algorithmTitle = "";

        Controls dubbyCtrls[CONTROLS_LAST];
        Parameters dubbyParameters[PARAMS_LAST];
        ChannelMappingMenu dubbyChannelMapping[CHANNELMAPPINGS_LAST];

        MidiSettingsMenu dubbyMidiSettings;
        bool trigger_save_parameters_qspi = false;
        bool trigger_reset_parameters_qspi = false;

        bool isModalActive = false;
        ModalOptions modalOptionSelected = YES;

    private:
        void InitAudio();
        void InitControls();
        void InitEncoder();
        void InitDisplay();
        void InitLEDs();
        void InitButtons();
        void InitMidi();
        void InitDubbyParameters();
        void InitDubbyControls();

        int margin = 8;
        bool windowSelectorActive = false;
        uint32_t screen_update_last_, screen_update_period_;

        bool isEncoderPressed = false;
        bool wasEncoderLongPressed = false;
        bool wasEncoderJustInHighlightMenu = false;
        int highlightMenuCounter = 0;
        unsigned long encoderPressStartTime = 0;

        unsigned long encoderLastDebounceTime2 = 0; // Time the button was last toggled
        unsigned long encoderDebounceDelay2 = 100;   // Debounce time in milliseconds

        float audioGains[2][4] = {{0.8f, 0.8f, 0.8f, 0.8f}, {0.8f, 0.8f, 0.8f, 0.8f}}; // 0 => INPUTS, 1 => OUTPUTS
    };

}
