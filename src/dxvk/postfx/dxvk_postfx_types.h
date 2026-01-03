#pragma once

#include <cstdint>

namespace dxvk {

  /**
   * \brief PostFX processing phases
   *
   * Defines when effects should execute in the rendering pipeline.
   * Effects register for a phase and are executed in order within that phase.
   */
  enum class PostFXPhase : uint8_t {
    PreTonemapping,   // SSAO, SSR, Bloom - before game's tonemapper
    PostTonemapping,  // Clarity, color grading - after tonemapper
    PreUI             // Debanding, sharpening - right before UI
  };

  /**
   * \brief Resource access mode for temporal buffers
   */
  enum class PostFXAccess : uint8_t {
    Read,
    Write,
    ReadWrite
  };

  /**
   * \brief Mip chain generation mode
   */
  enum class PostFXMipMode : uint8_t {
    Average,    // Standard box filter (default)
    Min,        // Minimum value (for depth)
    Max         // Maximum value
  };

}
