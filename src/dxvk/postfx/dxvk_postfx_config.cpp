#include "dxvk_postfx_config.h"

#include "../../util/log/log.h"
#include "../../util/util_string.h"

namespace dxvk {

  DxvkPostFXConfig::DxvkPostFXConfig() { }

  DxvkPostFXConfig::DxvkPostFXConfig(const Config& config) {
    enabled   = config.getOption<bool>("dxvk.postfx.enabled", false);
    toggleKey = config.getOption<int32_t>("dxvk.postfx.toggleKey", 121);
    preset    = config.getOption<std::string>("dxvk.postfx.preset", "gameplay");
  }

  void DxvkPostFXConfig::log() const {
    Logger::info(str::format(
      "PostFX: enabled=", enabled ? "true" : "false",
      ", toggleKey=0x", std::hex, toggleKey,
      ", preset=", preset.c_str()));
  }

}
