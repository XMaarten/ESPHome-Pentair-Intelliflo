import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import switch
from . import INTELLIFLO_CHILD_SCHEMA, CONF_INTELLIFLO_ID
from esphome.const import (
    DEVICE_CLASS_RUNNING,
)

DEPENDENCIES = ["pentair_intelliflo"]

IntellifloSwitch = intelliflo_ns.class_(
    "IntellifloSwitch", cg.Component, switch.Switch  # , SensorItem
)

CONFIG_SCHEMA = cv.All(
    switch.SWITCH_SCHEMA.extend(cv.COMPONENT_SCHEMA)
    .extend(INTELLIFLO_CHILD_SCHEMA)
    .extend(
        {
            cv.GenerateID(): cv.declare_id(IntellifloSwitch),
        }
    ),
)

async def to_code(config):
    var = cg.new_Pvariable(
        config[CONF_ID],
    )
    await cg.register_component(var, config)
    await switch.register_switch(var, config)

    paren = await cg.get_variable(config[CONF_INTELLIFLO_ID])
    cg.add(var.set_pump(paren))
    cg.add(paren.add_item(var))
    await add_intelliflo_base_properties(var, config, IntellifloSwitch)


async def to_code(config):
    var = await cg.get_variable(config[CONF_INTELLIFLO_ID])

    if running_config := config.get(CONF_RUNNING):
        sens = await binary_sensor.new_binary_sensor(running_config)
        cg.add(var.set_running(sens))
