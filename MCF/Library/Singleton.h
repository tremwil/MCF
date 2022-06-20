#pragma once
#include "AobScanMgr.h"

namespace MCF
{
	/// <summary>
	/// Simple template implementing a singleton pattern in a derived class
	/// </summary>
	/// <typeparam name="T"></typeparam>
	template<typename T>
	class Singleton
	{
	public:
		Singleton(Singleton& const) = delete;
		Singleton(Singleton&&) = delete;

		static T& Ins()
		{
			static T ins;
			return ins;
		}

	private:
		Singleton() { }
	};
}