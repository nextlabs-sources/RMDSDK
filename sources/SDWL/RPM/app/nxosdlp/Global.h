#pragma once

#include "util.hpp"


class Global {
public:
	Global() :is_inited_madchook(false), has_call_init_hook_api_for_fv(false), is_allow_debug_breakpoint(false){}
public:

	HMODULE this_dll_module;
	bool  is_inited_madchook;
	bool  has_call_init_hook_api_for_fv;


	CRwLock  fv_sw_lock;   // read write lock for fv
	std::vector<void*> fvs;   // file virtualization
	// global control for anti-reentrency
	recursion_control rc; // control hook

	// for debug used
	bool is_allow_debug_breakpoint;
};


extern Global global;


class fv {
public:
	inline void set_path(const wchar_t* t, const wchar_t* r) {
		target.assign(t);
		redirected.assign(r);
	}
public: // event
	// wait for callback of file apis
	bool on_event_detour(std::wstring& p) {
		if (ibegin_with(p, target)) {
			p.replace(0, target.size(), redirected);
			return true;
		}
		return false;
	}
private:
	std::wstring target;
	std::wstring redirected;
};


bool setup_anti_ole_link_embed();

void hook_api_for_fv();