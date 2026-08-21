#pragma once

#include "esphome/components/binary_sensor/binary_sensor.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/text_sensor/text_sensor.h"
#include "esphome/components/uart/uart.h"
#include "esphome/core/component.h"
#include <deque>
#include <string>
#include <vector>

namespace esphome {
namespace pentair_intelliflo {

// Byte [0] of the 0x07 status payload.
static const uint8_t PUMP_STOPPED = 0x04;
static const uint8_t PUMP_STARTED = 0x0A;

class PentairIntelliflo;

/**
 * Represents one physical IntelliFlo pump on the Pentair RS-485 bus.
 *
 * The user-facing address is 1-16. Pentair encodes those addresses on the wire
 * as 0x60-0x6F. The pump owns its state and Home Assistant entities; bus I/O,
 * framing and arbitration are owned by the parent PentairIntelliflo controller.
 */
class PentairIntellifloPump {
 public:
  explicit PentairIntellifloPump(PentairIntelliflo *parent) : parent_(parent) {}

  void set_address(uint8_t address) {
    this->address_ = address;
    this->wire_address_ = 0x5F + address;
  }
  uint8_t get_address() const { return this->address_; }
  uint8_t get_wire_address() const { return this->wire_address_; }

  void set_power_sensor(sensor::Sensor *s) { this->power_ = s; }
  void set_rpm_sensor(sensor::Sensor *s) { this->rpm_ = s; }
  void set_flow_sensor(sensor::Sensor *s) { this->flow_ = s; }
  void set_filter_percent_sensor(sensor::Sensor *s) { this->filter_percent_ = s; }
  void set_error_code_sensor(sensor::Sensor *s) { this->error_code_ = s; }
  void set_time_remaining_sensor(sensor::Sensor *s) { this->time_remaining_ = s; }
  void set_running_binary_sensor(binary_sensor::BinarySensor *s) { this->running_ = s; }
  void set_remote_control_binary_sensor(binary_sensor::BinarySensor *s) { this->remote_control_ = s; }
  void set_program_text_sensor(text_sensor::TextSensor *s) { this->program_ = s; }
  void set_pump_state_text_sensor(text_sensor::TextSensor *s) { this->pump_state_ = s; }
  void set_error_text_sensor(text_sensor::TextSensor *s) { this->error_ = s; }

  // Commands. These queue a frame on the parent controller and never block.
  void request_status();
  void set_remote_control(bool remote);
  void set_pump_running(bool running);
  void set_speed_rpm(uint16_t rpm);
  void set_speed_gpm(uint8_t gpm);
  void set_program_speed(uint8_t program, uint16_t rpm);
  void run_program(uint8_t program);
  void set_speed_index(uint8_t index);

  // Last known state, for use from lambdas.
  bool is_running() const { return this->running_state_; }
  bool is_remote_control() const { return this->remote_state_; }
  uint16_t current_rpm() const { return this->rpm_state_; }
  uint16_t current_watts() const { return this->watts_state_; }

  void dump_config();
  void handle_frame(uint8_t command, const uint8_t *payload, uint8_t length);

 protected:
  void publish_status_(const uint8_t *payload);

  PentairIntelliflo *parent_;
  uint8_t address_{1};
  uint8_t wire_address_{0x60};

  bool running_state_{false};
  bool remote_state_{false};
  uint16_t rpm_state_{0};
  uint16_t watts_state_{0};

  sensor::Sensor *power_{nullptr};
  sensor::Sensor *rpm_{nullptr};
  sensor::Sensor *flow_{nullptr};
  sensor::Sensor *filter_percent_{nullptr};
  sensor::Sensor *error_code_{nullptr};
  sensor::Sensor *time_remaining_{nullptr};
  binary_sensor::BinarySensor *running_{nullptr};
  binary_sensor::BinarySensor *remote_control_{nullptr};
  text_sensor::TextSensor *program_{nullptr};
  text_sensor::TextSensor *pump_state_{nullptr};
  text_sensor::TextSensor *error_{nullptr};
};

/**
 * Owns one Pentair RS-485 bus, including UART I/O, frame parsing, TX queue and
 * bus timing. Multiple addressed pumps can be registered on the same bus.
 */
class PentairIntelliflo : public PollingComponent, public uart::UARTDevice {
 public:
  void setup() override;
  void loop() override;
  void update() override;
  void dump_config() override;

  void register_pump(PentairIntellifloPump *pump) { this->pumps_.push_back(pump); }

 protected:
  friend class PentairIntellifloPump;

  void queue_command_(uint8_t wire_address, uint8_t command, const std::vector<uint8_t> &payload);
  void feed_byte_(uint8_t byte);
  bool resync_();
  void handle_frame_(size_t total);
  PentairIntellifloPump *find_pump_(uint8_t wire_address);

  std::vector<PentairIntellifloPump *> pumps_;
  std::vector<uint8_t> rx_;
  std::deque<std::vector<uint8_t>> tx_queue_;
  uint32_t last_rx_ms_{0};
  uint32_t last_tx_ms_{0};
};

}  // namespace pentair_intelliflo
}  // namespace esphome
