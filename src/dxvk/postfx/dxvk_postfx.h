#pragma once

#include <atomic>

#include "../dxvk_include.h"
#include "dxvk_postfx_config.h"
#include "dxvk_postfx_types.h"
#include "dxvk_postfx_resources.h"
#include "dxvk_postfx_ui_detector.h"

namespace dxvk {

  class DxvkDevice;
  class DxvkContext;
  class DxvkImageView;

  namespace hud {
    class HudPostFXItem;
  }

  /**
   * \brief PostFX Manager
   *
   * Coordinates config, input, HUD, and effect processing.
   * Designed for minimal coupling to core DXVK code.
   *
   * Architecture:
   * - Resources: Shared mip chains, temporal buffers, depth cache
   * - UI Detector: Tracks 3D→UI transition via FVF
   * - Phases: Effects register for PreTonemapping/PostTonemapping/PreUI
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
     * \brief Begin frame processing
     *
     * Call at start of Present to prepare resources for
     * the current frame. Does not reset UI detector state
     * so effects can query it during apply().
     *
     * \param [in] backbufferExtent Current backbuffer size
     */
    void beginFrame(VkExtent2D backbufferExtent);

    /**
     * \brief End frame processing
     *
     * Call at end of Present after effects are applied.
     * Swaps temporal buffers and resets UI detector for
     * the next frame.
     */
    void endFrame();

    /**
     * \brief Apply PostFX effects for a specific phase
     *
     * Call at appropriate point in render pipeline.
     *
     * \param [in] ctx Graphics context
     * \param [in] backbuffer Current backbuffer view
     * \param [in] phase Which effects to apply
     */
    void apply(
            DxvkContext*          ctx,
      const Rc<DxvkImageView>&    backbuffer,
            PostFXPhase           phase = PostFXPhase::PreUI);

    /**
     * \brief Update HUD item state
     */
    void updateHud(hud::HudPostFXItem* hudItem);

    /**
     * \brief Check if PostFX is enabled
     */
    bool isEnabled() const { return m_enabled.load(std::memory_order_relaxed); }

    /**
     * \brief Get shared resources manager
     *
     * Provides access to temporal buffers, mip chains, depth cache.
     */
    DxvkPostFXResources* getResources() { return m_resources.ptr(); }

    /**
     * \brief Get UI detector
     *
     * Used by D3D9 layer to notify FVF changes.
     */
    DxvkPostFXUIDetector* getUIDetector() { return &m_uiDetector; }

    /**
     * \brief Check if currently in UI phase
     *
     * Convenience wrapper for UI detector query.
     */
    bool isInUIPhase() const { return m_uiDetector.isInUIPhase(); }

    /**
     * \brief Notify of depth buffer availability
     *
     * Called when depth buffer is available for reading.
     * Used to cache depth format info.
     *
     * \param [in] depthView Current depth buffer view
     * \param [in] nearPlane Camera near plane
     * \param [in] farPlane Camera far plane
     */
    void setDepthInfo(
      const Rc<DxvkImageView>&    depthView,
            float                 nearPlane,
            float                 farPlane);

  private:

    DxvkDevice*              m_device;
    DxvkPostFXConfig         m_config;
    Rc<DxvkPostFXResources>  m_resources;
    DxvkPostFXUIDetector     m_uiDetector;

    std::atomic<bool> m_enabled       { false };
    bool              m_keyPressed    = false;

    // Cached depth info
    Rc<DxvkImageView> m_depthView;
    float             m_nearPlane = 0.1f;
    float             m_farPlane  = 10000.0f;

    /**
     * \brief Apply effects for PreTonemapping phase
     */
    void applyPreTonemapping(
            DxvkContext*          ctx,
      const Rc<DxvkImageView>&    backbuffer);

    /**
     * \brief Apply effects for PostTonemapping phase
     */
    void applyPostTonemapping(
            DxvkContext*          ctx,
      const Rc<DxvkImageView>&    backbuffer);

    /**
     * \brief Apply effects for PreUI phase
     */
    void applyPreUI(
            DxvkContext*          ctx,
      const Rc<DxvkImageView>&    backbuffer);

  };

}
