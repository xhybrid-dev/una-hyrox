/**
 ******************************************************************************
 * @file    Sprite.hpp
 * @brief   A 24x24 packed sprite shown at 2x as a 48x48 LVGL image.
 *
 * LVGL v9 decodes indexed images to 32-bit ARGB in its 40 KB pool at draw
 * time, so the generated 4-bit sprites (gui/Sprites.hpp) are not handed to
 * LVGL. Each on-screen Sprite owns a static RGB565A8 buffer (6,912 bytes,
 * outside the pool) and expands the current frame into it, doubling each
 * pixel; the lv_image points straight at that buffer, which LVGL's software
 * renderer draws without decoding. A frame change is an expand and an
 * invalidate, with no allocation.
 ******************************************************************************
 */

#ifndef TRAILQUEST_SPRITE_HPP
#define TRAILQUEST_SPRITE_HPP

#include <cstdint>

#include "lvgl.h"

class Sprite
{
public:
    static constexpr int32_t kLogical = 24;                 ///< Source size, pixels.
    static constexpr int32_t kScale   = 2;                  ///< Panel pixels per source pixel.
    static constexpr int32_t kSize    = kLogical * kScale;  ///< Shown size, 48.
    static constexpr uint32_t kBytes  = kSize * kSize * 3;  ///< RGB565 plane + A8 plane.

    /// Create the image on @p parent at (x, y), showing frame 0 of sprite
    /// @p spriteIndex (a TRAILQUEST_SPRITE_* index). @p buffer must hold
    /// kBytes, be 4-byte aligned, and outlive the Sprite.
    void create(lv_obj_t* parent, int32_t x, int32_t y, uint8_t spriteIndex, uint8_t* buffer);

    /// Show frame 0 or 1.
    void setFrame(uint8_t frame);

    lv_obj_t* object() const { return mImage; }

private:
    void expand(const uint8_t* packed);

    lv_image_dsc_t mDsc {};
    lv_obj_t*      mImage  = nullptr;
    uint8_t*       mBuffer = nullptr;
    uint8_t        mSprite = 0;
};

#endif // TRAILQUEST_SPRITE_HPP
