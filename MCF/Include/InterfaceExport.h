#pragma once
#include "SharedInterface.h"
#include <vector>
#include <typeinfo>

namespace MCF
{
	class AutoExportBase
	{
	protected:
		static std::vector<const ImpDetails*> metas;

	public:
		static const MCF::ImpDetails** Export(size_t* n)
		{
			*n = AutoExportBase::metas.size();
			return AutoExportBase::metas.data();
		}
	};

	// Class template which can be used to automatically register a shared interface implementation
	// on boot (if you add InterfaceExport.cpp to your project). If you prefer to 
	// do things manually, you will have to define MCF_GetExportedInterfacesMeta yourself.
	template<typename T>
	struct AutoExport : public AutoExportBase
	{
	private:
		AutoExport()
		{
			const ImpDetails* meta = T::ImpDetailsStatic();
			printf("%s init!\n", meta->version_string);
			AutoExportBase::metas.push_back(meta);
		}
		static int AddOnce()
		{
			static AutoExport ex;
			return 0;
		}

		static int ex;
	};
}
#define MCF_INTERFACE_EXPORT(T) int AutoExport<T>::ex = AutoExport<T>::AddOnce();

extern "C" __declspec(dllexport) const MCF::ImpDetails** MCF_GetExportedInterfacesMeta(size_t* n);