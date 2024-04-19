#pragma once
#include "daisy_core.h"

#include "daisysp.h"
#include "../Dubby.h"

#include "Midi.h"

using namespace daisy;
using namespace daisysp;

namespace daisy
{

void Monitor(Dubby& dubby) 
{
    dubby.ProcessAllControls();
    dubby.UpdateDisplay();
}

} // namespace daisy
