# Pentair IntelliFlo → ESPHome → Home Assistant

ESPHome external component for Pentair IntelliFlo pumps connected over the Pentair RS-485 automation bus.

The component supports multiple addressed pumps on a single shared RS-485 bus. One ESPHome controller owns the UART, receive parser and transmit queue; received frames are dispatched to the matching pump by source address.

## Multi-pump architecture

Pentair pump addresses 1 through 16 map to wire addresses `0x60` through `0x6F`:

| Pump address | Wire address |
| ---: | ---: |
| 1 | `0x60` |
| 2 | `0x61` |
| 3 | `0x62` |
| ... | ... |
| 16 | `0x6F` |

All pumps may share the same two-wire RS-485 bus. Each pump must have a unique address configured on the pump itself and the same address in ESPHome.

```text
ESP32 / RS-485 controller
        |
        +---- Pump 1 (address 1 / 0x60)
        |
        +---- Pump 2 (address 2 / 0x61)
        |
        +---- ...
```

The controller performs all bus I/O centrally. Pump objects contain only pump-specific state, entities, addressing and commands.

## Install

For development/testing of the multi-pump branch:

```yaml
external_components:
  - source:
      type: git
      url: https://github.com/XMaarten/ESPHome-Pentair-Intelliflo
      ref: feature/multi-pump-controller
    components: [pentair_intelliflo]
    refresh: 0s
```

After the multi-pump changes are merged upstream, the source should point back to the upstream repository and `main` branch.

## Controller configuration

A single pump:

```yaml
pentair_intelliflo:
  id: pentair_bus
  uart_id: rs485
  update_interval: 20s
  pumps:
    - id: pump_1
      address: 1
```

Two pumps on the same bus:

```yaml
pentair_intelliflo:
  id: pentair_bus
  uart_id: rs485
  update_interval: 20s
  pumps:
    - id: pump_1
      address: 1
    - id: pump_2
      address: 2
```

Addresses must be unique on a controller. ESPHome configuration validation rejects duplicate addresses.

## UART configuration

The Pentair automation bus uses 9600 baud, 8 data bits, no parity and one stop bit:

```yaml
uart:
  id: rs485
  tx_pin: GPIO22
  rx_pin: GPIO21
  baud_rate: 9600
  data_bits: 8
  parity: NONE
  stop_bits: 1
  rx_buffer_size: 512
```

The GPIO values above are for the LILYGO T-CAN485 example configuration. Adapt them for other RS-485 hardware.

## LILYGO T-CAN485 pins

| Signal | GPIO | Notes |
| --- | --- | --- |
| RS485 TX | 22 | |
| RS485 RX | 21 | |
| `PIN_5V_EN` | 16 | Must be HIGH; powers the transceiver |
| `RS485_EN` | 17 | Must be HIGH |
| `RS485_SE` | 19 | Must be HIGH; enables automatic TX/RX turnaround |

The T-CAN485 handles direction switching itself, so no separate DE pin needs to be toggled around writes.

## Pump entities

Entity platforms reference an individual pump using `pump_id`.

```yaml
sensor:
  - platform: pentair_intelliflo
    pump_id: pump_1
    power:
      name: "Pump 1 Power"
    rpm:
      name: "Pump 1 Speed"

  - platform: pentair_intelliflo
    pump_id: pump_2
    power:
      name: "Pump 2 Power"
    rpm:
      name: "Pump 2 Speed"
```

The same applies to binary and text sensors:

```yaml
binary_sensor:
  - platform: pentair_intelliflo
    pump_id: pump_1
    running:
      name: "Pump 1 Running"
    remote_control:
      name: "Pump 1 Remote Control"

text_sensor:
  - platform: pentair_intelliflo
    pump_id: pump_1
    program:
      name: "Pump 1 Program"
    pump_state:
      name: "Pump 1 Drive State"
    error:
      name: "Pump 1 Error"
```

Available status entities include power, RPM, flow, filter-cycle percentage, error code, time remaining, running state, remote-control state, program, drive state and error text.

Plain IntelliFlo VS pumps generally report zero for flow; VF/VSF models may report an actual flow value. The field previously interpreted as pressure is exposed as filter-cycle percentage instead.

## Pump commands

Pump IDs can be used directly from ESPHome lambdas:

```yaml
button:
  - platform: template
    name: "Start pump 1"
    on_press:
      - lambda: |-
          id(pump_1).set_remote_control(true);
          id(pump_1).set_speed_rpm(1800);
          id(pump_1).set_pump_running(true);

  - platform: template
    name: "Stop pump 2"
    on_press:
      - lambda: |-
          id(pump_2).set_pump_running(false);
```

Useful pump methods include:

- `request_status()`
- `set_remote_control(bool)`
- `set_pump_running(bool)`
- `set_speed_rpm(uint16_t)`
- `set_speed_gpm(uint8_t)`
- `set_program_speed(program, rpm)`
- `run_program(program)`
- `set_speed_index(index)`

## Bus handling

Every command uses the Pentair `0xA5` automation-bus frame format. The configured pump address determines the destination byte. For pump address 1 the destination is `0x60`; for pump address 2 it is `0x61`, and so on.

The controller has one receive parser and one transmit queue for the entire bus. Status requests for all registered pumps are queued during the polling interval and transmitted one at a time using the existing RS-485 bus timing and quiet-time handling.

Frames received from registered pump addresses are routed to the corresponding pump object. Frames from unregistered addresses and controller self-echoes are ignored.

## Single-pump full example

`pentair-intelliflo.yaml` contains the original full Home Assistant control example adapted to the controller/pump API. It still configures one pump, which makes it suitable for initial testing of the refactor before connecting multiple pumps.

`multipump-example.yaml` contains a smaller two-pump example focused on the shared-bus configuration.

## Protocol references

- [Controlling an IntelliFlo pump from Home Assistant](https://www.yoctopuce.com/EN/article/controlling-an-intelliflo-pump-from-home-assistant)
- [nodejs-poolController wiki: Pumps](https://github.com/tagyoureit/nodejs-poolController/wiki/Pumps)

## Credits

This project builds on the original work by [nicostrown](https://github.com/nicostrown/ESPHome-Pentair-Intelliflo).

Additional protocol, ESPHome compatibility, parser and pump-control improvements were contributed through the work of [gamer22026](https://github.com/gamer22026/ESPHome-Pentair-Intelliflo).

Support and findings for older IntelliFlo VS pumps were also informed by the work of [jostd](https://github.com/jostd/ESPHome-Pentair-Intelliflo-VS).

Thanks to all contributors and community members who reverse-engineered and documented the Pentair RS-485 protocol.
