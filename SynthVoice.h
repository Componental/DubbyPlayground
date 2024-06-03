/***** SynthVoice.h *****/
#include "daisysp.h"
class SynthVoice {

public:
    SynthVoice();
private:
daisysp::Oscillator osc1, osc2;

static daisysp::LadderFilter filter;

static daisysp::Adsr filterEnv, ampEnv;
float filterEnvOut, ampEnvOut;




void TriggerEnv(bool gate);
float Process();
void SetFreq(float freq);

};
