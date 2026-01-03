#include "dxvk_hud_postfx.h"

namespace dxvk::hud {

  HudPostFXItem::HudPostFXItem() { }

  void HudPostFXItem::update(dxvk::high_resolution_clock::time_point time) {
    // State updates handled externally via setEnabled()
  }

  HudPos HudPostFXItem::render(
      const Rc<DxvkCommandList>&  ctx,
      const HudPipelineKey&       key,
      const HudOptions&           options,
            HudRenderer&          renderer,
            HudPos                position) {
    position.y += 16.0f;

    bool enabled = m_enabled.load(std::memory_order_relaxed);
    uint32_t color = enabled ? 0xff40ff40u : 0xff4040ffu;  // Green/Red (ABGR)
    renderer.drawText(16.0f, position, color,
      enabled ? "PostFX: ON" : "PostFX: OFF");

    position.y += 8.0f;
    return position;
  }

}
