#pragma once
#include "esphome/core/component.h"
#include "esphome/core/hal.h"
#include "esphome/components/binary_sensor/binary_sensor.h"
#include "esphome/components/number/number.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/select/select.h"
#include "measure.h"

namespace esphome {
	static const char *const TAG = "pvr";

	
	namespace pvrouter {
		class PvRouter : public PollingComponent {
		public:
			sensor::Sensor *over_production_sensor = new sensor::Sensor();
			sensor::Sensor *consumption_sensor = new sensor::Sensor();
			sensor::Sensor *power_connecter_sensor = new sensor::Sensor();
			sensor::Sensor *instant_sensor = new sensor::Sensor();
			sensor::Sensor *voltage_sensor = new sensor::Sensor();
			sensor::Sensor *current_sensor = new sensor::Sensor();
  
			PvRouter();
			void set_active_pin(GPIOPin *pin) { active_pin_ = pin; }
			void set_conso_pin(GPIOPin *pin) { conso_pin_ = pin; }
			void set_inject_pin(GPIOPin *pin) { inject_pin_ = pin; }
			void set_zero_crossing_pin(InternalGPIOPin *pin) { zero_crossing_pin_ = pin; }
			void set_triac_pin(GPIOPin *pin) { triac_pin_ = pin; }
			void set_analog_ref_pin(InternalGPIOPin *pin) { analog_ref_pin_ = pin; }
			void set_analog_volt_pin(InternalGPIOPin *pin) { analog_volt_pin_ = pin; }
			void set_analog_current_pin(InternalGPIOPin *pin) { analog_current_pin_ = pin; }
			
			void set_over_production_sensor(binary_sensor::BinarySensor *over_production_sensor) { over_production_sensor_ = over_production_sensor; }
			void set_power_connected_sensor(binary_sensor::BinarySensor *power_connected_sensor) { power_connected_sensor_ = power_connected_sensor; }
			
			void set_tank_temperature_sensor(sensor::Sensor *temperature) { tank_temperature_sensor_ = temperature; }
			void set_tank_target_temperature_sensor(number::Number *tank_target_temperature_sensor) { tank_target_temperature_sensor_ = tank_target_temperature_sensor; }
			void set_triac_sensor(sensor::Sensor *triac_sensor) { triac_sensor_ = triac_sensor; }
			void set_consumption_sensor(sensor::Sensor *consumption_sensor) { consumption_sensor_ = consumption_sensor; }
			void set_instant_power_sensor(sensor::Sensor *instant_power_sensor) { instant_power_sensor_ = instant_power_sensor; }
			void set_instant_current_sensor(sensor::Sensor *instant_current_sensor) { instant_current_sensor_ = instant_current_sensor; }
			void set_instant_voltage_sensor(sensor::Sensor *instant_voltage_sensor) { instant_voltage_sensor_ = instant_voltage_sensor; }
			
			void set_tank_mode_select(select::Select *tank_mode_select) { tank_mode_select_ = tank_mode_select; }
			
			void set_volt_factor(float volt_factor) { this->volt_factor_ = volt_factor; };
			void set_current_factor(float current_factor) { this->current_factor_ = current_factor; };
			void set_min_triac_delay(int min_triac_delay) { this->min_triac_delay_ = min_triac_delay; };
			void set_power_margin(int power_margin) { this->power_margin_ = power_margin; };
			void set_triac_load_step(int triac_load_step) { this->triac_load_step_ = triac_load_step; }
			void loop() override;
			void setup() override;
			void update() override;
			float get_setup_priority() const override { return esphome::setup_priority::BUS; }

		protected:
			GPIOPin *active_pin_;
			GPIOPin *conso_pin_;
			GPIOPin *inject_pin_;
			InternalGPIOPin *zero_crossing_pin_;
			GPIOPin *triac_pin_;
			InternalGPIOPin *analog_ref_pin_;
			InternalGPIOPin *analog_volt_pin_;
			InternalGPIOPin *analog_current_pin_;
			
			binary_sensor::BinarySensor *over_production_sensor_;
			binary_sensor::BinarySensor *power_connected_sensor_;
			
			sensor::Sensor *tank_temperature_sensor_;
			number::Number *tank_target_temperature_sensor_;
			sensor::Sensor *triac_sensor_;
			sensor::Sensor *consumption_sensor_;
			sensor::Sensor *instant_power_sensor_;
			sensor::Sensor *instant_current_sensor_;
			sensor::Sensor *instant_voltage_sensor_;
			
			select::Select *tank_mode_select_;
			
			float volt_factor_;
			float current_factor_;
			int min_triac_delay_;
			int power_margin_;
			int triac_load_step_;
			float last_tank_temperature_ = 999;
            int last_tank_temperature_millis_ = 0;

			Measure* measure;
			
			static void critical_worker(void* _this);

		private:
			unsigned long previous_compute_millis = 0;
			unsigned long previous_blink_millis = 0;
			bool blink = false;
			void init_sequence();
			unsigned int get_tank_mode();
		};
	}  // namespace pvrouter
}  // namespace esphome