
/* Mod Compatibility Framework
 * Copyright (c) 2022 William Tremblay.
 
 * This program is free software; licensed under the MIT license. 
 * You should have received a copy of the license along with this program. 
 * If not, see <https://opensource.org/licenses/MIT>.
 *
 * Credit: This (partial) header comes from the Steamless project (https://github.com/atom0s/Steamless/).
 */

#pragma once
#include <cstdint>

namespace MCF
{
    struct SteamStubHeader31
    {
        uint32_t xor_key;
        uint32_t signature; //0xC0DEC0DF for 3.1
        uint64_t  image_base;
        uint64_t drm_entry_point;
        uint32_t bind_section_offset;
        uint32_t bind_section_code_size;
        uint64_t original_entry_point; // The only thing we actually care about here
    };

    static void SteamXorDecrypt(void* dest, const void* src, size_t size, uint32_t key = 0)
    {
        auto idest = (uint32_t*)dest;
        auto isrc = (uint32_t*)src;

        int i = 0;
        if (key == 0) {
            key = isrc[i];
            idest[i++] = key;
        }

        for (; i < size / 4; i++) {
            uint32_t tmp = isrc[i]; // Support in-place decryption
            idest[i] = key ^ tmp;
            key = tmp;
        }
    }

    static void SteamXorEncrypt(void* dest, const void* src, size_t size, uint32_t key = 0) {
        auto idest = (uint32_t*)dest;
        auto isrc = (uint32_t*)src;

        int i = 0;
        if (key == 0) {
            key = isrc[i];
            idest[i++] = key;
        }

        for (; i < size / 4; i++) {
            idest[i] = key ^ isrc[i];
            key = idest[i];
        }
    }
}