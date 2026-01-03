#include "dxvk_postfx_ui_detector.h"

namespace dxvk {

  void DxvkPostFXUIDetector::onFVFChanged(uint32_t fvf) {
    // Extract position format from FVF
    // D3DFVF_POSITION_MASK = 0x400E, D3DFVF_XYZRHW = 0x0004
    constexpr uint32_t FvfPositionMask = 0x400E;
    constexpr uint32_t FvfXyzRhw       = 0x0004;

    uint32_t positionFormat = fvf & FvfPositionMask;

    // Check if pre-transformed (screen-space) vertices
    m_isPreTransformed = (positionFormat == FvfXyzRhw);

    // Once we see pre-transformed vertices, we're in UI phase
    if (m_isPreTransformed && !m_uiDetectedThisFrame) {
      m_inUIPhase.store(true, std::memory_order_relaxed);
      m_drawsBeforeUI = m_drawCallCount;
      m_uiDetectedThisFrame = true;
    }
  }

  void DxvkPostFXUIDetector::resetFrame() {
    m_inUIPhase.store(false, std::memory_order_relaxed);
    m_isPreTransformed = false;
    m_drawCallCount = 0;
    m_drawsBeforeUI = 0;
    m_uiDetectedThisFrame = false;
  }

  void DxvkPostFXUIDetector::onDrawCall() {
    m_drawCallCount++;
  }

}
