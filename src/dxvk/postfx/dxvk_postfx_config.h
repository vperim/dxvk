#pragma once

#include "../../util/config/config.h"

namespace dxvk {

  /**
   * \brief PostFX configuration
   *
   * Loaded from dxvk.conf, isolated from core DxvkOptions
   * to minimize upstream merge conflicts.
   */
  struct DxvkPostFXConfig {
    bool        enabled       = false;
    int32_t     toggleKey     = 121;    // VK_F10 (0x79)
    std::string preset        = "gameplay";

    // Future effect settings will go here

    DxvkPostFXConfig();
    DxvkPostFXConfig(const Config& config);

    void log() const;
  };

}
