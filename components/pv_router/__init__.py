import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import pins
from esphome.const import (
    CONF_ID, 
    CONF_PIN,
    CONF_ADDRESS,
    CONF_DALLAS_ID,
    CONF_INDEX,
    CONF_RESOLUTION,
    DEVICE_CLASS_TEMPERATURE,
    DEVICE_CLASS_POWER_FACTOR,
    STATE_CLASS_MEASUREMENT,
    STATE_CLASS_TOTAL_INCREASING,
    DEVICE_CLASS_ENERGY,
    UNIT_CELSIUS,
    DEVICE_CLASS_POWER,
    UNIT_WATT,
)
from esphome.components import sensor, select, binary_sensor, number

MULTI_CONF = True
AUTO_LOAD = ["sensor"]  
CONF_ACTIVE_LED_PIN = "active_led"
CONF_CONSO_LED_PIN = "conso_led"
CONF_INJECT_LED_PIN = "inject_led"
CONF_ZERO_CROSSING_PIN = "zero_crossing"
CONF_ANALOG_REF_PIN = "vcc_ref"
CONF_VOLT_REF_PIN = "volt_ref"
CONF_CURRENT_REF_PIN = "current_ref"
CONF_TANK_TEMPERATURE_ID = "tank_temperature"
CONF_TANK_TARGET_TEMPERATURE_ID = "tank_target_temperature"
CONF_TANK_MODE_ID = "tank_mode"
CONF_VOLT_FACTOR = "volt_factor"
CONF_CURRENT_FACTOR = "current_factor"
CONF_POWER_MARGIN = "power_margin"
CONF_TRIAC="triac"
CONF_TRIAC_LOAD_STEP = "load_step"
CONF_MIN_TRIAC_DELAY = "min_delay"
CONF_TRIAC_PIN = "pin"
CONF_VALUE = "value"
CONF_OVER_PRODUCTION = "over_production"
CONF_POWER_CONNECTED = "power_connecter"
CONF_CONSUMPTION = "consumption"
CONF_INSTANT_POWER = "instant_power"
CONF_INSTANT_CURRENT = "instant_current"
CONF_INSTANT_VOLTAGE = "instant_voltage"

pvrouter_ns = cg.esphome_ns.namespace("pvrouter")
PvRouter = pvrouter_ns.class_("PvRouter", cg.PollingComponent)

def validate_internal_pin(value):
    value = pins.internal_gpio_input_pin_number(value)
    return pins.internal_gpio_input_pin_schema(value)


CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(PvRouter),
        cv.Required(CONF_ACTIVE_LED_PIN): pins.gpio_output_pin_schema,
        cv.Required(CONF_CONSO_LED_PIN): pins.gpio_output_pin_schema,
        cv.Required(CONF_INJECT_LED_PIN): pins.gpio_output_pin_schema,
        cv.Required(CONF_ZERO_CROSSING_PIN): pins.internal_gpio_input_pin_schema,
        cv.Required(CONF_ANALOG_REF_PIN): validate_internal_pin,
        cv.Required(CONF_VOLT_REF_PIN): validate_internal_pin,
        cv.Required(CONF_CURRENT_REF_PIN): validate_internal_pin,
        cv.Required(CONF_TANK_TEMPERATURE_ID): cv.use_id(sensor.Sensor),
        cv.Required(CONF_TANK_MODE_ID): cv.use_id(select.Select),
        cv.Required(CONF_TANK_TARGET_TEMPERATURE_ID): cv.use_id(number.Number),
        cv.Required(CONF_TRIAC): cv.All(
            sensor.sensor_schema(
                accuracy_decimals=0,
                device_class=DEVICE_CLASS_POWER_FACTOR,
                state_class=STATE_CLASS_MEASUREMENT
            ).extend(
                {
                    cv.Optional(CONF_TRIAC_LOAD_STEP, 100): cv.int_range(0, 1000),
                    cv.Optional(CONF_MIN_TRIAC_DELAY, 50): cv.int_range(0, 100),
                    cv.Required(CONF_TRIAC_PIN): pins.gpio_output_pin_schema,
                }
            )
        ),
        cv.Required(CONF_OVER_PRODUCTION): cv.All(binary_sensor.binary_sensor_schema()),
        cv.Required(CONF_POWER_CONNECTED): cv.All(binary_sensor.binary_sensor_schema()),
        cv.Required(CONF_CONSUMPTION): cv.All(sensor.sensor_schema(unit_of_measurement='kwh', accuracy_decimals=2, device_class=DEVICE_CLASS_ENERGY, state_class=STATE_CLASS_TOTAL_INCREASING)),
        cv.Required(CONF_INSTANT_POWER): cv.All(sensor.sensor_schema(unit_of_measurement=UNIT_WATT, accuracy_decimals=0, device_class=DEVICE_CLASS_POWER, state_class=STATE_CLASS_MEASUREMENT)),
        cv.Required(CONF_INSTANT_CURRENT): cv.All(sensor.sensor_schema(unit_of_measurement='A', accuracy_decimals=0, device_class=DEVICE_CLASS_POWER, state_class=STATE_CLASS_MEASUREMENT)),
        cv.Required(CONF_INSTANT_VOLTAGE): cv.All(sensor.sensor_schema(unit_of_measurement='V', accuracy_decimals=0, device_class=DEVICE_CLASS_POWER, state_class=STATE_CLASS_MEASUREMENT)),
        cv.Required(CONF_VOLT_FACTOR): cv.float_range(0),
        cv.Required(CONF_CURRENT_FACTOR): cv.float_range(0),
        cv.Optional(CONF_POWER_MARGIN, 0): cv.int_range(0, 1000),
    }
).extend(cv.polling_component_schema("30s"))


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    pin = await cg.gpio_pin_expression(config[CONF_ACTIVE_LED_PIN])
    cg.add(var.set_active_pin(pin))
    pin = await cg.gpio_pin_expression(config[CONF_CONSO_LED_PIN])
    cg.add(var.set_conso_pin(pin))
    pin = await cg.gpio_pin_expression(config[CONF_INJECT_LED_PIN])
    cg.add(var.set_inject_pin(pin))
    pin = await cg.gpio_pin_expression(config[CONF_ZERO_CROSSING_PIN])
    cg.add(var.set_zero_crossing_pin(pin))
    pin = await cg.gpio_pin_expression(config[CONF_ANALOG_REF_PIN])
    cg.add(var.set_analog_ref_pin(pin))
    pin = await cg.gpio_pin_expression(config[CONF_VOLT_REF_PIN])
    cg.add(var.set_analog_volt_pin(pin))
    pin = await cg.gpio_pin_expression(config[CONF_CURRENT_REF_PIN])
    cg.add(var.set_analog_current_pin(pin))

    sens = await cg.get_variable(config[CONF_TANK_TEMPERATURE_ID])
    cg.add(var.set_tank_temperature_sensor(sens))
    sens = await cg.get_variable(config[CONF_TANK_TARGET_TEMPERATURE_ID])
    cg.add(var.set_tank_target_temperature_sensor(sens))
    sens = await cg.get_variable(config[CONF_TANK_MODE_ID])
    cg.add(var.set_tank_mode_select(sens))
    val = config[CONF_VOLT_FACTOR]
    cg.add(var.set_volt_factor(val))
    val = config[CONF_CURRENT_FACTOR]
    cg.add(var.set_current_factor(val))
    val = config[CONF_POWER_MARGIN]
    cg.add(var.set_power_margin(val))
    
    sens = await binary_sensor.new_binary_sensor(config[CONF_OVER_PRODUCTION])
    cg.add(var.set_over_production_sensor(sens))
    sens = await binary_sensor.new_binary_sensor(config[CONF_POWER_CONNECTED])
    cg.add(var.set_power_connected_sensor(sens))
    
    sens = await sensor.new_sensor(config[CONF_CONSUMPTION])
    cg.add(var.set_consumption_sensor(sens))
    sens = await sensor.new_sensor(config[CONF_INSTANT_POWER])
    cg.add(var.set_instant_power_sensor(sens))
    sens = await sensor.new_sensor(config[CONF_INSTANT_CURRENT])
    cg.add(var.set_instant_current_sensor(sens))
    sens = await sensor.new_sensor(config[CONF_INSTANT_VOLTAGE])
    cg.add(var.set_instant_voltage_sensor(sens))
    
    sens = await sensor.new_sensor(config[CONF_TRIAC])
    cg.add(var.set_triac_sensor(sens))
    pin = await cg.gpio_pin_expression(config[CONF_TRIAC][CONF_TRIAC_PIN])
    cg.add(var.set_triac_pin(pin))
    val = config[CONF_TRIAC][CONF_MIN_TRIAC_DELAY]
    cg.add(var.set_min_triac_delay(val))
    val = config[CONF_TRIAC][CONF_TRIAC_LOAD_STEP]
    cg.add(var.set_triac_load_step(val))
