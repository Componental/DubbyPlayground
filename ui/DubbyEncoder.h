#pragma once
#include "daisy_core.h"
#include "per/gpio.h"
#include "hid/switch.h"

#define DEBOUNCE_TIME 2
#define ACCELERATION_THRESHOLD 20 // Time in milliseconds to trigger acceleration
#define MAX_ACCELERATION 10       // Maximum acceleration multiplier

namespace daisy
{
  /**
      @brief Generic Class for handling Quadrature Encoders \n
      Inspired/influenced by Mutable Instruments (pichenettes) Encoder classes
      @author Stephen Hensley
      @date December 2019
      @ingroup controls
  */
  class DubbyEncoder
  {
  public:
    DubbyEncoder() {}
    ~DubbyEncoder() {}

    /** Initializes the encoder with the specified hardware pins.
     * Update rate is to be deprecated in a future release
     */
    void Init(dsy_gpio_pin a,
              dsy_gpio_pin b,
              dsy_gpio_pin click,
              float update_rate = 0.f);
    /** Called at update_rate to debounce and handle timing for the switch.
     * In order for events not to be missed, its important that the Edge/Pressed checks be made at the same rate as the debounce function is being called.
     */
    void Debounce();

    /** Returns the adjusted increment value considering acceleration. */
    inline int32_t Increment() const { return updated_ ? (use_acceleration_ ? accumulated_inc_ : inc_) : 0; }

    /** Returns true if the encoder was just pressed. */
    inline bool RisingEdge() const { return sw_.RisingEdge(); }

    bool RisingEdgeCustom();

    /** Returns true if the encoder was just released. */
    inline bool FallingEdge() const { return sw_.FallingEdge(); }

    bool FallingEdgeCustom();

    /** Returns true while the encoder is held down.*/
    inline bool Pressed() const { return sw_.Pressed(); }

    /** Returns the time in milliseconds that the encoder has been held down. */
    inline float TimeHeldMs() const { return sw_.TimeHeldMs(); }

    /** To be removed in breaking update
     * \param update_rate Does nothing
     */
    inline void SetUpdateRate(float update_rate) {}

    /** Enable or disable acceleration dynamically. */
    inline void EnableAcceleration(bool enable) { use_acceleration_ = enable; }

  private:
    uint32_t last_update_;
    bool updated_;
    Switch sw_;
    dsy_gpio hw_a_, hw_b_;
    uint8_t a_, a_last, b_, b_last, a_stable_, b_stable_;
    int32_t inc_;
    int32_t accumulated_inc_; // Accumulated increments for acceleration
    int32_t last_encoder_change_time_;
    int prev_state;

    bool encoderState = false;                 // Previous state of the button
    bool encoderLastState = true;              // Previous state of the button
    
    unsigned long encoderLastDebounceTime = 0; // Time the button was last toggled
    unsigned long encoderDebounceDelay = 10;   // Debounce time in milliseconds

    // Variables for acceleration
    bool use_acceleration_ = false;
    float acceleration_factor_;
    float acceleration_multiplier_ = 1;
    uint32_t acceleration_time_threshold_;
    uint32_t max_acceleration_;
  };
} // namespace daisy