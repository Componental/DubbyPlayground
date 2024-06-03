/***** SynthVoice.h *****/
#include "daisysp.h"
class SynthVoice {

public:
    SynthVoice();

    void Init (int sample_rate);
void TriggerEnv(bool gate);
float Process();
void SetFreq(float freq);
void SetOsc1Shape(float shape);
void SetOsc2Shape(float shape);
void SetFilterDrive(float filterDrive);
void SetFilterRes(float filterRes);
void SetFilterCutoff(float filterCutoff, float filterEnvAmt);
void SetFilterADSR(float attack, float decay, float sustain, float release);
void SetAmpADSR(float attack, float decay, float sustain, float release);

private:
daisysp::Oscillator osc1, osc2;

static daisysp::LadderFilter filter;

static daisysp::Adsr filterEnv, ampEnv;
float filterEnvOut, ampEnvOut;




};
