#pragma once

#include <algorithm>
#include <string>
#include <unordered_map>
#include <vector>

#include "../dxvk_include.h"
#include "dxvk_postfx_types.h"

namespace dxvk {

  class DxvkDevice;
  class DxvkContext;
  class DxvkImage;
  class DxvkImageView;

  /**
   * \brief Temporal buffer pair
   *
   * Ping-pong buffer pair for temporal accumulation effects.
   * Automatically handles buffer swapping each frame.
   */
  struct PostFXTemporalPair {
    Rc<DxvkImage>     images[2];
    Rc<DxvkImageView> views[2];
    uint32_t          currentIndex = 0;
    VkExtent2D        extent       = { 0, 0 };
    VkFormat          format       = VK_FORMAT_UNDEFINED;
    bool              historyValid = false;  ///< False after creation/resize

    Rc<DxvkImageView> current() const { return views[currentIndex]; }
    Rc<DxvkImageView> history() const { return views[1 - currentIndex]; }

    /**
     * \brief Swap buffers for next frame
     *
     * After swap, history buffer contains previous frame's current.
     * Sets historyValid = true since we now have valid history.
     */
    void swap() {
      currentIndex = 1 - currentIndex;
      historyValid = true;
    }

    bool valid() const { return images[0] != nullptr; }

    /**
     * \brief Check if history buffer contains valid data
     *
     * Returns false on first frame after creation or resize.
     * Effects should use current-only sampling when this is false.
     */
    bool hasValidHistory() const { return historyValid; }
  };

  /**
   * \brief Mip chain resource
   *
   * Pre-generated mip pyramid for effects that need
   * multi-scale sampling (clarity, bloom).
   */
  struct PostFXMipChain {
    Rc<DxvkImage>                    image;
    std::vector<Rc<DxvkImageView>>   mipViews;
    VkExtent2D                       baseExtent = { 0, 0 };
    uint32_t                         mipLevels  = 0;

    bool valid() const { return image != nullptr; }
    uint32_t getMipCount() const { return mipLevels; }
  };

  /**
   * \brief Linearized depth cache
   *
   * Stores linearized depth values for effects that need
   * depth-aware processing (SSAO, clarity, SSR).
   */
  struct PostFXDepthCache {
    Rc<DxvkImage>     linearDepth;
    Rc<DxvkImageView> linearDepthView;
    VkExtent2D        extent = { 0, 0 };
    float             nearPlane = 0.1f;
    float             farPlane  = 10000.0f;
    bool              dirty     = true;

    bool valid() const { return linearDepth != nullptr; }
  };

  /**
   * \brief PostFX shared resource manager
   *
   * Manages shared resources used by multiple effects:
   * - Temporal buffer pools (for TAA, temporal clarity, SSR)
   * - Scene mip chain (for clarity, bloom)
   * - Linearized depth cache (for SSAO, clarity, SSR)
   *
   * Resources are created on demand and resized as needed.
   */
  class DxvkPostFXResources : public RcObject {

  public:

    DxvkPostFXResources(DxvkDevice* device);
    ~DxvkPostFXResources();

    /**
     * \brief Begin frame resource management
     *
     * Call at start of frame to prepare resources.
     * Marks depth cache as dirty for regeneration.
     *
     * \param [in] backbufferExtent Current backbuffer size
     */
    void beginFrame(VkExtent2D backbufferExtent);

    /**
     * \brief End frame resource management
     *
     * Call at end of frame to swap temporal buffers.
     */
    void endFrame();

    /**
     * \brief Get or create temporal buffer pair
     *
     * Returns a named temporal buffer pair, creating if needed.
     * Buffers are resized automatically when backbuffer changes.
     *
     * \param [in] name Unique identifier for the buffer pair
     * \param [in] format Desired format (default: RGBA16F)
     * \returns Reference to temporal buffer pair
     */
    PostFXTemporalPair& getTemporalPair(
      const std::string&  name,
            VkFormat      format = VK_FORMAT_R16G16B16A16_SFLOAT);

    /**
     * \brief Get scene mip chain
     *
     * Returns the shared scene mip chain. The mip chain is
     * regenerated each frame from the backbuffer.
     *
     * \returns Reference to mip chain
     */
    const PostFXMipChain& getMipChain() const { return m_mipChain; }

    /**
     * \brief Get linearized depth cache
     *
     * Returns the linearized depth buffer. Must call
     * updateDepthCache() first if dirty flag is set.
     *
     * \returns Reference to depth cache
     */
    const PostFXDepthCache& getDepthCache() const { return m_depthCache; }

    /**
     * \brief Check if depth cache needs update
     */
    bool isDepthCacheDirty() const { return m_depthCache.dirty; }

    /**
     * \brief Update linearized depth cache
     *
     * Linearizes the raw depth buffer for use by effects.
     * Should be called once per frame when depth is needed.
     *
     * \param [in] ctx Graphics context
     * \param [in] rawDepthView Raw depth buffer from D3D9
     * \param [in] nearPlane Camera near plane
     * \param [in] farPlane Camera far plane
     */
    void updateDepthCache(
            DxvkContext*             ctx,
      const Rc<DxvkImageView>&       rawDepthView,
            float                    nearPlane,
            float                    farPlane);

    /**
     * \brief Generate scene mip chain
     *
     * Generates mip pyramid from the scene image.
     * Uses SPD (Single Pass Downsampler) when available.
     *
     * \param [in] ctx Graphics context
     * \param [in] sourceView Source image to generate mips from
     */
    void generateMipChain(
            DxvkContext*             ctx,
      const Rc<DxvkImageView>&       sourceView);

    /**
     * \brief Set depth range for linearization
     *
     * \param [in] nearPlane Camera near plane
     * \param [in] farPlane Camera far plane
     */
    void setDepthRange(float nearPlane, float farPlane);

  private:

    DxvkDevice* m_device;
    VkExtent2D  m_currentExtent = { 0, 0 };

    // Temporal buffer pools (keyed by name)
    std::unordered_map<std::string, PostFXTemporalPair> m_temporalPairs;

    // Shared mip chain for scene
    PostFXMipChain m_mipChain;

    // Linearized depth cache
    PostFXDepthCache m_depthCache;

    /**
     * \brief Create or resize temporal pair
     */
    void ensureTemporalPair(
            PostFXTemporalPair& pair,
            VkExtent2D          extent,
            VkFormat            format);

    /**
     * \brief Create or resize mip chain
     */
    void ensureMipChain(VkExtent2D extent);

    /**
     * \brief Create or resize depth cache
     */
    void ensureDepthCache(VkExtent2D extent);

    /**
     * \brief Create image with given parameters
     */
    Rc<DxvkImage> createImage(
            VkExtent2D extent,
            VkFormat   format,
            uint32_t   mipLevels,
            VkImageUsageFlags usage);

    /**
     * \brief Create image view
     *
     * \param [in] image Source image
     * \param [in] mipLevel Mip level for the view
     * \param [in] usage View usage flags (must match image usage)
     */
    Rc<DxvkImageView> createImageView(
      const Rc<DxvkImage>& image,
            uint32_t       mipLevel,
            VkImageUsageFlags usage);

  };

}
