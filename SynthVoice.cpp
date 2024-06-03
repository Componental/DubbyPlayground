/***** SynthVoice.cpp *****/
#include "SynthVoice.h"

SynthVoice::SynthVoice()
{
	
}

void SynthVoice::TriggerEnv(bool gate){

}

float SynthVoice::Process(){

  //  float output = synthVoice.Process();
        float osc1Out = osc1.Process();
        float osc2Out = osc2.Process();

        float oscCombinedOut = osc1Out + osc2Out;

        float filterOut = filter.Process(oscCombinedOut);

	return filterOut;
}
void SynthVoice::SetFreq(float freq)
{
	
}


