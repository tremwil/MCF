#pragma clang diagnostic push
#pragma ide diagnostic ignored "google-explicit-constructor"

/* Mod Compatibility Framework
 * Copyright (c) 2022 William Tremblay.
 
 * This program is free software; licensed under the MIT license. 
 * You should have received a copy of the license along with this program. 
 * If not, see <https://opensource.org/licenses/MIT>.
 */

#pragma once

namespace MCF
{
    template<unsigned N>
    struct FixedString
    {
        char buf[N + 1]{};
        constexpr FixedString(char const* s)
        {
            for (unsigned i = 0; i != N; ++i) buf[i] = s[i];
        }
        constexpr operator char const* () const { return buf; }
    };
    template<unsigned N> FixedString(char const (&)[N])->FixedString<N - 1>;
#pragma clang diagnostic pop
}

