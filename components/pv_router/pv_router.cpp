#include "pv_router.h"
#include "const.h"
#include "esphome/core/log.h"

namespace esphome {
	namespace pvrouter {
		TaskHandle_t critical_thread_task;

		bool is_valid_temperature(float raw_temperature) {
			if (isnan(raw_temperature) || raw_temperature < -40.0f) {
                return false;
            } else {
                return true;
            }
        }
		
		float get_valid_temperature(float raw_temperature) {
			if (!is_valid_temperature(raw_temperature)) {
				return 999.;
			} else {
				return raw_temperature;
			}
		}
		
		unsigned int PvRouter::get_tank_mode() {
			return this->tank_mode_select_->index_of(this->tank_mode_select_->state).value();
		}
		
		void PvRouter::setup() {
			ESP_LOGI(TAG, "Initialize PV Router on core %d", xPortGetCoreID());
			this->active_pin_->setup();
			this->conso_pin_->setup();
			this->inject_pin_->setup();
			this->init_sequence();
			this->measure = new Measure(
				this->zero_crossing_pin_, 
				this->triac_pin_, 
				this->analog_ref_pin_, 
				this->analog_volt_pin_, 
				this->analog_current_pin_,
				this->volt_factor_,
				this->current_factor_,
				this->min_triac_delay_,
				this->power_margin_,
				this->triac_load_step_
			);
			this->measure->setup();
			xTaskCreatePinnedToCore(critical_worker, "Critical thread", 10000, (void*)this, 0, &critical_thread_task, 0);
		}
		
		
		void PvRouter::critical_worker(void *_this) {
			ESP_LOGI(TAG, "Critical thread created on core %d", xPortGetCoreID());
			vTaskDelay(2000);
			// loop
			for(;;) {
				((PvRouter*)_this)->measure->update();
				vTaskDelay(20);
			}
			vTaskDelete(critical_thread_task);
		}
		
		void PvRouter::update() {
			this->triac_sensor_->publish_state(100 - this->measure->triac_delay);
			this->over_production_sensor_->publish_state(this->measure->over_production);
			this->power_connected_sensor_->publish_state(this->measure->is_power_connected);
			if (!this->measure->is_power_connected) {
				this->consumption_sensor_->publish_state(NAN);
				this->instant_power_sensor_->publish_state(NAN);
				this->instant_current_sensor_->publish_state(NAN);
				this->instant_voltage_sensor_->publish_state(NAN);
			} else {
				this->consumption_sensor_->publish_state(this->measure->Wh / 1000);
				this->instant_power_sensor_->publish_state(this->measure->pW);
				this->instant_current_sensor_->publish_state(this->measure->current);
				this->instant_voltage_sensor_->publish_state(this->measure->voltage);
				
			}
		}
		
		void PvRouter::loop() {
			// this->measure->update();
			if(millis() - this->previous_compute_millis > 1000) {
				this->previous_compute_millis = millis();
				int tank_mode = this->get_tank_mode();
				float current_temperature = get_valid_temperature(float(this->tank_temperature_sensor_->state));
				if (is_valid_temperature(float(this->tank_temperature_sensor_->state))) {
                	this->last_tank_temperature_ = current_temperature;
                	this->last_tank_temperature_millis_ = millis();
                } else if (millis() - last_tank_temperature_millis_ < 60000) {
                    current_temperature = this->last_tank_temperature_;
				}
				float target_temperature = get_valid_temperature(float(this->tank_target_temperature_sensor_->state));
				this->measure->heating_allowed = (tank_mode & TANK_MODE_ON_MASK) > 0 && current_temperature < target_temperature;
				this->measure->force_heating = tank_mode == TANK_MODE_ON;
				//ESP_LOGD(TAG, "POWER %f TRIAC %d", this->measure->pW, 100 - this->measure->triac_delay);
			}
			
			if (millis() - previous_blink_millis >= 2000) {
				this->blink = !this->blink;
				if (this->blink) {
					previous_blink_millis = millis() - 1900;
				} else {
					previous_blink_millis = millis();
				}
				if (!this->measure->is_power_connected) {
					this->conso_pin_->digital_write(this->blink);
					this->inject_pin_->digital_write(this->blink);
				} else if (this->measure->over_production) {
					this->conso_pin_->digital_write(false);
					this->inject_pin_->digital_write(this->blink);
				} else {
					this->conso_pin_->digital_write(this->blink);
					this->inject_pin_->digital_write(false);
				}
				if ((this->get_tank_mode() & TANK_MODE_ON_MASK) > 0) {
					this->active_pin_->digital_write(true); 
				} else {
					this->active_pin_->digital_write(false);
				}
			}
		}
		
		PvRouter::PvRouter() {
			
		}
		
		
		// private
		
		
		void PvRouter::init_sequence() {
		  this->active_pin_->digital_write(true);
		  delay(500);
		  this->active_pin_->digital_write(false);
		  this->conso_pin_->digital_write(true);
		  delay(500);
		  this->conso_pin_->digital_write(false);
		  this->inject_pin_->digital_write(true);
		  delay(500);
		  this->inject_pin_->digital_write(false);
		}
		
	}  // namespace pvrouter
}  // namespace esphome