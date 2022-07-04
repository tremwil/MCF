#pragma once
#include "SharedInterface.h"
#include <vector>
#include <typeinfo>

namespace MCF
{
	class AutoExportBase
	{
	protected:
		static std::vector<const CompInfo*> comp_infos;

	public:
		static const MCF::CompInfo** Export(size_t* n)
		{
			*n = AutoExportBase::comp_infos.size();
			return AutoExportBase::comp_infos.data();
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
			const CompInfo* info = T::ComponentInfoStatic();
			printf("%s init!\n", info->version_string);
			AutoExportBase::comp_infos.push_back(info);
		}
		static int AddOnce()
		{
			static AutoExport ex;
			return 0;
		}

		static int ex;
	};
}
#define MCF_COMPONENT_EXPORT(T) int AutoExport<T>::ex = AutoExport<T>::AddOnce();

extern "C" __declspec(dllexport) const MCF::CompInfo** MCF_GetExportedInterfaces(size_t* n);