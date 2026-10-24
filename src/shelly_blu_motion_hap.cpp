#include "shelly_blu_motion_hap.hpp"
#ifdef SHELLY_GARAGE_BLE
#include "mgos_hap_chars.hpp"
#include "mgos_timers.hpp"
#include "shelly_blu_garage.hpp"
namespace shelly { namespace blu {
namespace {
// The old 0x1400 service is already stored as Occupancy Sensor by some
// controllers, even though the firmware uses it for Motion Sensor. Retire the
// affected blu40/41 identities so controllers create fresh services rather than
// retaining the conflicting records. These replacement IDs must remain stable.
class MotionService : public mgos::hap::Service {
 public:
  MotionService()
      : Service(0x1800, &kHAPServiceType_MotionSensor,
                kHAPServiceDebugDescription_MotionSensor),
        timer_([this] { Poll(); }) {
    AddNameChar(0x1801, SensorName(2));
    motion_ = new mgos::hap::BoolCharacteristic(
        0x1802, &kHAPCharacteristicType_MotionDetected,
        [this](HAPAccessoryServerRef *, const HAPBoolCharacteristicReadRequest *, bool *value) {
          *value = MotionReady() ? MotionReading().active : active_;
          return kHAPError_None;
        }, true, nullptr, kHAPCharacteristicDebugDescription_MotionDetected);
    AddChar(motion_);
    fault_ = new mgos::hap::UInt8Characteristic(
        0x1803, &kHAPCharacteristicType_StatusFault, 0, 1, 1,
        [](HAPAccessoryServerRef *, const HAPUInt8CharacteristicReadRequest *, uint8_t *value) {
          *value = MotionReady() ? 0 : 1;
          return kHAPError_None;
        }, true, nullptr, kHAPCharacteristicDebugDescription_StatusFault);
    AddChar(fault_);
    status_active_ = new mgos::hap::BoolCharacteristic(
        0x1804, &kHAPCharacteristicType_StatusActive,
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
class LightLevelCharacteristic : public mgos::hap::FloatCharacteristic {
 public:
  explicit LightLevelCharacteristic(ReadHandler read)
      : FloatCharacteristic(0x1b02, &kHAPCharacteristicType_CurrentAmbientLightLevel,
                            0.0001f, 100000.0f, 0, read, true, nullptr,
                            kHAPCharacteristicDebugDescription_CurrentAmbientLightLevel) {
    hap_char_.char_.float_.units = kHAPCharacteristicUnits_Lux;
  }
};
class MotionLightService : public mgos::hap::Service {
 public:
  MotionLightService()
      : Service(0x1b00, &kHAPServiceType_LightSensor,
                kHAPServiceDebugDescription_LightSensor),
        timer_([this] { Poll(); }) {
    AddNameChar(0x1b01, "Light sensor");
    light_ = new LightLevelCharacteristic(
        [this](HAPAccessoryServerRef *, const HAPFloatCharacteristicReadRequest *, float *value) {
          const int32_t raw = MotionLightReady() ? MotionReading().illuminance100 : last_lux100_;
          // HomeKit's valid range excludes zero and is smaller than BTHome's.
          *value = raw <= 0 ? 0.0001f : raw >= 10000000 ? 100000.0f : raw / 100.0f;
          return kHAPError_None;
        });
    AddChar(light_);
    fault_ = new mgos::hap::UInt8Characteristic(
        0x1b03, &kHAPCharacteristicType_StatusFault, 0, 1, 1,
        [](HAPAccessoryServerRef *, const HAPUInt8CharacteristicReadRequest *, uint8_t *value) {
          *value = MotionLightReady() ? 0 : 1;
          return kHAPError_None;
        }, true, nullptr, kHAPCharacteristicDebugDescription_StatusFault);
    AddChar(fault_);
    status_active_ = new mgos::hap::BoolCharacteristic(
        0x1b04, &kHAPCharacteristicType_StatusActive,
        [](HAPAccessoryServerRef *, const HAPBoolCharacteristicReadRequest *, bool *value) {
          *value = MotionLightReady();
          return kHAPError_None;
        }, true, nullptr, kHAPCharacteristicDebugDescription_StatusActive);
    AddChar(status_active_);
    timer_.Reset(1000, MGOS_TIMER_REPEAT);
  }
 private:
  void Poll() {
    const bool ready = MotionLightReady();
    const int32_t raw = MotionReading().illuminance100;
    if (ready != ready_) { fault_->RaiseEvent(); status_active_->RaiseEvent(); }
    if (ready && (!ready_ || raw != last_lux100_)) light_->RaiseEvent();
    ready_ = ready;
    if (ready) last_lux100_ = raw;
  }
  bool ready_ = false;
  int32_t last_lux100_ = -1;
  mgos::hap::FloatCharacteristic *light_ = nullptr;
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
      : Service(0x1900, &kHAPServiceType_OccupancySensor,
                kHAPServiceDebugDescription_OccupancySensor),
        timer_([this] { Poll(); }) {
    AddNameChar(0x1901, CarName());
    presence_ = new mgos::hap::UInt8Characteristic(
        0x1902, &kHAPCharacteristicType_OccupancyDetected, 0, 1, 1,
        [this](HAPAccessoryServerRef *, const HAPUInt8CharacteristicReadRequest *, uint8_t *value) {
          *value = (CarPresenceReady() ? CarPresent() : present_) ? 1 : 0;
          return kHAPError_None;
        }, true, nullptr, kHAPCharacteristicDebugDescription_OccupancyDetected);
    AddChar(presence_);
    fault_ = new mgos::hap::UInt8Characteristic(
        0x1903, &kHAPCharacteristicType_StatusFault, 0, 1, 1,
        [](HAPAccessoryServerRef *, const HAPUInt8CharacteristicReadRequest *, uint8_t *value) {
          *value = CarPresenceReady() ? 0 : 1;
          return kHAPError_None;
        }, true, nullptr, kHAPCharacteristicDebugDescription_StatusFault);
    AddChar(fault_);
    status_active_ = new mgos::hap::BoolCharacteristic(
        0x1904, &kHAPCharacteristicType_StatusActive,
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
      : Service(0x1a00, &kHAPServiceType_StatelessProgrammableSwitch,
                kHAPServiceDebugDescription_StatelessProgrammableSwitch) {
    AddNameChar(0x1a01, SensorName(5));
    event_ = new mgos::hap::UInt8Characteristic(
        0x1a02, &kHAPCharacteristicType_ProgrammableSwitchEvent, 0, 2, 1,
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
std::unique_ptr<mgos::hap::Service> CreateMotionLightService() {
  return std::unique_ptr<mgos::hap::Service>(new MotionLightService());
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
