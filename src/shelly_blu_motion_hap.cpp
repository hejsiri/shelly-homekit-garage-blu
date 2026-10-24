#include "shelly_blu_motion_hap.hpp"
#ifdef SHELLY_GARAGE_BLE
#include "mgos_hap_chars.hpp"
#include "mgos_timers.hpp"
#include "shelly_blu_garage.hpp"
namespace shelly { namespace blu {
namespace {
// Stable instance IDs; optional services exist only for securely paired sensors.
class MotionService : public mgos::hap::Service {
 public:
  MotionService()
      : Service(0x1400, &kHAPServiceType_MotionSensor,
                kHAPServiceDebugDescription_MotionSensor),
        timer_([this] { Poll(); }) {
    AddNameChar(0x1401, SensorName(2));
    motion_ = new mgos::hap::BoolCharacteristic(
        0x1402, &kHAPCharacteristicType_MotionDetected,
        [this](HAPAccessoryServerRef *, const HAPBoolCharacteristicReadRequest *, bool *value) {
          *value = MotionReady() ? MotionReading().active : active_;
          return kHAPError_None;
        }, true, nullptr, kHAPCharacteristicDebugDescription_MotionDetected);
    AddChar(motion_);
    fault_ = new mgos::hap::UInt8Characteristic(
        0x1403, &kHAPCharacteristicType_StatusFault, 0, 1, 1,
        [](HAPAccessoryServerRef *, const HAPUInt8CharacteristicReadRequest *, uint8_t *value) {
          *value = MotionReady() ? 0 : 1;
          return kHAPError_None;
        }, true, nullptr, kHAPCharacteristicDebugDescription_StatusFault);
    AddChar(fault_);
    status_active_ = new mgos::hap::BoolCharacteristic(
        0x1404, &kHAPCharacteristicType_StatusActive,
        [](HAPAccessoryServerRef *, const HAPBoolCharacteristicReadRequest *, bool *value) {
          *value = MotionReady();
          return kHAPError_None;
        }, true, nullptr, kHAPCharacteristicDebugDescription_StatusActive);
    AddChar(status_active_);
    timer_.Reset(1000, MGOS_TIMER_REPEAT);
  }
 private:
  void Poll() {
    const bool ready = MotionReady(), active = MotionReading().active;
    if (ready != ready_) { fault_->RaiseEvent(); status_active_->RaiseEvent(); }
    if (ready && (!ready_ || active != active_)) motion_->RaiseEvent();
    ready_ = ready;
    if (ready) active_ = active;
  }
  bool ready_ = false, active_ = false;
  mgos::hap::BoolCharacteristic *motion_ = nullptr;
  mgos::hap::UInt8Characteristic *fault_ = nullptr;
  mgos::hap::BoolCharacteristic *status_active_ = nullptr;
  mgos::Timer timer_;
};
class HTService : public mgos::hap::Service {
 public:
  HTService(int role, bool temperature)
      : Service(0x1500 + (role - 3) * 0x20 + (temperature ? 0 : 0x10),
                temperature ? &kHAPServiceType_TemperatureSensor : &kHAPServiceType_HumiditySensor,
                temperature ? kHAPServiceDebugDescription_TemperatureSensor : kHAPServiceDebugDescription_HumiditySensor),
        role_(role), temperature_(temperature), timer_([this] { Poll(); }) {
    const auto base = svc_.iid;
    AddNameChar(base + 1, SensorName(role));
    value_ = new mgos::hap::FloatCharacteristic(base + 2,
        temperature ? &kHAPCharacteristicType_CurrentTemperature : &kHAPCharacteristicType_CurrentRelativeHumidity,
        temperature ? -100 : 0, 100, temperature ? .1 : 1,
        [this](HAPAccessoryServerRef *, const HAPFloatCharacteristicReadRequest *, float *value) {
          if (!SensorReady(role_)) return kHAPError_Unknown;
          const auto &reading = ReadingFor(role_);
          *value = temperature_ ? reading.temperature10 / 10.0f : reading.humidity;
          return kHAPError_None;
        }, true, nullptr, temperature ? kHAPCharacteristicDebugDescription_CurrentTemperature :
                                      kHAPCharacteristicDebugDescription_CurrentRelativeHumidity);
    AddChar(value_);
    fault_ = new mgos::hap::UInt8Characteristic(base + 3, &kHAPCharacteristicType_StatusFault, 0, 1, 1,
        [this](HAPAccessoryServerRef *, const HAPUInt8CharacteristicReadRequest *, uint8_t *value) {
          *value = SensorReady(role_) ? 0 : 1;
          return kHAPError_None;
        }, true, nullptr, kHAPCharacteristicDebugDescription_StatusFault);
    AddChar(fault_);
    timer_.Reset(1000, MGOS_TIMER_REPEAT);
  }
 private:
  void Poll() {
    const bool ready = SensorReady(role_);
    const auto &reading = ReadingFor(role_);
    const int value = temperature_ ? reading.temperature10 : reading.humidity;
    if (ready != ready_) fault_->RaiseEvent();
    if (ready && (!ready_ || value != last_value_)) value_->RaiseEvent();
    ready_ = ready; last_value_ = value;
  }
  int role_, last_value_ = 0;
  bool temperature_, ready_ = false;
  mgos::hap::FloatCharacteristic *value_ = nullptr;
  mgos::hap::UInt8Characteristic *fault_ = nullptr;
  mgos::Timer timer_;
};
class CarPresenceService : public mgos::hap::Service {
 public:
  CarPresenceService()
      : Service(0x1600, &kHAPServiceType_OccupancySensor,
                kHAPServiceDebugDescription_OccupancySensor),
        timer_([this] { Poll(); }) {
    AddNameChar(0x1601, CarName());
    presence_ = new mgos::hap::UInt8Characteristic(
        0x1602, &kHAPCharacteristicType_OccupancyDetected, 0, 1, 1,
        [this](HAPAccessoryServerRef *, const HAPUInt8CharacteristicReadRequest *, uint8_t *value) {
          *value = (CarPresenceReady() ? CarPresent() : present_) ? 1 : 0;
          return kHAPError_None;
        }, true, nullptr, kHAPCharacteristicDebugDescription_OccupancyDetected);
    AddChar(presence_);
    fault_ = new mgos::hap::UInt8Characteristic(
        0x1603, &kHAPCharacteristicType_StatusFault, 0, 1, 1,
        [](HAPAccessoryServerRef *, const HAPUInt8CharacteristicReadRequest *, uint8_t *value) {
          *value = CarPresenceReady() ? 0 : 1;
          return kHAPError_None;
        }, true, nullptr, kHAPCharacteristicDebugDescription_StatusFault);
    AddChar(fault_);
    status_active_ = new mgos::hap::BoolCharacteristic(
        0x1604, &kHAPCharacteristicType_StatusActive,
        [](HAPAccessoryServerRef *, const HAPBoolCharacteristicReadRequest *, bool *value) {
          *value = CarPresenceReady();
          return kHAPError_None;
        }, true, nullptr, kHAPCharacteristicDebugDescription_StatusActive);
    AddChar(status_active_);
    timer_.Reset(1000, MGOS_TIMER_REPEAT);
  }
 private:
  void Poll() {
    const bool ready = CarPresenceReady(), present = CarPresent();
    if (ready != ready_) { fault_->RaiseEvent(); status_active_->RaiseEvent(); }
    if (ready && (!ready_ || present != present_)) presence_->RaiseEvent();
    ready_ = ready;
    if (ready) present_ = present;
  }
  bool ready_ = false, present_ = false;
  mgos::hap::UInt8Characteristic *presence_ = nullptr, *fault_ = nullptr;
  mgos::hap::BoolCharacteristic *status_active_ = nullptr;
  mgos::Timer timer_;
};
class ButtonService : public mgos::hap::Service {
 public:
  ButtonService()
      : Service(0x1700, &kHAPServiceType_StatelessProgrammableSwitch,
                kHAPServiceDebugDescription_StatelessProgrammableSwitch) {
    AddNameChar(0x1701, SensorName(5));
    event_ = new mgos::hap::UInt8Characteristic(
        0x1702, &kHAPCharacteristicType_ProgrammableSwitchEvent, 0, 2, 1,
        [this](HAPAccessoryServerRef *, const HAPUInt8CharacteristicReadRequest *, uint8_t *value) {
          if (last_event_ < 0) return kHAPError_InvalidState;
          *value = static_cast<uint8_t>(last_event_);
          return kHAPError_None;
        }, true, nullptr, kHAPCharacteristicDebugDescription_ProgrammableSwitchEvent);
    AddChar(event_);
    SetButtonEventHandler([this](uint8_t event) {
      last_event_ = event;
      event_->RaiseEvent();
    });
  }
  ~ButtonService() override { SetButtonEventHandler(nullptr); }
 private:
  int last_event_ = -1;
  mgos::hap::UInt8Characteristic *event_ = nullptr;
};
}
std::unique_ptr<mgos::hap::Service> CreateMotionService() {
  return std::unique_ptr<mgos::hap::Service>(new MotionService());
}
std::unique_ptr<mgos::hap::Service> CreateHTService(int role, bool temperature) {
  return std::unique_ptr<mgos::hap::Service>(new HTService(role, temperature));
}
std::unique_ptr<mgos::hap::Service> CreateCarPresenceService() {
  return std::unique_ptr<mgos::hap::Service>(new CarPresenceService());
}
std::unique_ptr<mgos::hap::Service> CreateButtonService() {
  return std::unique_ptr<mgos::hap::Service>(new ButtonService());
}
} }
#endif
