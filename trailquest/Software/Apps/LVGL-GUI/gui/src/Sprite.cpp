/**
 ******************************************************************************
 * @file    Sprite.cpp
 * @brief   A 24x24 packed sprite shown at 2x (see Sprite.hpp).
 ******************************************************************************
 */

#include "gui/Sprite.hpp"

#include "gui/Sprites.hpp"

void Sprite::create(lv_obj_t* parent, int32_t x, int32_t y, uint8_t spriteIndex, uint8_t* buffer)
{
    mBuffer = buffer;
    mSprite = spriteIndex;

    mDsc.header.magic  = LV_IMAGE_HEADER_MAGIC;
    mDsc.header.cf     = LV_COLOR_FORMAT_RGB565A8;
    mDsc.header.flags  = 0;
    mDsc.header.w      = kSize;
    mDsc.header.h      = kSize;
    mDsc.header.stride = kSize * 2;   // the RGB565 plane's row; the A8 plane follows it
    mDsc.data_size     = kBytes;
    mDsc.data          = mBuffer;

    expand(trailquest_sprites[mSprite].frame[0]);

    mImage = lv_image_create(parent);
    lv_image_set_src(mImage, &mDsc);
    lv_obj_set_pos(mImage, x, y);
}

void Sprite::setFrame(uint8_t frame)
{
    expand(trailquest_sprites[mSprite].frame[frame & 1u]);
    if (mImage) {
        lv_obj_invalidate(mImage);
    }
}

void Sprite::expand(const uint8_t* packed)
{
    constexpr uint32_t kPlane = kSize * kSize;
    uint8_t* const alpha = mBuffer + kPlane * 2;

    for (int32_t sy = 0; sy < kLogical; ++sy) {
        for (int32_t sx = 0; sx < kLogical; ++sx) {
            const uint32_t i   = static_cast<uint32_t>(sy * kLogical + sx);
            const uint8_t  idx = (i & 1u) ? (packed[i >> 1] & 0x0Fu) : (packed[i >> 1] >> 4);
            const uint32_t c   = trailquest_palette[idx];
            const uint16_t px  = static_cast<uint16_t>(((c >> 8) & 0xF800u) | ((c >> 5) & 0x07E0u) | ((c >> 3) & 0x001Fu));
            const uint8_t  a   = idx == 0 ? 0u : 255u;

            for (int32_t dy = 0; dy < kScale; ++dy) {
                for (int32_t dx = 0; dx < kScale; ++dx) {
                    const uint32_t o = static_cast<uint32_t>((sy * kScale + dy) * kSize + sx * kScale + dx);
                    mBuffer[o * 2]     = static_cast<uint8_t>(px & 0xFFu);   // little endian
                    mBuffer[o * 2 + 1] = static_cast<uint8_t>(px >> 8);
                    alpha[o]           = a;
                }
            }
        }
    }
}
