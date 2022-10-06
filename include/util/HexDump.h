
/* Mod Compatibility Framework
 * Copyright (c) 2022 William Tremblay.
 
 * This program is free software; licensed under the MIT license. 
 * You should have received a copy of the license along with this program. 
 * If not, see <https://opensource.org/licenses/MIT>.
 */

#pragma once
#include <string>
#include <vector>
#include <iomanip>
#include <sstream>
#include <type_traits>

namespace MCF::Utils
{
    std::string BytesToHexStr(const void* bytes, size_t size, bool spaces = false)
    {
        std::stringstream ss;
        ss << std::setfill('0') << std::hex;
        for (int i = 0; i < size; i++) {
            ss << std::setw(2) << (int)((uint8_t*)bytes)[i];
            if (spaces) ss << ' ';
        }
        return ss.str();
    }

    template<class It> requires std::is_same_v<typename std::iterator_traits<It>::value_type, uint8_t>
    std::string BytesToHexStr(It first, It last, bool spaces = false)
    {
        std::stringstream ss;
        ss << std::setfill('0') << std::hex;

        for (; first != last; first++) {
            ss << std::setw(2) << (int)*first;
            if (spaces) ss << ' ';
        }
        return ss.str();
    }

    template<class C>
    std::string BytesToHexStr(const C& container, bool spaces = false)
    {
        return BytesToHexStr(container.begin(), container.end(), spaces);
    }

    uint8_t HexToInt(char input)
    {
        if (input >= '0' && input <= '9')
            return input - '0';
        if (input >= 'A' && input <= 'F')
            return input - 'A' + 10;
        if (input >= 'a' && input <= 'f')
            return input - 'a' + 10;
        throw std::invalid_argument("Invalid input string");
    }

    void HexStrToBytes(const std::string& byteStr, uint8_t* buff, size_t size)
    {
        if (byteStr.size() & 1) throw std::invalid_argument("Invalid input string");
        if (byteStr.size() > 2*size) throw std::invalid_argument("Buffer too small");

        for (size_t i = 0; 2 * i < byteStr.size() && i < size; i++)
            buff[i] = HexToInt(byteStr[2 * i]) << 4 | HexToInt(byteStr[2 * i + 1]);
    }

    std::vector<uint8_t> HexStrToBytes(const std::string& byteStr)
    {
        if (byteStr.size() & 1) throw std::invalid_argument("Invalid input string");

        std::vector<uint8_t> buff;
        for (size_t i = 0; 2 * i < byteStr.size(); i++)
            buff.push_back(HexToInt(byteStr[2 * i]) << 4 | HexToInt(byteStr[2 * i + 1]));

        return buff;
    }
}