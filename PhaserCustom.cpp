#define SAMPLE_RATE (48000.f)  // Sample rate
#define PI (3.14159f)

class PhaserCustom{
public:
    PhaserCustom()  // Initialize to some useful defaults...
        : feedback( .7f )
        , lfoPhase( 0.f )
        , depth( 1.f )
        , zMinus1( 0.f )
    {
        Range( 440.f, 1600.f );
        Rate( .5f );
    }

    void Range( float minFreq, float maxFreq ){ // Hz
        minDelay = minFreq / (SAMPLE_RATE/2.f);
        maxDelay = maxFreq / (SAMPLE_RATE/2.f);
    }

    void Rate( float rate ){ // cps
        lfoIncrement = 2.f * PI * (rate / SAMPLE_RATE);
    }

    void Feedback( float fb ){ // 0 -> <1.
        feedback = fb;
    }

    void Depth( float d ){  // 0 -> 1.
        depth = d;
    }

    float Update( float inputSample ){
        // Calculate and update PhaserCustom sweep LFO...
        float delay  = minDelay + (maxDelay - minDelay) * ((sin( lfoPhase ) + 1.f) / 2.f);
        lfoPhase += lfoIncrement;
        if( lfoPhase >= PI * 2.f )
            lfoPhase -= PI * 2.f;

        // Update filter coefficients
        for( int i=0; i<6; i++ )
            allpassFilters[i].Delay( delay );

        // Calculate output
        float output = 	allpassFilters[0].Update(
                        allpassFilters[1].Update(
                        allpassFilters[2].Update(
                        allpassFilters[3].Update(
                        allpassFilters[4].Update(
                        allpassFilters[5].Update( inputSample + zMinus1 * feedback ))))));
        zMinus1 = output;

        return inputSample + output * depth;
    }
private:
    class AllpassDelay{
    public:
        AllpassDelay()
            : a1( 0.f )
            , zMinus1( 0.f )
            {}

        void Delay( float delay ){ // Sample delay time
            a1 = (1.f - delay) / (1.f + delay);
        }

        float Update( float inputSample ){
            float output = inputSample * -a1 + zMinus1;
            zMinus1 = output * a1 + inputSample;

            return output;
        }
    private:
        float a1, zMinus1;
    };

    AllpassDelay allpassFilters[6];

    float minDelay, maxDelay; // Range
    float feedback; // Feedback
    float lfoPhase;
    float lfoIncrement;
    float depth;

    float zMinus1;
};
