#include "SynthVoice.h"



SynthVoice::SynthVoice() {
            osc2Tune = 0.0f; // Initialize osc2Tune to 0

    // Constructor implementation (if needed)
}
void SynthVoice::Init(int sample_rate){
	   osc1.Init(sample_rate);
    osc1.SetFreq(250.f); // Set an initial frequency for osc1
    osc1.SetAmp(0.1);

    osc2.Init(sample_rate);
    osc2.SetFreq(250.f); // Set an initial frequency for osc2
    osc2.SetAmp(0.1);

    filter.Init(sample_rate);

    filterEnv.Init(sample_rate);
    ampEnv.Init(sample_rate);
        daisysp::LadderFilter::FilterMode currentMode = daisysp::LadderFilter::FilterMode::LP24;

}
void SynthVoice::TriggerEnv(bool gate){
        
        filterEnvOut = filterEnv.Process(gate);
        ampEnvOut = ampEnv.Process(gate);

        osc1.SetAmp(ampEnvOut);
        osc2.SetAmp(ampEnvOut);


}

float SynthVoice::Process(){

  //  float output = synthVoice.Process();
        float osc1Out = osc1.Process();
        float osc2Out = osc2.Process();

        float oscCombinedOut = osc1Out + osc2Out;

        float filterOut = filter.Process(oscCombinedOut);

	return filterOut;
}

void SynthVoice::SetOsc1Tune(float tuneKnobValue, float octaveKnobValue) {
    // Tuning factor for OSC 1
    const float TUNE_SEMITONES = 7.0f;
    const float SEMITONE_FACTOR = powf(2.0f, 1.0f / 12.0f); // Factor for one semitone

    // Calculate the tuning shift in semitones
    float osc1NoteShift = tuneKnobValue * TUNE_SEMITONES;

    // Calculate the octave shift
    float octaveShift = CalculateOctaveShift(octaveKnobValue);

    // Calculate the frequency multiplier based on the semitone shift
    float noteShiftMultiplier = powf(SEMITONE_FACTOR, osc1NoteShift);

    // Calculate the frequency multiplier based on the octave shift
    float octaveMultiplier = powf(2.0f, octaveShift);

    // Combine both multipliers
    float frequencyMultiplier = noteShiftMultiplier * octaveMultiplier;

    // Set the frequency for OSC 1
    osc1.SetFreq(oscFreq * frequencyMultiplier);
}

void SynthVoice::SetOsc2Tune(float tuneKnobValue, float octaveKnobValue) {
    // Tuning factor for OSC 2
    const float TUNE_SEMITONES = 7.0f;
    const float SEMITONE_FACTOR = powf(2.0f, 1.0f / 12.0f); // Factor for one semitone

    // Calculate the tuning shift in semitones
    float osc2NoteShift = tuneKnobValue * TUNE_SEMITONES;

    // Calculate the octave shift
    float octaveShift = CalculateOctaveShift(octaveKnobValue);

    // Calculate the frequency multiplier based on the semitone shift
    float noteShiftMultiplier = powf(SEMITONE_FACTOR, osc2NoteShift);

    // Calculate the frequency multiplier based on the octave shift
    float octaveMultiplier = powf(2.0f, octaveShift);

    // Combine both multipliers
    float frequencyMultiplier = noteShiftMultiplier * octaveMultiplier;

    // Set the frequency for OSC 2
    osc2.SetFreq(oscFreq * frequencyMultiplier);
}

float SynthVoice::CalculateOctaveShift(float knobValue) {
    float octaveShift = 0.0f;

    if (knobValue >= 0.0f && knobValue < 0.20f) {
        // Pitch down 2 octaves
        octaveShift = -2.0f;
    } else if (knobValue >= 0.20f && knobValue < 0.40f) {
        // Pitch down one octave
        octaveShift = -1.0f;
    } else if (knobValue >= 0.40f && knobValue < 0.60f) {
        // Do nothing
        octaveShift = 0.0f;
    } else if (knobValue >= 0.60f && knobValue < 0.80f) {
        // Pitch up one octave
        octaveShift = 1.0f;
    } else if (knobValue >= 0.80f && knobValue <= 1.0f) {
        // Pitch up two octaves
        octaveShift = 2.0f;
    }

    return octaveShift;
}

void SynthVoice::SetFreq(float freq) {
    osc1.SetFreq(freq);
    osc2.SetFreq(freq);
    oscFreq = freq; // Update the stored frequency value
}

float SynthVoice::GetFreq() {
    return oscFreq; // Return the stored frequency value
}



void SynthVoice::SetOsc1Shape(float shape){
        osc1.SetWaveform(shape * 6);

}

void SynthVoice::SetOsc2Shape(float shape){
	        osc2.SetWaveform(shape * 6);

}

void SynthVoice::SetFilterDrive(float filterDrive){
		filter.SetInputDrive(filterDrive * 4);


}

void SynthVoice::SetFilterRes(float filterRes){
	        filter.SetRes(filterRes);

}


void SynthVoice::SetFilterCutoff(float filterCutoff, float filterEnvAmp){
	                float cutOffKnobValue = filterCutoff + (filterEnvAmp*filterEnvOut);

			        // Map the knob value to a logarithmic scale for cutoff frequency
        float minCutoff = .05f;    // Minimum cutoff frequency in Hz
        float maxCutoff = 17000.f; // Maximum cutoff frequency in Hz
        float mappedCutoff = daisysp::fmap(cutOffKnobValue, minCutoff, maxCutoff, daisysp::Mapping::LOG);


			filter.SetFreq(mappedCutoff);

}

void SynthVoice::SetFilterADSR(float attack, float decay, float sustain, float release){

	  float minAttackFilter = .01f; // Minimum cutoff frequency in Hz
        float maxAttackFilter = 3.f;  // Maximum cutoff frequency in Hz
        float mappedAttackFilter = daisysp::fmap(attack, minAttackFilter, maxAttackFilter, daisysp::Mapping::LOG);

        // Set envelope parameters
        filterEnv.SetTime(daisysp::ADSR_SEG_ATTACK, mappedAttackFilter);
        filterEnv.SetTime(daisysp::ADSR_SEG_DECAY, decay);
        filterEnv.SetSustainLevel(sustain);
        filterEnv.SetTime(daisysp::ADSR_SEG_RELEASE, release);
}

void SynthVoice::SetAmpADSR(float attack, float decay, float sustain, float release){

	   // Map the knob value to a logarithmic scale for cutoff frequency
        float minAttackAmp = .01f; // Minimum cutoff frequency in Hz
        float maxAttackAmp = 3.f;  // Maximum cutoff frequency in Hz
        float mappedAttackAmp = daisysp::fmap(attack, minAttackAmp, maxAttackAmp, daisysp::Mapping::LOG);

        // Set envelope parameters
        ampEnv.SetTime(daisysp::ADSR_SEG_ATTACK, mappedAttackAmp);
        ampEnv.SetTime(daisysp::ADSR_SEG_DECAY, decay);
        ampEnv.SetSustainLevel(sustain);
        ampEnv.SetTime(daisysp::ADSR_SEG_RELEASE, release);

}