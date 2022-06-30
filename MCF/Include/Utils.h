#pragma once
#include <stdint.h>

namespace MCF
{
	namespace Utils
	{
		/// <summary>
		/// converts a hexadecimal character to a number, or 255 if not valid hex.
		/// </summary>
		/// <param name="c"></param>
		/// <returns></returns>
		inline uint8_t ChrToHex(char c)
		{
			if (c >= 'a' && c <= 'f')
				return 10 + c - 'a';

			if (c >= 'A' && c <= 'F')
				return 10 + c - 'A';

			if (c >= '0' && c <= '9')
				return c - '0';

			return 0xFF;
		}
	}
}