#pragma once

#include <atomic>

#include "../dxvk_device.h"
#include "../hud/dxvk_hud_postfx.h"
#include "dxvk_postfx_config.h"

namespace dxvk {

  /**
   * \brief PostFX Manager
   *
   * Coordinates config, input, HUD, and effect processing.
   * Designed for minimal coupling to core DXVK code.
   */
  class DxvkPostFXManager : public RcObject {

  public:

    DxvkPostFXManager(
            DxvkDevice*           device,
      const Config&               config);

    ~DxvkPostFXManager();

    /**
     * \brief Process input and update state
     *
     * Call once per frame before rendering.
     * Handles hotkey detection with edge detection.
     */
    void processInput();

    /**
     * \brief Apply PostFX effects
     *
     * Call inside present path. Currently a placeholder
     * that will be expanded in Phase 2+.
     */
    void apply(
            DxvkContext*          ctx,
      const Rc<DxvkImageView>&    backbuffer);

    /**
     * \brief Update HUD item state
     */
    void updateHud(hud::HudPostFXItem* hudItem);

    bool isEnabled() const { return m_enabled.load(std::memory_order_relaxed); }

  private:

    DxvkDevice*       m_device;
    DxvkPostFXConfig  m_config;
    std::atomic<bool> m_enabled       { false };
    bool              m_keyPressed    = false;

  };

}
