#pragma once
#include "SharedInterface.h"
#include <vector>
#include <typeinfo>

namespace MCF
{
	// Struct passed to MCF_ExportInterfaces containing metadata about
	// a given shared interface. 
	struct SIMeta
	{
		// new() operator for this interface, allocates and calls ctor
		SharedInterfaceBase* (*new_fcn)();
		// delete() operator for this interface, calls dtor and frees
		void (*delete_fun)(SharedInterfaceBase*);

		const char* version_string;
		const char** dependencies;
		size_t num_dependencies;

		// Generate interface metadata given a type. 
		template<typename T>
		static SIMeta* Get()
		{			
			static SIMeta meta
			{
				&OpNew<T>,
				&OpDelete<T>,
				T::version_string,
				nullptr,
				0
			};
			meta.dependencies = T::StaticDependencies(&meta.num_dependencies);
			return &meta;
		}

	private:
		template<typename T>
		static SharedInterfaceBase* OpNew() { return new T; }

		template<typename T>
		static void OpDelete(SharedInterfaceBase* obj) { delete (T*)obj; }

	};

	class AutoExportBase
	{
	protected:
		static std::vector<SIMeta*> metas;

	public:
		static MCF::SIMeta** Export(size_t* n)
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
			SIMeta* meta = SIMeta::Get<T>();
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

extern "C" __declspec(dllexport) MCF::SIMeta** MCF_GetExportedInterfacesMeta(size_t* n);