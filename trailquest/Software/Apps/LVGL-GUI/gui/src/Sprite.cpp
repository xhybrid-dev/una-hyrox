/**
 ******************************************************************************
 * @file    Sprite.cpp
 * @brief   A packed sprite shown at 2x (see Sprite.hpp).
 ******************************************************************************
 */

#include "gui/Sprite.hpp"

namespace
{
alignas(4) uint8_t sSmall[3][SpriteBuffers::kSmall];
alignas(4) uint8_t sLarge[SpriteBuffers::kLarge];
} // namespace

uint8_t* SpriteBuffers::small(uint8_t i)
{
    return sSmall[i < 3 ? i : 0];
}

uint8_t* SpriteBuffers::large()
{
    return sLarge;
}

bool Sprite::fits(uint8_t spriteIndex) const
{
    // No MMU: an index past the table would read whatever lies beyond it.
    return spriteIndex < TRAILQUEST_SPRITE_COUNT && bytesFor(trailquest_sprites[spriteIndex].size) <= mCapacity;
}

void Sprite::create(lv_obj_t* parent, int32_t x, int32_t y, uint8_t spriteIndex, uint8_t* buffer, uint32_t capacity)
{
    mBuffer   = buffer;
    mCapacity = buffer != nullptr ? capacity : 0;
    mImage    = lv_image_create(parent);
    lv_obj_set_pos(mImage, x, y);
    show(spriteIndex);
}

void Sprite::show(uint8_t spriteIndex)
{
    if (mImage == nullptr || !fits(spriteIndex)) {
        if (mImage) {
            lv_obj_add_flag(mImage, LV_OBJ_FLAG_HIDDEN);
        }
        return;
    }
    mSprite = spriteIndex;
    mSide   = trailquest_sprites[mSprite].size;
    mFrame  = 0;

    const int32_t shown = mSide * kScale;
    mDsc.header.magic  = LV_IMAGE_HEADER_MAGIC;
    mDsc.header.cf     = LV_COLOR_FORMAT_RGB565A8;
    mDsc.header.flags  = 0;
    mDsc.header.w      = shown;
    mDsc.header.h      = shown;
    mDsc.header.stride = shown * 2;   // the RGB565 plane's row; the A8 plane follows it
    mDsc.data_size     = bytesFor(mSide);
    mDsc.data          = mBuffer;

    expand(trailquest_sprites[mSprite].frame[0]);
    lv_image_set_src(mImage, &mDsc);
    lv_obj_remove_flag(mImage, LV_OBJ_FLAG_HIDDEN);
    lv_obj_invalidate(mImage);
}

void Sprite::setFrame(uint8_t frame)
{
    frame &= 1u;
    if (mImage == nullptr || frame == mFrame || !fits(mSprite)) {
        return;
    }
    mFrame = frame;
    expand(trailquest_sprites[mSprite].frame[frame]);
    lv_obj_invalidate(mImage);
}

void Sprite::expand(const uint8_t* packed)
{
    const int32_t  shown = mSide * kScale;
    const uint32_t plane = static_cast<uint32_t>(shown * shown);
    uint8_t* const alpha = mBuffer + plane * 2;

    for (int32_t sy = 0; sy < mSide; ++sy) {
        for (int32_t sx = 0; sx < mSide; ++sx) {
            const uint32_t i   = static_cast<uint32_t>(sy * mSide + sx);
            const uint8_t  idx = (i & 1u) ? (packed[i >> 1] & 0x0Fu) : (packed[i >> 1] >> 4);
            const uint32_t c   = trailquest_palette[idx];
            const uint16_t px  = static_cast<uint16_t>(((c >> 8) & 0xF800u) | ((c >> 5) & 0x07E0u) | ((c >> 3) & 0x001Fu));
            const uint8_t  a   = idx == 0 ? 0u : 255u;

            for (int32_t dy = 0; dy < kScale; ++dy) {
                for (int32_t dx = 0; dx < kScale; ++dx) {
                    const uint32_t o = static_cast<uint32_t>((sy * kScale + dy) * shown + sx * kScale + dx);
                    mBuffer[o * 2]     = static_cast<uint8_t>(px & 0xFFu);   // little endian
                    mBuffer[o * 2 + 1] = static_cast<uint8_t>(px >> 8);
                    alpha[o]           = a;
                }
            }
        }
    }
}
