#pragma once
#include "daisy_core.h"

#include "daisysp.h"
#include "../Dubby.h"

using namespace daisy;
using namespace daisysp;

namespace daisy
{


void AssignScopeData(Dubby& dubby, size_t i, AudioHandle::InputBuffer& in, AudioHandle::OutputBuffer& out) 
{
    switch (dubby.scopeSelector)
    {
        case 0: dubby.scope_buffer[i] = (in[0][i] + in[1][i]) * .5f; break;
        case 1: dubby.scope_buffer[i] = (in[0][i] + in[1][i]) * .5f; break;
        case 2: dubby.scope_buffer[i] = (out[0][i] + out[1][i]) * .5f; break;
        case 3: dubby.scope_buffer[i] = (out[2][i] + out[3][i]) * .5f; break;
        case 4: dubby.scope_buffer[i] = in[0][i]; break;
        case 5: dubby.scope_buffer[i] = in[1][i]; break;
        case 6: dubby.scope_buffer[i] = in[2][i]; break;
        case 7: dubby.scope_buffer[i] = in[3][i]; break;
        case 8: dubby.scope_buffer[i] = out[0][i]; break;
        case 9: dubby.scope_buffer[i] = out[1][i]; break;
        case 10: dubby.scope_buffer[i] = out[2][i]; break;
        case 11: dubby.scope_buffer[i] = out[3][i]; break;
        default: dubby.scope_buffer[i] = (in[0][i] + in[1][i]) * .5f; break;
    }
}

} // namespace daisy
