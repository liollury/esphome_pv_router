#pragma once
#include <esp32-hal-timer.h>
#include "esphome/core/hal.h"

#define sampling_count 100
#define smoothing_factor 10 // number of samples to average

namespace esphome {
	namespace pvrouter {
		static const char *const TAG = "pvr_measure";
		static const int smoothing_weight = smoothing_factor - 1;
		
		class Measure {
		  private:
			int volt[sampling_count];
			int amp[sampling_count];
			float voltM;
			float ampM;
			unsigned long previous_compute_millis;
			volatile unsigned long last_zero_cross_interruption = 0;
			volatile int current_triac_position = 0;
			int sampling_value_count = 0;
			hw_timer_t* timer = NULL;
			bool sync_lost = false;
		  protected:
			InternalGPIOPin *zero_crossing_pin_;
			GPIOPin *triac_pin_;
			InternalGPIOPin *analog_ref_pin_;
			InternalGPIOPin *analog_volt_pin_;
			InternalGPIOPin *analog_current_pin_;
			float volt_factor_;
			float current_factor_;
			int min_triac_delay_;
			int power_margin_;
			int triac_load_step_;
		  public:
			bool heating_allowed = false;
			bool force_heating = false;
			
			float pW = 0;
			float pVA = 0;
			float Wh = 0;
			bool is_power_connected = false; 
			int triac_delay = 100;
			bool over_production = false;
			float voltage = 0;
			float current = 0;
			Measure(
				InternalGPIOPin *zero_crossing_pin, 
				GPIOPin *triac_pin, 
				InternalGPIOPin *analog_ref_pin, 
				InternalGPIOPin *analog_volt_pin, 
				InternalGPIOPin *analog_current_pin,
				float volt_factor,
				float current_factor,
				int min_triac_delay,
				int power_margin,
				int triac_load_step
			);
			void setup();
			void measure_power();
			void compute_power();
			void compute_triac_delay();
			void update();
			void reset_wh();
			static void zero_cross_interrupt(void *_this);
			void on_timer_interrupt();
		};
	}
}