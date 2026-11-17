#pragma once

#include <cmath>

#include "esphome/core/component.h"
#include "esphome/components/climate/climate.h"

namespace esphome {
namespace navien_mat {

/// 나비엔 온수매트용 climate 엔티티.
/// 제어 로직은 YAML 스크립트가 담당한다.
///   - HA 요청     : control() 은 요청값만 저장하고 pending 플래그를 세움 → YAML interval 이 take_pending() 으로 수거
///                   (엔티티 상태는 바꾸지 않음 — 낙관적 표시 금지)
///   - 매트 상태   : publish_from_device() 로만 반영 (매트가 실제로 응답했을 때만 HA 상태가 바뀜)
class NavienClimate : public climate::Climate, public Component {
 public:
  void setup() override {
    this->mode = climate::CLIMATE_MODE_OFF;
    this->action = climate::CLIMATE_ACTION_OFF;
    this->current_temperature = NAN;
    this->target_temperature = NAN;
  }

  float get_setup_priority() const override { return setup_priority::DATA; }

  /// HA 에서 새 요청이 들어왔으면 true 를 한 번만 돌려준다.
  bool take_pending() {
    if (!this->pending_)
      return false;
    this->pending_ = false;
    return true;
  }

  /// 매트가 보내온 상태프레임을 엔티티에 반영
  void publish_from_device(bool power, float current, float target) {
    this->mode = power ? climate::CLIMATE_MODE_HEAT : climate::CLIMATE_MODE_OFF;
    this->action = power ? climate::CLIMATE_ACTION_HEATING : climate::CLIMATE_ACTION_OFF;
    this->current_temperature = current;
    this->target_temperature = target;
    this->publish_state();
  }

  /// HA 요청값 읽기용 (YAML 람다에서 사용) — 엔티티 상태가 아니라 마지막 요청값
  bool requested_power() const { return this->req_mode_ != climate::CLIMATE_MODE_OFF; }

  int requested_temp() const {
    if (std::isnan(this->req_target_))
      return 0;
    return (int) lroundf(this->req_target_);
  }

  // ESPHome 2026.9: set_supports_current_temperature() / set_supports_action() 는
  // ClimateFeature 비트마스크(add_feature_flags)로 대체됨.
  climate::ClimateTraits traits() override {
    auto traits = climate::ClimateTraits();
    traits.add_supported_mode(climate::CLIMATE_MODE_OFF);
    traits.add_supported_mode(climate::CLIMATE_MODE_HEAT);
    traits.add_feature_flags(climate::CLIMATE_SUPPORTS_CURRENT_TEMPERATURE |
                             climate::CLIMATE_SUPPORTS_ACTION);
    return traits;
  }

 protected:
  void control(const climate::ClimateCall &call) override {
    // 요청값만 기록. publish_state() 를 하지 않으므로 HA 표시는
    // 매트 상태프레임(publish_from_device)이 도착해야 바뀐다.
    // 명령이 BLE 에서 유실되면 HA 는 실제 상태(이전 값)를 그대로 보여준다.
    this->req_mode_ = call.get_mode().has_value() ? *call.get_mode() : this->mode;
    this->req_target_ = call.get_target_temperature().has_value() ? *call.get_target_temperature()
                                                                  : this->target_temperature;
    this->pending_ = true;
  }

  bool pending_{false};
  climate::ClimateMode req_mode_{climate::CLIMATE_MODE_OFF};
  float req_target_{NAN};
};

}  // namespace navien_mat
}  // namespace esphome
