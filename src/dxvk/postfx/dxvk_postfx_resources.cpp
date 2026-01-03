#include "dxvk_postfx_resources.h"

#include "../dxvk_device.h"
#include "../dxvk_context.h"
#include "../dxvk_image.h"

#include "../../util/util_bit.h"
#include "../../util/util_string.h"
#include "../../util/log/log.h"

namespace dxvk {

  DxvkPostFXResources::DxvkPostFXResources(DxvkDevice* device)
  : m_device(device) {
    Logger::info("PostFX: Resources manager initialized");
  }

  DxvkPostFXResources::~DxvkPostFXResources() {
    Logger::info("PostFX: Resources manager destroyed");
  }

  void DxvkPostFXResources::beginFrame(VkExtent2D backbufferExtent) {
    // Check if resolution changed
    if (m_currentExtent.width  != backbufferExtent.width ||
        m_currentExtent.height != backbufferExtent.height) {
      m_currentExtent = backbufferExtent;

      // Resize existing temporal pairs (lazy: only if already allocated)
      for (auto& entry : m_temporalPairs) {
        ensureTemporalPair(entry.second, m_currentExtent, entry.second.format);
      }

      // Mip chain and depth cache are resized lazily when first requested
      // (generateMipChain / updateDepthCache will call ensure* as needed)
    }

    // Mark depth cache as needing update this frame
    m_depthCache.dirty = true;
  }

  void DxvkPostFXResources::endFrame() {
    // Swap all temporal pairs
    for (auto& entry : m_temporalPairs) {
      entry.second.swap();
    }
  }

  PostFXTemporalPair& DxvkPostFXResources::getTemporalPair(
    const std::string& name,
    VkFormat           format) {
    auto it = m_temporalPairs.find(name);

    if (it == m_temporalPairs.end()) {
      // Create new pair
      PostFXTemporalPair pair;
      pair.format = format;
      ensureTemporalPair(pair, m_currentExtent, format);

      auto result = m_temporalPairs.emplace(name, std::move(pair));
      return result.first->second;
    }

    // Check for format mismatch - recreate if different format requested
    if (it->second.format != format) {
      Logger::warn(str::format(
        "PostFX: Temporal pair '", name, "' format mismatch: ",
        "requested ", format, " but have ", it->second.format,
        ". Recreating with new format."));
      it->second.format = format;
      ensureTemporalPair(it->second, m_currentExtent, format);
    }

    return it->second;
  }

  void DxvkPostFXResources::updateDepthCache(
          DxvkContext*             ctx,
    const Rc<DxvkImageView>&       rawDepthView,
          float                    nearPlane,
          float                    farPlane) {
    if (!rawDepthView)
      return;

    // Lazy allocation: ensure depth cache exists at current resolution
    ensureDepthCache(m_currentExtent);

    if (!m_depthCache.valid())
      return;

    m_depthCache.nearPlane = nearPlane;
    m_depthCache.farPlane  = farPlane;

    // TODO: Dispatch depth linearization compute shader
    // For now, this is a placeholder. The actual implementation
    // will be added when we have the compute shader infrastructure.
    // DO NOT set dirty = false until shader is implemented!
  }

  void DxvkPostFXResources::generateMipChain(
          DxvkContext*             ctx,
    const Rc<DxvkImageView>&       sourceView) {
    if (!sourceView)
      return;

    // Lazy allocation: ensure mip chain exists at current resolution
    ensureMipChain(m_currentExtent);

    if (!m_mipChain.valid())
      return;

    // TODO: Generate mip chain using SPD or fallback
    // For now, this is a placeholder. The actual implementation
    // will use either SPD (Single Pass Downsampler) if subgroup
    // operations are available, or a multi-pass fallback.
  }

  void DxvkPostFXResources::setDepthRange(float nearPlane, float farPlane) {
    m_depthCache.nearPlane = nearPlane;
    m_depthCache.farPlane  = farPlane;
    m_depthCache.dirty     = true;
  }

  void DxvkPostFXResources::ensureTemporalPair(
          PostFXTemporalPair& pair,
          VkExtent2D          extent,
          VkFormat            format) {
    if (extent.width == 0 || extent.height == 0)
      return;

    // Skip if already valid with matching extent and format
    if (pair.valid() &&
        pair.extent.width == extent.width &&
        pair.extent.height == extent.height &&
        pair.format == format)
      return;

    VkImageUsageFlags usage =
      VK_IMAGE_USAGE_SAMPLED_BIT |
      VK_IMAGE_USAGE_STORAGE_BIT |
      VK_IMAGE_USAGE_TRANSFER_DST_BIT;

    VkImageUsageFlags viewUsage =
      VK_IMAGE_USAGE_SAMPLED_BIT |
      VK_IMAGE_USAGE_STORAGE_BIT;

    for (uint32_t i = 0; i < 2; i++) {
      pair.images[i] = createImage(extent, format, 1, usage);
      pair.views[i]  = createImageView(pair.images[i], 0, viewUsage);
    }

    pair.extent       = extent;
    pair.format       = format;
    pair.currentIndex = 0;
    pair.historyValid = false;  // History invalid until first swap
  }

  void DxvkPostFXResources::ensureMipChain(VkExtent2D extent) {
    if (extent.width == 0 || extent.height == 0)
      return;

    if (m_mipChain.valid() &&
        m_mipChain.baseExtent.width == extent.width &&
        m_mipChain.baseExtent.height == extent.height)
      return;

    // Calculate mip levels using bit scan (safer than loop, no overflow risk)
    uint32_t maxDim = std::max(extent.width, extent.height);
    uint32_t mipLevels = maxDim > 0 ? (32u - bit::lzcnt(maxDim)) : 1u;

    // Cap at reasonable level (12 for 4K is sufficient)
    mipLevels = std::min(mipLevels, 12u);

    VkImageUsageFlags usage =
      VK_IMAGE_USAGE_SAMPLED_BIT |
      VK_IMAGE_USAGE_STORAGE_BIT |
      VK_IMAGE_USAGE_TRANSFER_DST_BIT;

    m_mipChain.image      = createImage(extent, VK_FORMAT_R16G16B16A16_SFLOAT, mipLevels, usage);
    m_mipChain.baseExtent = extent;
    m_mipChain.mipLevels  = mipLevels;

    // Create views for each mip level (need both sampled and storage for SPD)
    VkImageUsageFlags viewUsage =
      VK_IMAGE_USAGE_SAMPLED_BIT |
      VK_IMAGE_USAGE_STORAGE_BIT;

    m_mipChain.mipViews.clear();
    m_mipChain.mipViews.reserve(mipLevels);

    for (uint32_t i = 0; i < mipLevels; i++) {
      m_mipChain.mipViews.push_back(createImageView(m_mipChain.image, i, viewUsage));
    }

    Logger::info(str::format(
      "PostFX: Created mip chain with ", mipLevels, " levels"));
  }

  void DxvkPostFXResources::ensureDepthCache(VkExtent2D extent) {
    if (extent.width == 0 || extent.height == 0)
      return;

    if (m_depthCache.valid() &&
        m_depthCache.extent.width == extent.width &&
        m_depthCache.extent.height == extent.height)
      return;

    VkImageUsageFlags usage =
      VK_IMAGE_USAGE_SAMPLED_BIT |
      VK_IMAGE_USAGE_STORAGE_BIT;

    // R32F for linearized depth
    m_depthCache.linearDepth     = createImage(extent, VK_FORMAT_R32_SFLOAT, 1, usage);
    m_depthCache.linearDepthView = createImageView(m_depthCache.linearDepth, 0, usage);
    m_depthCache.extent          = extent;
    m_depthCache.dirty           = true;

    Logger::info("PostFX: Created linearized depth cache");
  }

  Rc<DxvkImage> DxvkPostFXResources::createImage(
          VkExtent2D extent,
          VkFormat   format,
          uint32_t   mipLevels,
          VkImageUsageFlags usage) {
    DxvkImageCreateInfo info;
    info.type        = VK_IMAGE_TYPE_2D;
    info.format      = format;
    info.flags       = 0;
    info.sampleCount = VK_SAMPLE_COUNT_1_BIT;
    info.extent      = { extent.width, extent.height, 1 };
    info.numLayers   = 1;
    info.mipLevels   = mipLevels;
    info.usage       = usage;
    info.stages      = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT |
                       VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    info.access      = VK_ACCESS_SHADER_READ_BIT |
                       VK_ACCESS_SHADER_WRITE_BIT;
    info.tiling      = VK_IMAGE_TILING_OPTIMAL;
    info.layout      = VK_IMAGE_LAYOUT_GENERAL;

    // Add transfer stage/access if transfer usage is requested
    if (usage & VK_IMAGE_USAGE_TRANSFER_DST_BIT) {
      info.stages |= VK_PIPELINE_STAGE_TRANSFER_BIT;
      info.access |= VK_ACCESS_TRANSFER_WRITE_BIT;
    }

    if (usage & VK_IMAGE_USAGE_TRANSFER_SRC_BIT) {
      info.stages |= VK_PIPELINE_STAGE_TRANSFER_BIT;
      info.access |= VK_ACCESS_TRANSFER_READ_BIT;
    }

    return m_device->createImage(info, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
  }

  Rc<DxvkImageView> DxvkPostFXResources::createImageView(
    const Rc<DxvkImage>& image,
          uint32_t       mipLevel,
          VkImageUsageFlags usage) {
    DxvkImageViewKey viewInfo;
    viewInfo.viewType   = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format     = image->info().format;
    viewInfo.usage      = VkImageUsageFlagBits(usage);
    viewInfo.aspects    = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.mipIndex   = static_cast<uint8_t>(mipLevel);
    viewInfo.mipCount   = 1;
    viewInfo.layerIndex = 0;
    viewInfo.layerCount = 1;

    return image->createView(viewInfo);
  }

}
