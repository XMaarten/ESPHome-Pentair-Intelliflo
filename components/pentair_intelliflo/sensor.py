import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import sensor
from esphome.const import (
    DEVICE_CLASS_POWER,
    DEVICE_CLASS_VOLUME_FLOW_RATE,
    DEVICE_CLASS_PRESSURE,
    UNIT_WATT,
    UNIT_REVOLUTIONS_PER_MINUTE,
    UNIT_CUBIC_METER_PER_HOUR,
)
from . import INTELLIFLO_CHILD_SCHEMA, CONF_INTELLIFLO_ID

DEPENDENCIES = ["pentair_intelliflo"]

#sensors
CONF_POWER = "power"
CONF_RPM = "rpm"

CONFIG_SCHEMA = cv.Schema(
    {
        cv.Optional(CONF_POWER): sensor.sensor_schema(
            unit_of_measurement=UNIT_WATT,
            accuracy_decimals=0,
            device_class=DEVICE_CLASS_POWER,
        ),
        cv.Optional(CONF_RPM): sensor.sensor_schema(
            unit_of_measurement=UNIT_REVOLUTIONS_PER_MINUTE,
            accuracy_decimals=0,
        ),
    }
).extend(INTELLIFLO_CHILD_SCHEMA)


async def to_code(config):
    var = await cg.get_variable(config[CONF_INTELLIFLO_ID])

    if power_config := config.get(CONF_POWER):
        sens = await sensor.new_sensor(power_config)
        cg.add(var.set_power(sens))

    if rpm_config := config.get(CONF_RPM):
        sens = await sensor.new_sensor(rpm_config)
        cg.add(var.set_rpm(sens))
