/**
 ******************************************************************************
 * @file    Sprite.hpp
 * @brief   A packed sprite (24x24, or 32x32 for a boss) shown at 2x as an
 *          LVGL image.
 *
 * LVGL v9 decodes indexed images to 32-bit ARGB in its 40 KB pool at draw
 * time, so the generated 4-bit sprites (gui/Sprites.hpp) are not handed to
 * LVGL. Each on-screen Sprite uses a static RGB565A8 buffer (outside the
 * pool) and expands the current frame into it, doubling each pixel; the
 * lv_image points straight at that buffer, which LVGL's software renderer
 * draws without decoding. A frame change is an expand and an invalidate, with
 * no allocation.
 *
 * The buffers are shared by every screen (SpriteBuffers): only one screen is
 * built at a time (ScreenManager frees the old before building the new).
 ******************************************************************************
 */

#ifndef TRAILQUEST_SPRITE_HPP
#define TRAILQUEST_SPRITE_HPP

#include <cstdint>

#include "lvgl.h"

#include "gui/Sprites.hpp"

class Sprite
{
public:
    static constexpr int32_t  kScale    = 2;   ///< Panel pixels per source pixel.
    static constexpr int32_t  kMaxSize  = TRAILQUEST_SPRITE_MAX_SIZE * kScale;   ///< Shown side, at most.
    /// Bytes a buffer for a sprite of logical side @p side needs (RGB565 plane + A8 plane).
    static constexpr uint32_t bytesFor(int32_t side) { return static_cast<uint32_t>(side * kScale * side * kScale * 3); }

    /// Create the image on @p parent at (x, y), showing frame 0 of sprite
    /// @p spriteIndex (a TRAILQUEST_SPRITE_* index). @p buffer must hold
    /// @p capacity bytes (at least bytesFor() the sprite's side), be 4-byte
    /// aligned, and outlive the Sprite. A sprite that does not fit is not shown.
    void create(lv_obj_t* parent, int32_t x, int32_t y, uint8_t spriteIndex, uint8_t* buffer, uint32_t capacity);

    /// Show another sprite in the same image (the buffer must hold it).
    void show(uint8_t spriteIndex);

    /// Show frame 0 or 1.
    void setFrame(uint8_t frame);

    /// The side shown, px (48 or 64).
    int32_t size() const { return mSide * kScale; }

    lv_obj_t* object() const { return mImage; }

private:
    void expand(const uint8_t* packed);
    bool fits(uint8_t spriteIndex) const;

    lv_image_dsc_t mDsc {};
    lv_obj_t*      mImage    = nullptr;
    uint8_t*       mBuffer   = nullptr;
    uint32_t       mCapacity = 0;
    uint8_t        mSprite   = 0;
    int32_t        mSide     = 24;
    uint8_t        mFrame    = 0;
};

/// The static buffers every screen's sprites share: three at 24x24, and one
/// big enough for a boss.
namespace SpriteBuffers
{
constexpr uint32_t kSmall = Sprite::bytesFor(24);
constexpr uint32_t kLarge = Sprite::bytesFor(TRAILQUEST_SPRITE_MAX_SIZE);
uint8_t* small(uint8_t i);   ///< i in 0..2
uint8_t* large();
} // namespace SpriteBuffers

#endif // TRAILQUEST_SPRITE_HPP
