#pragma once
#include "daisy_core.h"
#include "../Dubby.h"

using namespace daisy;

namespace daisy
{

void SaveToQspi(Dubby& dubby, PersistentStorage<PersistantMemoryParameterSettings>& SavedParameterSettings) {

	//Reference to local copy of settings stored in flash
	PersistantMemoryParameterSettings &LocalParameterSettings = SavedParameterSettings.GetSettings();

	LocalParameterSettings.savedParams[dubby.parameterSelected].value = dubby.dubbyParameters[dubby.parameterSelected].value;
	LocalParameterSettings.savedParams[dubby.parameterSelected].control = dubby.dubbyParameters[dubby.parameterSelected].control;
	LocalParameterSettings.savedParams[dubby.parameterSelected].min = dubby.dubbyParameters[dubby.parameterSelected].min;
	LocalParameterSettings.savedParams[dubby.parameterSelected].max = dubby.dubbyParameters[dubby.parameterSelected].max;
	LocalParameterSettings.savedParams[dubby.parameterSelected].curve = dubby.dubbyParameters[dubby.parameterSelected].curve;
    LocalParameterSettings.isSaved[dubby.parameterSelected] = true;

    SavedParameterSettings.Save(); // Writing locally stored settings to the external flash
}


void LoadFromQspi(Dubby& dubby, PersistentStorage<PersistantMemoryParameterSettings>& SavedParameterSettings) {

	//Reference to local copy of settings stored in flash
	PersistantMemoryParameterSettings &LocalParameterSettings = SavedParameterSettings.GetSettings();

    for (int i = 1; i < PARAMS_LAST; i++) {

        if(LocalParameterSettings.isSaved) 
        {
            dubby.dubbyParameters[i].value = LocalParameterSettings.savedParams[i].value;
            dubby.dubbyParameters[i].control = LocalParameterSettings.savedParams[i].control;
            dubby.dubbyParameters[i].min = LocalParameterSettings.savedParams[i].min;
            dubby.dubbyParameters[i].max = LocalParameterSettings.savedParams[i].max;
            dubby.dubbyParameters[i].curve = LocalParameterSettings.savedParams[i].curve;
        } 
    }
}

void InitPersistantMemory(Dubby& dubby, PersistentStorage<PersistantMemoryParameterSettings>& SavedParameterSettings) 
{
    PersistantMemoryParameterSettings DefaultParameterSettings;

    for (int i = 0; i < PARAMS_LAST - 1; i++) 
    {
        DefaultParameterSettings.savedParams[i] = dubby.dubbyParameters[i];
    }

	SavedParameterSettings.Init(DefaultParameterSettings);
    LoadFromQspi(dubby, SavedParameterSettings);
}

void MonitorPersistantMemory(Dubby& dubby, PersistentStorage<PersistantMemoryParameterSettings>& SavedParameterSettings) 
{
    dubby.ProcessAllControls();
    dubby.UpdateDisplay();

    if(dubby.trigger_save_parameters_qspi) {
        SaveToQspi(dubby, SavedParameterSettings);
        dubby.trigger_save_parameters_qspi = false;
    }
}

} // namespace daisy
