#pragma once

#include <Arduino.h>
#include <Wire.h>

class At24cAsync {
 public:
  struct Stats {
    uint32_t write_calls = 0;
    uint32_t read_calls = 0;
    uint32_t poll_calls = 0;
    uint32_t max_service_us = 0;
  };

  At24cAsync(uint8_t i2c_addr, uint8_t addr_bytes, uint8_t page_size, uint8_t max_chunk)
      : i2c_addr_(i2c_addr),
        addr_bytes_(addr_bytes),
        page_size_(page_size),
        max_chunk_(max_chunk) {}

  void Begin() { Wire.begin(); }

  bool ReadByte(uint16_t address, uint8_t *value) {
    Wire.beginTransmission(i2c_addr_);
    if (addr_bytes_ == 2) {
      Wire.write(static_cast<uint8_t>(address >> 8));
    }
    Wire.write(static_cast<uint8_t>(address & 0xFF));
    if (Wire.endTransmission(false) != 0) {
      return false;
    }
    if (Wire.requestFrom(i2c_addr_, static_cast<uint8_t>(1)) != 1) {
      return false;
    }
    *value = Wire.read();
    return true;
  }

  bool WriteByte(uint16_t address, uint8_t value) {
    Wire.beginTransmission(i2c_addr_);
    if (addr_bytes_ == 2) {
      Wire.write(static_cast<uint8_t>(address >> 8));
    }
    Wire.write(static_cast<uint8_t>(address & 0xFF));
    Wire.write(value);
    if (Wire.endTransmission() != 0) {
      return false;
    }
    return true;
  }

  bool WaitReady(uint32_t timeout_ms) {
    const uint32_t start = millis();
    while (millis() - start < timeout_ms) {
      if (ReadyPoll()) {
        return true;
      }
      delay(1);
    }
    return false;
  }

  void StartWrite(uint16_t start_address, const uint8_t *data, uint16_t length) {
    start_address_ = start_address;
    current_address_ = start_address;
    data_ = data;
    remaining_ = length;
    state_ = State::kWritePending;
  }

  void StartRead(uint16_t start_address, uint8_t *data, uint16_t length) {
    start_address_ = start_address;
    current_address_ = start_address;
    read_data_ = data;
    remaining_ = length;
    state_ = State::kReadPending;
  }

  void Service() {
    const uint32_t t0 = micros();
    switch (state_) {
      case State::kIdle:
      case State::kDone:
      case State::kError:
        break;
      case State::kWritePending:
        if (!WriteNextPage()) {
          state_ = State::kError;
        } else {
          state_ = State::kWaiting;
        }
        break;
      case State::kReadPending:
        if (!ReadNextChunk()) {
          state_ = State::kError;
        } else if (remaining_ == 0) {
          state_ = State::kDone;
        }
        break;
      case State::kWaiting:
        if (ReadyPoll()) {
          if (remaining_ == 0) {
            state_ = State::kDone;
          } else {
            state_ = State::kWritePending;
          }
        }
        break;
    }
    const uint32_t elapsed = micros() - t0;
    if (elapsed > stats_.max_service_us) {
      stats_.max_service_us = elapsed;
    }
  }

  bool IsDone() const { return state_ == State::kDone; }
  bool HasError() const { return state_ == State::kError; }
  bool IsBusy() const { return state_ != State::kIdle && state_ != State::kDone; }
  const Stats &GetStats() const { return stats_; }

 private:
  enum class State : uint8_t {
    kIdle,
    kWritePending,
    kReadPending,
    kWaiting,
    kDone,
    kError
  };

  bool WriteNextPage() {
    uint16_t count =
        remaining_ >= page_size_ ? page_size_ : static_cast<uint8_t>(remaining_);
    if (max_chunk_ > 0 && count > max_chunk_) {
      count = max_chunk_;
    }
    Wire.beginTransmission(i2c_addr_);
    if (addr_bytes_ == 2) {
      Wire.write(static_cast<uint8_t>(current_address_ >> 8));
    }
    Wire.write(static_cast<uint8_t>(current_address_ & 0xFF));
    Wire.write(data_ + (current_address_ - start_address_), count);
    stats_.write_calls++;
    if (Wire.endTransmission() != 0) {
      return false;
    }
    current_address_ += count;
    remaining_ -= count;
    return true;
  }

  bool ReadyPoll() {
    Wire.beginTransmission(i2c_addr_);
    stats_.poll_calls++;
    return Wire.endTransmission() == 0;
  }

  bool ReadNextChunk() {
    uint16_t count =
        remaining_ >= page_size_ ? page_size_ : static_cast<uint8_t>(remaining_);
    if (max_chunk_ > 0 && count > max_chunk_) {
      count = max_chunk_;
    }
    Wire.beginTransmission(i2c_addr_);
    if (addr_bytes_ == 2) {
      Wire.write(static_cast<uint8_t>(current_address_ >> 8));
    }
    Wire.write(static_cast<uint8_t>(current_address_ & 0xFF));
    if (Wire.endTransmission(false) != 0) {
      return false;
    }
    stats_.read_calls++;
    if (Wire.requestFrom(i2c_addr_, static_cast<uint8_t>(count)) != count) {
      return false;
    }
    for (uint16_t i = 0; i < count; ++i) {
      read_data_[current_address_ - start_address_ + i] = Wire.read();
    }
    current_address_ += count;
    remaining_ -= count;
    return true;
  }

  uint8_t i2c_addr_;
  uint8_t addr_bytes_;
  uint8_t page_size_;
  uint8_t max_chunk_;
  State state_ = State::kIdle;
  uint16_t start_address_ = 0;
  uint16_t current_address_ = 0;
  const uint8_t *data_ = nullptr;
  uint8_t *read_data_ = nullptr;
  uint16_t remaining_ = 0;
  Stats stats_;
};
