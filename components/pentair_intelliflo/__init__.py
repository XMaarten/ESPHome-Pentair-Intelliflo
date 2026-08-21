import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import uart
from esphome.const import CONF_ADDRESS, CONF_ID

DEPENDENCIES = ["uart"]
AUTO_LOAD = ["sensor", "binary_sensor", "text_sensor"]
MULTI_CONF = True

CONF_PUMPS = "pumps"
CONF_PUMP_ID = "pump_id"

pentair_intelliflo_ns = cg.esphome_ns.namespace("pentair_intelliflo")
PentairIntelliflo = pentair_intelliflo_ns.class_(
    "PentairIntelliflo", cg.PollingComponent, uart.UARTDevice
)
PentairIntellifloPump = pentair_intelliflo_ns.class_("PentairIntellifloPump")

PUMP_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(PentairIntellifloPump),
        cv.Required(CONF_ADDRESS): cv.int_range(min=1, max=16),
    }
)


def _validate_pumps(pumps):
    addresses = [pump[CONF_ADDRESS] for pump in pumps]
    if len(addresses) != len(set(addresses)):
        raise cv.Invalid("Pump addresses must be unique on an RS-485 bus")
    return pumps


CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(PentairIntelliflo),
            cv.Required(CONF_PUMPS): cv.All(
                cv.ensure_list(PUMP_SCHEMA),
                cv.Length(min=1, max=16),
                _validate_pumps,
            ),
        }
    )
    .extend(cv.polling_component_schema("20s"))
    .extend(uart.UART_DEVICE_SCHEMA)
)

PENTAIR_INTELLIFLO_PUMP_CHILD_SCHEMA = cv.Schema(
    {
        cv.GenerateID(CONF_PUMP_ID): cv.use_id(PentairIntellifloPump),
    }
)

FINAL_VALIDATE_SCHEMA = uart.final_validate_device_schema(
    "pentair_intelliflo", baud_rate=9600, require_tx=True, require_rx=True
)


async def to_code(config):
    controller = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(controller, config)
    await uart.register_uart_device(controller, config)

    for pump_config in config[CONF_PUMPS]:
        pump = cg.new_Pvariable(pump_config[CONF_ID], controller)
        cg.add(pump.set_address(pump_config[CONF_ADDRESS]))
        cg.add(controller.register_pump(pump))
