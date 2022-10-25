#include "pch.h"
#include "exported_interface.h"
#include "util.hpp"
#include "Global.h"


bool _stdcall  init_file_virtualization(HANDLE* outhandler) {
	// in each context, only support one instance
	fv* pfv = new fv;
	if (pfv == nullptr) {
		DEVLOG(L"failed to new fv");
		return false;
	}
	{
		CRwExclusiveLocker l(&global.fv_sw_lock);
		global.fvs.push_back(pfv);
	}
	*outhandler = pfv;
	if (!global.has_call_init_hook_api_for_fv) {
		hook_api_for_fv();
		global.has_call_init_hook_api_for_fv = true;
	}

	return true;
}


bool _stdcall set_fv_path(HANDLE handler, const wchar_t* target, const wchar_t* redirected) {
	// sanity check
	bool found = false;
	{
		CRwSharedLocker l(&global.fv_sw_lock);
		for (auto i : global.fvs) {
			if (i == handler) {
				found = true;
				break;
			}
		}
	}
	if (!found) {
		DEVLOG(L"invalid handler");
		return false;
	}
	if (target == NULL) {
		DEVLOG(L"invalid target path");
		return false;
	}
	if (redirected == NULL) {
		DEVLOG(L"invalid redirected path");
		return false;
	}

	// todo, additional check if path exsit;
	((fv*)handler)->set_path(target, redirected);

	return true;
}