#pragma once
#include "daisy_core.h"

#include "daisysp.h"
#include "../Dubby.h"

using namespace daisy;
using namespace daisysp;

namespace daisy
{
    int selectedPage = 0;
    const float knobTolerance = 0.02f; // Define a tolerance for knob adjustment, Adjust as needed
    bool knobWithinTolerance[5][NUM_KNOBS] = {0};
    std::vector<float> knobValues;

    bool WithinTolerance(float value1, float value2)
    {
        return fabs(value1 - value2) <= knobTolerance; // Check if the difference between two values is within a tolerance limit
    }

    void HandleKnobs(Dubby &dubby, const char **algorithmTitles, const char *(*customLabels)[NUM_KNOBS], float (*getKnobValueMatrix)[NUM_KNOBS])
    {
        static bool prevButtonState = false;
        static int prevPage = 0;

        int rotationDirection = dubby.encoder.Increment(); // Get the direction of knob rotation
        bool buttonPressed = dubby.buttons[0].Pressed();   // Check if the button is pressed

        if (buttonPressed && !prevButtonState)
        {
            prevPage = selectedPage;
            selectedPage = 4; // Set selectedPage to 4 if the button is pressed for the first time

            // Reset all knobs to false for all drums except the selected one
            for (int i = 0; i < NUM_PAGES; i++)
            {
                if (i != selectedPage)
                {
                    for (int j = 0; j < NUM_KNOBS; j++)
                    {
                        knobWithinTolerance[i][j] = false; // Reset tolerance status for all knobs except on the selected page
                    }
                }
            }
        }
        else if (!buttonPressed && prevButtonState)
        {
            selectedPage = prevPage; // Return to the previous page if the button is released
        }
        prevButtonState = buttonPressed; // Save the current button state for the next iteration
        if (rotationDirection != 0 && !buttonPressed && !dubby.windowSelectorActive && dubby.windowItemSelected == dubby.WIN1)
        {
            // Reset all knobs to false for all drums except the selected one
            for (int i = 0; i < NUM_PAGES; i++)
            {
                if (i != selectedPage)
                {
                    for (int j = 0; j < NUM_KNOBS; j++)
                    {
                        knobWithinTolerance[i][j] = false; // Reset tolerance status for all knobs except on the selected page
                    }
                }
            }
            // If there was a knob rotation and the button is not pressed, change the selected page
            selectedPage = (selectedPage + rotationDirection + (NUM_PAGES)) % (NUM_PAGES);
        }
        // Update the algorithm title and custom labels for the selected page
        dubby.algorithmTitle = algorithmTitles[selectedPage];
        dubby.customLabels.assign(customLabels[selectedPage], customLabels[selectedPage] + NUM_KNOBS);
        if (dubby.windowItemSelected == dubby.WIN1)
        {
            dubby.UpdateAlgorithmTitle();
        }
        // Process knobs for the selected page
        for (int j = 0; j < NUM_KNOBS; j++)
        {
            float knobValue = dubby.GetKnobValue(static_cast<daisy::Dubby::Ctrl>(j)); // Get the value of each knob
            // Only check for tolerance if the knob is not already within tolerance
            if (!knobWithinTolerance[selectedPage][j])
            {
                if (WithinTolerance(knobValue, getKnobValueMatrix[selectedPage][j])) // Check if knob value is within tolerance
                {
                    knobWithinTolerance[selectedPage][j] = true; // Set knob within tolerance
                }
            }
            // Update the saved value only if it's within tolerance
            if (knobWithinTolerance[selectedPage][j])
            {
                getKnobValueMatrix[selectedPage][j] = knobValue; // Update saved knob value
            }
            knobValues[j] = getKnobValueMatrix[selectedPage][j];                      // Update knob values array
            dubby.savedKnobValuesForVisuals[j] = getKnobValueMatrix[selectedPage][j]; // Update saved knob values for visuals
        }
        // Update the knob values for the visual representation
        dubby.UpdateKnobValues(knobValues);
    }
} // namespace daisy
