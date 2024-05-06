#include "daisysp.h"
using namespace daisysp;

class Hihat
{
  
  public:
 
    Hihat()  = default;
    ~Hihat() = default;
 
    void Init(float sample_rate);
 
    float Process();
  
    void SetFreq(float freq);
 
    void SetAttack(float attack);
 
    void SetDecay(float decay);
 
    void SetFilterRes(float res);
    void Trigger();

 
  private:
 
    float      sample_rate_;
    float      alpha_;
    WhiteNoise noise;
    AdEnv env;
    Svf bpFilter;
     

};
 
