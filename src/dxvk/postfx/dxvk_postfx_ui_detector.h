#pragma once

#include <atomic>
#include <cstdint>

namespace dxvk {

  // D3D9 FVF constants - values only, names would conflict with d3d9.h
  // D3DFVF_POSITION_MASK = 0x400E (extracts position format from FVF)
  // D3DFVF_XYZRHW = 0x0004 (pre-transformed screen-space vertices)

  /**
   * \brief PostFX UI Detector
   *
   * Detects when the game transitions from 3D scene rendering
   * to UI rendering. This is used to apply effects only to
   * the 3D scene and avoid processing UI elements.
   *
   * Detection is based on D3DFVF_XYZRHW - pre-transformed vertices
   * are a near-certain indicator of UI/2D rendering.
   *
   * Thread safety:
   * - isInUIPhase(): Thread-safe (atomic read), can be called from any thread
   * - onFVFChanged(), onDrawCall(), resetFrame(): Must be called from D3D9
   *   render thread only. These modify non-atomic state and are not thread-safe.
   * - isPreTransformed(), getDrawsBeforeUI(): Read-only but access non-atomic
   *   state. Only safe to call from the same thread as the update methods.
   */
  class DxvkPostFXUIDetector {

  public:

    DxvkPostFXUIDetector() = default;
    ~DxvkPostFXUIDetector() = default;

    /**
     * \brief Notify FVF change
     *
     * Called when the game sets a new FVF. Detects transition
     * to UI phase when D3DFVF_XYZRHW is set.
     *
     * Thread safety: Must be called from D3D9 render thread only.
     *
     * \param [in] fvf New flexible vertex format flags
     */
    void onFVFChanged(uint32_t fvf);

    /**
     * \brief Check if currently in UI phase
     *
     * Returns true once D3DFVF_XYZRHW has been seen this frame.
     * Remains true until resetFrame() is called.
     *
     * Thread safety: Safe to call from any thread (atomic read).
     *
     * \returns True if UI rendering detected
     */
    bool isInUIPhase() const {
      return m_inUIPhase.load(std::memory_order_relaxed);
    }

    /**
     * \brief Check if current draw uses pre-transformed vertices
     *
     * Returns true if the current FVF uses D3DFVF_XYZRHW.
     * This is per-draw-call state, not frame state.
     *
     * Thread safety: Must be called from D3D9 render thread only.
     *
     * \returns True if current draw is pre-transformed
     */
    bool isPreTransformed() const {
      return m_isPreTransformed;
    }

    /**
     * \brief Reset frame state
     *
     * Call at end of Present (after effects are applied) to reset
     * UI detection for the next frame.
     *
     * Thread safety: Must be called from D3D9 render thread only.
     */
    void resetFrame();

    /**
     * \brief Get draw call count before UI
     *
     * Returns the number of draw calls processed before
     * UI was detected. Useful for debugging.
     *
     * Thread safety: Must be called from D3D9 render thread only.
     *
     * \returns Draw call count before UI phase
     */
    uint32_t getDrawsBeforeUI() const {
      return m_drawsBeforeUI;
    }

    /**
     * \brief Increment draw call counter
     *
     * Call for each draw call to track when UI phase begins.
     *
     * Thread safety: Must be called from D3D9 render thread only.
     */
    void onDrawCall();

  private:

    // Per-draw-call state (single thread)
    bool     m_isPreTransformed = false;

    // Frame state (cross-thread)
    std::atomic<bool> m_inUIPhase { false };

    // Debug statistics
    uint32_t m_drawCallCount   = 0;
    uint32_t m_drawsBeforeUI   = 0;
    bool     m_uiDetectedThisFrame = false;

  };

}
