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
 * The pump owns its address, state and Home Assistant entities. Bus I/O,
 * framing and arbitration are owned by the parent PentairIntelliflo controller.
 */
class PentairIntellifloPump {
 public:
  explicit PentairIntellifloPump(PentairIntelliflo *parent) : parent_(parent) {}

  void set_address(uint8_t address) { this->address_ = address; }
  uint8_t get_address() const { return this->address_; }

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
  uint8_t address_{0x60};

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
 * Owns the Pentair RS-485 bus, frame parser, TX queue and bus timing.
 *
 * For now the public API still exposes one pump so this refactor is backwards
 * compatible. Multi-pump registration can be added in a separate change.
 */
class PentairIntelliflo : public PollingComponent, public uart::UARTDevice {
 public:
  PentairIntelliflo() : pump_(this) {}

  void setup() override;
  void loop() override;
  void update() override;
  void dump_config() override;

  // Compatibility facade: existing ESPHome code generation and lambdas keep
  // using PentairIntelliflo while pump-specific data lives in pump_.
  void set_address(uint8_t address) { this->pump_.set_address(address); }
  void set_power_sensor(sensor::Sensor *s) { this->pump_.set_power_sensor(s); }
  void set_rpm_sensor(sensor::Sensor *s) { this->pump_.set_rpm_sensor(s); }
  void set_flow_sensor(sensor::Sensor *s) { this->pump_.set_flow_sensor(s); }
  void set_filter_percent_sensor(sensor::Sensor *s) { this->pump_.set_filter_percent_sensor(s); }
  void set_error_code_sensor(sensor::Sensor *s) { this->pump_.set_error_code_sensor(s); }
  void set_time_remaining_sensor(sensor::Sensor *s) { this->pump_.set_time_remaining_sensor(s); }
  void set_running_binary_sensor(binary_sensor::BinarySensor *s) { this->pump_.set_running_binary_sensor(s); }
  void set_remote_control_binary_sensor(binary_sensor::BinarySensor *s) {
    this->pump_.set_remote_control_binary_sensor(s);
  }
  void set_program_text_sensor(text_sensor::TextSensor *s) { this->pump_.set_program_text_sensor(s); }
  void set_pump_state_text_sensor(text_sensor::TextSensor *s) { this->pump_.set_pump_state_text_sensor(s); }
  void set_error_text_sensor(text_sensor::TextSensor *s) { this->pump_.set_error_text_sensor(s); }

  void request_status() { this->pump_.request_status(); }
  void set_remote_control(bool remote) { this->pump_.set_remote_control(remote); }
  void set_pump_running(bool running) { this->pump_.set_pump_running(running); }
  void set_speed_rpm(uint16_t rpm) { this->pump_.set_speed_rpm(rpm); }
  void set_speed_gpm(uint8_t gpm) { this->pump_.set_speed_gpm(gpm); }
  void set_program_speed(uint8_t program, uint16_t rpm) { this->pump_.set_program_speed(program, rpm); }
  void run_program(uint8_t program) { this->pump_.run_program(program); }
  void set_speed_index(uint8_t index) { this->pump_.set_speed_index(index); }

  // Aliases matching the upstream component's method names.
  void requestPumpStatus() { this->request_status(); }                       // NOLINT
  void pumpToRemoteControl() { this->set_remote_control(true); }             // NOLINT
  void pumpToLocalControl() { this->set_remote_control(false); }             // NOLINT
  void run() { this->set_pump_running(true); }
  void stop() { this->set_pump_running(false); }
  void commandRPM(int rpm) { this->set_speed_rpm(rpm < 0 ? 0 : (uint16_t) rpm); }   // NOLINT
  void commandFlow(int gpm) { this->set_speed_gpm(gpm < 0 ? 0 : (uint8_t) gpm); }   // NOLINT
  void commandExternalProgram(int prog) { this->run_program((uint8_t) prog); }      // NOLINT
  void saveValueForProgram(int prog, int value) {                                   // NOLINT
    this->set_program_speed((uint8_t) prog, value < 0 ? 0 : (uint16_t) value);
  }

  bool is_running() const { return this->pump_.is_running(); }
  bool is_remote_control() const { return this->pump_.is_remote_control(); }
  uint16_t current_rpm() const { return this->pump_.current_rpm(); }
  uint16_t current_watts() const { return this->pump_.current_watts(); }

 protected:
  friend class PentairIntellifloPump;

  void queue_command_(uint8_t address, uint8_t command, const std::vector<uint8_t> &payload);
  void feed_byte_(uint8_t byte);
  bool resync_();
  void handle_frame_(size_t total);

  PentairIntellifloPump pump_;
  std::vector<uint8_t> rx_;
  std::deque<std::vector<uint8_t>> tx_queue_;
  uint32_t last_rx_ms_{0};
  uint32_t last_tx_ms_{0};
};

}  // namespace pentair_intelliflo
}  // namespace esphome
