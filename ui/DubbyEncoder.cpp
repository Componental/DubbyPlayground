#include "DubbyEncoder.h"

using namespace daisy;

void DubbyEncoder::Init(dsy_gpio_pin a,
                   dsy_gpio_pin b,
                   dsy_gpio_pin click,
                   float        update_rate)
{
    last_update_ = System::GetNow();
    updated_     = false;

    // Init GPIO for A, and B
    hw_a_.pin  = a;
    hw_a_.mode = DSY_GPIO_MODE_INPUT;
    hw_a_.pull = DSY_GPIO_PULLUP;
    hw_b_.pin  = b;
    hw_b_.mode = DSY_GPIO_MODE_INPUT;
    hw_b_.pull = DSY_GPIO_PULLUP;
    dsy_gpio_init(&hw_a_);
    dsy_gpio_init(&hw_b_);
    // Default Initialization for Switch
    sw_.Init(click);
    // Set initial states, etc.
    inc_ = 0;
    a_ = b_ = 0xff;


    a_last = dsy_gpio_read(&hw_a_);   
    b_last = dsy_gpio_read(&hw_b_);   
}

void DubbyEncoder::Debounce()
{
    // update no faster than 1kHz
    uint32_t now = System::GetNow(); 
    updated_     = false;

    if (now - last_update_ >= 1)
    {
        last_update_ = now;
        updated_     = true;

        a_ = dsy_gpio_read(&hw_a_);
        b_ = dsy_gpio_read(&hw_b_);

        int lastInc = inc_;
        if (lastInc == 0) lastInc = 1;

        // infer increment direction
        inc_ = 0; // reset inc_ first

        // Introduce a time-based debounce
        if (now - last_encoder_change_time_ >= DEBOUNCE_TIME)
        {
            if (lastInc == 1) 
            {
                // Only detect changes when both A and B have settled
                if (a_ != a_last && b_ == a_) inc_ = 1; // Clockwise rotation
                else if (b_ != b_last && b_ == a_) inc_ = -1; // Counterclockwise rotation
            }
            else if (lastInc == -1)
            {
                // Only detect changes when both A and B have settled
                if (b_ != b_last && b_ == a_) inc_ = -1; // Counterclockwise rotation
                else if (a_ != a_last && b_ == a_) inc_ = 1; // Clockwise rotation
            }

            // Update the time of the last encoder state change
            last_encoder_change_time_ = now;
        }
    }

    a_last = a_; // Updates the previous state of the a with the current state
    b_last = b_; // Updates the previous state of the a with the current state

    // Debounce built-in switch
    sw_.Debounce();
}