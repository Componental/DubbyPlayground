#include "DubbyEncoder.h"

using namespace daisy;

void DubbyEncoder::Init(dsy_gpio_pin a,
                        dsy_gpio_pin b,
                        dsy_gpio_pin click,
                        float update_rate)
{
    last_update_ = System::GetNow();
    updated_ = false;

    // Init GPIO for A, and B
    hw_a_.pin = a;
    hw_a_.mode = DSY_GPIO_MODE_INPUT;
    hw_a_.pull = DSY_GPIO_PULLUP;
    hw_b_.pin = b;
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
    updated_ = false;

    if (now - last_update_ >= 0.5)
    {
        last_update_ = now;
        updated_ = true;

        a_ = dsy_gpio_read(&hw_a_);
        b_ = dsy_gpio_read(&hw_b_);

        // Reset inc_ to ensure it only changes if movement is detected
        inc_ = 0;

        // Introduce a time-based debounce
        if (now - last_encoder_change_time_ >= DEBOUNCE_TIME)
        {
            if (a_ != a_last || b_ != b_last) // Only update inc_ if either A or B changes
            {
                if (a_ != a_last && b_ == a_)
                    inc_ = 1; // Clockwise rotation
                else if (b_ != b_last && b_ == a_)
                    inc_ = -1; // Counterclockwise rotation

                // Calculate the time difference between this and the last change
                uint32_t deltaTime = now - last_encoder_change_time_;

                if (deltaTime < ACCELERATION_THRESHOLD)
                {
                    acceleration_factor_ = (1.0f + ((ACCELERATION_THRESHOLD - deltaTime) / (float)ACCELERATION_THRESHOLD) * (MAX_ACCELERATION - 1));
                }
                else
                {
                    acceleration_factor_ = 1.0f; // No acceleration
                }

                // Apply the acceleration factor to the increment
                accumulated_inc_ += (int32_t)(inc_ * acceleration_factor_ * acceleration_multiplier_);

                if (acceleration_multiplier_ < 8)
                    acceleration_multiplier_ *= 1.05f;
                else if (acceleration_multiplier_ < 64)
                    acceleration_multiplier_ *= 1.04f;
                else if (acceleration_multiplier_ < 256)
                    acceleration_multiplier_ *= 1.03f;
                else if (acceleration_multiplier_ < 512)
                    acceleration_multiplier_ *= 1.02f;
                else if (acceleration_multiplier_ < 2048)
                    acceleration_multiplier_ *= 1.01f;

                if (now - last_encoder_change_time_ >= 100)
                    acceleration_multiplier_ = 1.0f;

                // Update the time of the last encoder state change
                last_encoder_change_time_ = now;
            }
        }

        // Update the previous state variables
        a_last = a_;
        b_last = b_;
    }

    // Reset accumulated_inc_ if no movement is detected to avoid false positives
    if (inc_ == 0)
    {
        accumulated_inc_ = 0;
    }

    // Debounce built-in switch
    sw_.Debounce();
}

bool DubbyEncoder::RisingEdgeCustom()
{
    bool reading = sw_.Pressed(); // Read the encoder button state, assuming true is pressed

    if (reading != encoderLastState)
        encoderLastDebounceTime = System::GetNow();

    if ((System::GetNow() - encoderLastDebounceTime) > encoderDebounceDelay)
    {
        if (reading != encoderState)
        {
            encoderState = reading;

            if (encoderState == false)
            { // Encoder button released
                return true;
            }
        }
    }

    encoderLastState = reading;

    return false;
}

bool DubbyEncoder::FallingEdgeCustom()
{
    bool reading = sw_.Pressed(); // Read the encoder button state, assuming true is pressed

    if (reading != encoderLastState)
    {
        encoderLastDebounceTime = System::GetNow();
    }

    if ((System::GetNow() - encoderLastDebounceTime) > encoderDebounceDelay)
    {
        if (reading != encoderState)
        {
            encoderState = reading;

            if (encoderState == true)
            { // Encoder button pressed
                return true;
            }
        }
    }

    encoderLastState = reading;

    return false;
}