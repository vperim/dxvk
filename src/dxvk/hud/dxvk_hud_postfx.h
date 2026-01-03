#pragma once

#include <atomic>

#include "dxvk_hud_item.h"

namespace dxvk::hud {

  /**
   * \brief PostFX status HUD item
   *
   * Displays current PostFX state and active effects.
   */
  class HudPostFXItem : public HudItem {

  public:

    HudPostFXItem();

    void update(dxvk::high_resolution_clock::time_point time) override;

    HudPos render(
      const Rc<DxvkCommandList>&  ctx,
      const HudPipelineKey&       key,
      const HudOptions&           options,
            HudRenderer&          renderer,
            HudPos                position) override;

    void setEnabled(bool enabled) { m_enabled.store(enabled, std::memory_order_relaxed); }

  private:

    std::atomic<bool> m_enabled { false };

  };

}
