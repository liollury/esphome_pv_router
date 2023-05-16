#include "const.h"
#include "measure.h"
#include "esphome/core/log.h"
#include "esphome/core/helpers.h"

 
namespace esphome {
	namespace pvrouter {
		int sq(int value) { return value * value; }
		
		Measure* instance;

		void IRAM_ATTR on_timer_interrupt_handler() {
			instance->on_timer_interrupt();
		}

		Measure::Measure(
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
		) {
			instance = this;
			this->zero_crossing_pin_ = zero_crossing_pin;
			this->triac_pin_ = triac_pin;
			this->analog_ref_pin_ = analog_ref_pin;
			this->analog_volt_pin_ = analog_volt_pin;
			this->analog_current_pin_ = analog_current_pin;
			this->previous_compute_millis = millis();
			this->volt_factor_ = volt_factor;
			this->current_factor_ = current_factor;
			this->min_triac_delay_ = min_triac_delay;
			this->power_margin_ = power_margin;
			this->triac_load_step_ = triac_load_step;
		}

		void Measure::setup() {
			this->zero_crossing_pin_->setup();
			this->triac_pin_->setup();
			this->triac_pin_->digital_write(false);

			// this->zero_crossing_pin_->attach_interrupt(zero_cross_interrupt, this, gpio::INTERRUPT_RISING_EDGE); // RISING
			// remplacer par une version "a la main" qui a une priorité plus élevé
			auto res = gpio_install_isr_service(ESP_INTR_FLAG_IRAM);
			if (res != ESP_OK) {
			  ESP_LOGE(TAG, "call to gpio_install_isr_service() failed, error code: %d", res);
			  return;
			}
			
			gpio_set_intr_type((gpio_num_t)this->zero_crossing_pin_->get_pin(), GPIO_INTR_POSEDGE); // RISING
			gpio_intr_enable((gpio_num_t)this->zero_crossing_pin_->get_pin());
			gpio_isr_handler_add((gpio_num_t)this->zero_crossing_pin_->get_pin(), zero_cross_interrupt, (void*) this);
			
			this->timer = timerBegin(0, 80, true);    //Clock Divider, 1 micro second Tick
			timerAttachInterrupt(this->timer, on_timer_interrupt_handler, true);
			timerAlarmWrite(this->timer, 100, true);    //Interrupt every 100 Ticks or microsecond
			timerAlarmEnable(this->timer);
		}

		void IRAM_ATTR Measure::zero_cross_interrupt(void *_this) {
			if ((millis() - ((Measure*)_this)->last_zero_cross_interruption) > 2) {    // to avoid glitch detection during 2ms
				((Measure*)_this)->current_triac_position = 0; //Time synchro every 10ms
				((Measure*)_this)->last_zero_cross_interruption = millis();
				((Measure*)_this)->triac_pin_->digital_write(false);    //Stop Triac
			}
		}

		void IRAM_ATTR Measure::on_timer_interrupt() {    //Interruption every 100 micro second
			this->current_triac_position += 1;
			if (this->current_triac_position > this->triac_delay && this->triac_delay < 98 && this->is_power_connected && !this->sync_lost) {    //100 steps in 10 ms
				this->triac_pin_->digital_write(true); //Activate Triac
			}
		}

		void Measure::measure_power() {
			int index;
			int ref_volt = analogRead(this->analog_ref_pin_->get_pin());
			unsigned long measure_millis = millis();
			int volt_pin = this->analog_volt_pin_->get_pin();
			int current_pin = this->analog_current_pin_->get_pin();
			//Reset table measurements
			for (int i = 0; i < 100; i++) {
				this->volt[i] = 0;
				this->amp[i] = 0;
			}

			while (millis() - measure_millis < 21) {                                                         //Read values in continuous during 20ms. One loop is around 150 micro seconds
				index = (micros() % 20000) / 200;    //We have more results that we need during 20ms to fill the tables of 100 samples
				this->volt[index] = analogRead(volt_pin) - ref_volt;
				this->amp[index] = analogRead(current_pin) - ref_volt;
			}
		}


		void Measure::compute_power() {
		    float V, I;
			float volt_acc = 0;
			float current_acc = 0;
			int active_values = 0;
			this->pW = 0;
			for (int i = 0; i < 100; i++) {
				if (this->volt[i] != 0 || this->amp[i] != 0) {
				    active_values++;
                    V = this->volt_factor_ * this->volt[i];
                    volt_acc += sq(V);
                    I = this->current_factor_ * this->amp[i];
                    current_acc += sq(I);
                    this->pW += V * I;
				}
			}
			this->voltage = (19 * this->voltage + sqrt(volt_acc / active_values)) / 20;
			this->current = (19 * this->current + sqrt(current_acc / active_values)) / 20;
			this->is_power_connected = (this->voltage > 190 && this->voltage < 280);
			if (this->is_power_connected) {
				this->pW = floor(this->pW / active_values);
				this->pVA = floor(this->voltage * this->current);
				this->Wh += this->pW / 90000;    // Watt Hour, Every 40ms
			} else {
				//ESP_LOGI(TAG, "Power signal lost (%fV)", this->voltage);
				this->pW = 0;
				this->pVA = 0;
			}
			// ESP_LOGI(TAG, "%fW", this->pW);
		}

		void Measure::compute_triac_delay() {
			if (this->heating_allowed && this->is_power_connected) {        
				float delay = this->triac_delay + (this->pW + this->power_margin_) / this->triac_load_step_;    //Decrease/Increase delay to increase/Decrease Triac conduction
				if (delay < this->min_triac_delay_ || this->force_heating) { 
					delay = this->min_triac_delay_; 
				} else if (delay > 100) {
					delay = 100; 
				}
				this->over_production = delay < 100;
				this->triac_delay = int(delay);
			} else {
				this->triac_pin_->digital_write(false);
				this->triac_delay = 100;
			}
		}

		void Measure::update() {
			if (millis() - this->previous_compute_millis > 40) {				
				this->previous_compute_millis = millis();
				this->measure_power();
				this->compute_power();
				this->compute_triac_delay();
				if (millis() - this->last_zero_cross_interruption > 500) {
					if (!this->sync_lost) {
						ESP_LOGI(TAG, "Zero crossing sync lost");
					}
					this->sync_lost = true;
					this->stop_triac();
				} else {
					if (this->sync_lost) {
						ESP_LOGI(TAG, "Zero crossing sync restored");
					}
					this->sync_lost = false;            
				}
			}
		}

		void Measure::reset_wh() {
			this->Wh = 0;
		}

		void Measure::stop_triac() {
			this->triac_pin_->digital_write(false);
		}
	}
}