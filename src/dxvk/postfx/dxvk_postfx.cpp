#include "dxvk_postfx.h"

#include <windows.h>

#include "../../util/log/log.h"
#include "../../util/util_string.h"

namespace dxvk {

  DxvkPostFXManager::DxvkPostFXManager(
          DxvkDevice*           device,
    const Config&               config)
  : m_device  (device),
    m_config  (config),
    m_enabled (m_config.enabled) {
    m_config.log();
    Logger::info("PostFX: Manager initialized");
  }

  DxvkPostFXManager::~DxvkPostFXManager() {
    Logger::info("PostFX: Manager destroyed");
  }

  void DxvkPostFXManager::processInput() {
    // Edge detection: trigger on key press, not hold
    bool currentKeyState = (GetAsyncKeyState(m_config.toggleKey) & 0x8000) != 0;

    if (currentKeyState && !m_keyPressed) {
      bool newState = !m_enabled.load(std::memory_order_relaxed);
      m_enabled.store(newState, std::memory_order_relaxed);
      Logger::info(str::format("PostFX: Toggled ", newState ? "ON" : "OFF"));
    }

    m_keyPressed = currentKeyState;
  }

  void DxvkPostFXManager::apply(
          DxvkContext*          ctx,
    const Rc<DxvkImageView>&    backbuffer) {
    if (!m_enabled.load(std::memory_order_relaxed))
      return;

    // Phase 2+ will add actual effect processing here
  }

  void DxvkPostFXManager::updateHud(hud::HudPostFXItem* hudItem) {
    if (hudItem)
      hudItem->setEnabled(m_enabled.load(std::memory_order_relaxed));
  }

}
