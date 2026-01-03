#include "dxvk_postfx.h"

#include "../dxvk_device.h"
#include "../dxvk_context.h"
#include "../hud/dxvk_hud_postfx.h"

#include "../../util/util_string.h"
#include "../../util/log/log.h"

#include <windows.h>

namespace dxvk {

  DxvkPostFXManager::DxvkPostFXManager(
          DxvkDevice*           device,
    const Config&               config)
  : m_device    (device),
    m_config    (config),
    m_resources (new DxvkPostFXResources(device)),
    m_enabled   (m_config.enabled) {
    m_config.log();
    Logger::info("PostFX: Manager initialized with phase-based architecture");
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
    }

    m_keyPressed = currentKeyState;
  }

  void DxvkPostFXManager::beginFrame(VkExtent2D backbufferExtent) {
    // Prepare resources for new frame
    m_resources->beginFrame(backbufferExtent);
  }

  void DxvkPostFXManager::endFrame() {
    // Swap temporal buffers
    m_resources->endFrame();

    // Reset UI detector for next frame (after effects have used current state)
    m_uiDetector.resetFrame();
  }

  void DxvkPostFXManager::apply(
          DxvkContext*          ctx,
    const Rc<DxvkImageView>&    backbuffer,
          PostFXPhase           phase) {
    if (!m_enabled.load(std::memory_order_relaxed))
      return;

    // Dispatch to appropriate phase handler
    switch (phase) {
      case PostFXPhase::PreTonemapping:
        applyPreTonemapping(ctx, backbuffer);
        break;

      case PostFXPhase::PostTonemapping:
        applyPostTonemapping(ctx, backbuffer);
        break;

      case PostFXPhase::PreUI:
        applyPreUI(ctx, backbuffer);
        break;
    }
  }

  void DxvkPostFXManager::updateHud(hud::HudPostFXItem* hudItem) {
    if (hudItem)
      hudItem->setEnabled(m_enabled.load(std::memory_order_relaxed));
  }

  void DxvkPostFXManager::setDepthInfo(
    const Rc<DxvkImageView>&    depthView,
          float                 nearPlane,
          float                 farPlane) {
    m_depthView  = depthView;
    m_nearPlane  = nearPlane;
    m_farPlane   = farPlane;
    m_resources->setDepthRange(nearPlane, farPlane);
  }

  void DxvkPostFXManager::applyPreTonemapping(
          DxvkContext*          ctx,
    const Rc<DxvkImageView>&    backbuffer) {
    // TODO(Phase 3+): SSAO, SSR, Bloom
    // These effects need HDR scene data before the game's tonemapper
  }

  void DxvkPostFXManager::applyPostTonemapping(
          DxvkContext*          ctx,
    const Rc<DxvkImageView>&    backbuffer) {
    // TODO(Phase 2): Local Contrast (Clarity)
    // - Use m_resources->getTemporalPair("clarity") for temporal stability
    // - Use m_resources->getMipChain() for multi-scale detail extraction
    // - Use m_resources->getDepthCache() for depth-aware de-haloing
    // - Check isInUIPhase() to skip UI frames
  }

  void DxvkPostFXManager::applyPreUI(
          DxvkContext*          ctx,
    const Rc<DxvkImageView>&    backbuffer) {
    // TODO(Phase 3+): Debanding, CAS sharpening
    // These effects run after all other effects, right before UI overlay
  }

}
