#include "stdafx.h"
#include "config.h"
#include "global_data_model.h"
#include "registry_whitelist.h"


NXCONFIG_NAMESPACE

#ifdef _DEBUG

void load_debug_watermark() {
	using namespace nx;
	using namespace nx::utils;
	auto load_view_overlay = [](overlay::WatermarkParam& param) {
		// for view_overlay and print_overlay
		Registry::param rp(LR"(Software\NextLabs\SkyDRM\OSRMX\watermark)");
		Registry r;
		std::wstring text;
		if (!r.get(rp, L"text", text) || text.empty()) {
			return false;
		}
		{
			// change escape chr in text \n \t 
			const static std::map<std::wstring, std::wstring> pattern{
				{L"\\n",L"\n"},
				{L"\\t",L"\t"},
				{L"$(User)",L"Osmond.Ye"},
				{L"$(Email)",L"Osmond.Ye@nextlabs.com"},
				{L"$(Date)",L"2019-10-30"},
				{L"$(Time)",L"21:33:21"},
			};
			for (const auto& p : pattern) {
				size_t pos = 0;
				while ((pos = text.find(p.first, pos)) != text.npos) {
					text.replace(pos, p.first.size(), p.second);
					pos += p.second.size();
				}
			}
		}

		overlay::WatermarkParamBuilder builder;
		builder.text(text);

		if (r.get(rp, L"font_name", text)) {
			builder.font_name(text);
		}
		std::uint32_t data;
		if (r.get(rp, L"font_size", data)) {
			builder.font_size(data);
		}
		if (r.get(rp, L"font_style", data)) {
			builder.font_style((overlay::WatermarkParam::FontStyle)data);
		}
		if (r.get(rp, L"text_rotation", data)) {
			builder.text_rotation(data);
		}
		if (r.get(rp, L"text_alignment", data)) {
			builder.text_alignment((overlay::WatermarkParam::TextAlignment)data);
		}
		if (r.get(rp, L"line_alignment", data)) {
			builder.line_alignment((overlay::WatermarkParam::TextAlignment)data);
		}
		if (r.get(rp, L"text_color", data)) {
			builder.text_color(data);
		}
		param = builder.Build();
		return true;

	};

	overlay::WatermarkParam param;
	if (load_view_overlay(param)) {
		global.default_watermark = param;
	}
}

void load_debug_rights() {
	using namespace nx;
	using namespace nx::utils;
	auto load_from_registry = [](const std::wstring& enable_right_name) {
		bool rt = true;  // by default
		// for view_overlay and print_overlay
		Registry::param rp(LR"(Software\NextLabs\SkyDRM\OSRMX)");
		Registry r;
		std::uint32_t data;
		if (!r.get(rp, enable_right_name, data)) {
			return rt;
		}
		return data == 1;
	};

	global.whitelistActionRights.enable_edit = load_from_registry(L"enable_edit");
	global.whitelistActionRights.enable_save_as = load_from_registry(L"enable_save_as");
	global.whitelistActionRights.enable_print = load_from_registry(L"enable_print");
	global.whitelistActionRights.enable_clipboard = load_from_registry(L"enable_clipboard");
	global.whitelistActionRights.enable_screen_capture = load_from_registry(L"enable_screen_capture");
}

void load_debug_dev_configruaitons() {
	load_debug_watermark();
	load_debug_rights();
}
#endif // _DEBUG

void load_app_whitelist_config()
{
	global.whitelistActionRights.enable_view = nx::cregistry_whitelist::getInstance()->is_app_can_view();
	global.whitelistActionRights.enable_edit = nx::cregistry_whitelist::getInstance()->is_app_can_save();
	global.whitelistActionRights.enable_save_as = nx::cregistry_whitelist::getInstance()->is_app_can_saveas();
	global.whitelistActionRights.enable_print = nx::cregistry_whitelist::getInstance()->is_app_can_print();
	global.whitelistActionRights.enable_clipboard = nx::cregistry_whitelist::getInstance()->is_app_can_copycontent();
	global.whitelistActionRights.enable_screen_capture = nx::cregistry_whitelist::getInstance()->is_app_can_printscreen();
}

END_NXCONFIG_NAMESPACE