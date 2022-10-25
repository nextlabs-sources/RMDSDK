

#include <windows.h>
#include <Psapi.h>

#include <map>
#include <memory>

#include <boost/algorithm/string.hpp>

#include <nudf\eh.hpp>
#include <nudf\debug.hpp>
#include <nudf\string.hpp>
#include <nudf\filesys.hpp>
#include <nudf\handyutil.hpp>

#include <nudf/shared/rightsdef.h>

#include "nxrmflt.h"
#include "process.hpp"


namespace {


class pe_cache
{
public:
    pe_cache()
    {
    }
    
    ~pe_cache()
    {
    }

    std::shared_ptr<NX::win::pe_file> find(const std::wstring& image_path)
    {
        std::shared_ptr<NX::win::pe_file> info_ptr;
        std::wstring normalized_image_path(image_path);
        std::transform(normalized_image_path.begin(), normalized_image_path.end(), normalized_image_path.begin(), tolower);

        NX::utility::CRwSharedLocker locker(&_lock);
        auto pos = _map.find(normalized_image_path);
        if (pos != _map.end()) {
            info_ptr = (*pos).second;
        }

        return info_ptr;
    }

    void insert(const std::wstring& image_path, const NX::win::pe_file& info)
    {
        std::wstring normalized_image_path(image_path);
        std::transform(normalized_image_path.begin(), normalized_image_path.end(), normalized_image_path.begin(), tolower);
        std::shared_ptr<NX::win::pe_file> info_ptr(new NX::win::pe_file(info));
        if (info_ptr != nullptr) {
            NX::utility::CRwExclusiveLocker locker(&_lock);
            _map[normalized_image_path] = info_ptr;
        }
    }

    void insert(const std::wstring& image_path, const std::shared_ptr<NX::win::pe_file>& info_ptr)
    {
        std::wstring normalized_image_path(image_path);
        std::transform(normalized_image_path.begin(), normalized_image_path.end(), normalized_image_path.begin(), tolower);
        if (info_ptr != nullptr) {
            NX::utility::CRwExclusiveLocker locker(&_lock);
            _map[normalized_image_path] = info_ptr;
        }
    }

    void remove(const std::wstring& image_path)
    {
        std::wstring normalized_image_path(image_path);
        std::transform(normalized_image_path.begin(), normalized_image_path.end(), normalized_image_path.begin(), tolower);
        NX::utility::CRwExclusiveLocker locker(&_lock);
        _map.erase(normalized_image_path);
    }

private:
    std::map<std::wstring, std::shared_ptr<NX::win::pe_file>>  _map;
    mutable NX::utility::CRwLock    _lock;
};

}

static pe_cache PE_CACHE;


process_forbidden_rights::process_forbidden_rights()
{
    clear();
}

process_forbidden_rights::~process_forbidden_rights()
{
}

void process_forbidden_rights::disable_right(RIGHT_ID id)
{
    increase_forbidden_count(id);
}

void process_forbidden_rights::enable_right(RIGHT_ID id, bool force)
{
    if (id < _RIGHT_MAX) {
        if (force) {
            _rights_counter[id] = 0;
        }
        else {
            decrease_forbidden_count(id);
        }
    }
}

void process_forbidden_rights::increase_forbidden_count(RIGHT_ID id)
{
    if (id < _RIGHT_MAX) {
        if (0xFFFF == _rights_counter[id]) {
            _rights_counter[id] = 1;
        }
        else {
            _rights_counter[id] = _rights_counter[id] + 1;
        }
    }
}

void process_forbidden_rights::decrease_forbidden_count(RIGHT_ID id)
{
    if (id < _RIGHT_MAX && 0 != _rights_counter[id]) {
        _rights_counter[id] = _rights_counter[id] - 1;
    }
}

bool process_forbidden_rights::is_right_forbidden(RIGHT_ID id) const
{
    bool result = true;
    if (id < _RIGHT_MAX) {
        if (0 == _rights_counter[id]) {
            result = false;
        }
    }
    return result;
}

void process_forbidden_rights::set_rights(unsigned __int64 desired_rights)
{
    // No VIEW rights
    if (!flags64_on(desired_rights, BUILTIN_RIGHT_VIEW)) {
        disable_right(_RIGHT_VIEW);
        disable_right(_RIGHT_EDIT);
        disable_right(_RIGHT_PRINT);
        disable_right(_RIGHT_CLIPBOARD);
        disable_right(_RIGHT_SAVEAS);
        disable_right(_RIGHT_DECRYPT);
        disable_right(_RIGHT_SCREENCAP);
        disable_right(_RIGHT_SEND);
        disable_right(_RIGHT_CLASSIFY);
        disable_right(_RIGHT_SHARE);
        disable_right(_RIGHT_DOWNLOAD);
        return;
    }

    // Enable rights

    enable_right(_RIGHT_VIEW, true);

    if (flags64_on(desired_rights, BUILTIN_RIGHT_EDIT)) {
        enable_right(_RIGHT_EDIT);
    }
    else {
        disable_right(_RIGHT_EDIT);
    }

    if (flags64_on(desired_rights, BUILTIN_RIGHT_PRINT)) {
        enable_right(_RIGHT_PRINT);
    }
    else {
        disable_right(_RIGHT_PRINT);
    }

    if (flags64_on(desired_rights, BUILTIN_RIGHT_CLIPBOARD)) {
        enable_right(_RIGHT_CLIPBOARD);
    }
    else {
        disable_right(_RIGHT_CLIPBOARD);
    }

    if (flags64_on(desired_rights, BUILTIN_RIGHT_SAVEAS)) {
        enable_right(_RIGHT_SAVEAS);
    }
    else {
        disable_right(_RIGHT_SAVEAS);
    }

    if (flags64_on(desired_rights, BUILTIN_RIGHT_DECRYPT)) {
        enable_right(_RIGHT_DECRYPT);
    }
    else {
        disable_right(_RIGHT_DECRYPT);
    }

    if (flags64_on(desired_rights, BUILTIN_RIGHT_SCREENCAP)) {
        enable_right(_RIGHT_SCREENCAP);
    }
    else {
        disable_right(_RIGHT_SCREENCAP);
    }

    if (flags64_on(desired_rights, BUILTIN_RIGHT_SEND)) {
        enable_right(_RIGHT_SEND);
    }
    else {
        disable_right(_RIGHT_SEND);
    }

    if (flags64_on(desired_rights, BUILTIN_RIGHT_CLASSIFY)) {
        enable_right(_RIGHT_CLASSIFY);
    }
    else {
        disable_right(_RIGHT_CLASSIFY);
    }

    if (flags64_on(desired_rights, BUILTIN_RIGHT_SHARE)) {
        enable_right(_RIGHT_SHARE);
    }
    else {
        disable_right(_RIGHT_SHARE);
    }

    if (flags64_on(desired_rights, BUILTIN_RIGHT_DOWNLOAD)) {
        enable_right(_RIGHT_DOWNLOAD);
    }
    else {
        disable_right(_RIGHT_DOWNLOAD);
    }
}

unsigned __int64 process_forbidden_rights::get_allowed_rights() const
{
    unsigned __int64 result = 0;

    if (_rights_counter[_RIGHT_VIEW] == 0) {

        if (_rights_counter[_RIGHT_EDIT] == 0) {
            result |= BUILTIN_RIGHT_EDIT;
        }
        if (_rights_counter[_RIGHT_PRINT] == 0) {
            result |= BUILTIN_RIGHT_PRINT;
        }
        if (_rights_counter[_RIGHT_CLIPBOARD] == 0) {
            result |= BUILTIN_RIGHT_CLIPBOARD;
        }
        if (_rights_counter[_RIGHT_SAVEAS] == 0) {
            result |= BUILTIN_RIGHT_SAVEAS;
        }
        if (_rights_counter[_RIGHT_DECRYPT] == 0) {
            result |= BUILTIN_RIGHT_DECRYPT;
        }
        if (_rights_counter[_RIGHT_SCREENCAP] == 0) {
            result |= BUILTIN_RIGHT_SCREENCAP;
        }
        if (_rights_counter[_RIGHT_SEND] == 0) {
            result |= BUILTIN_RIGHT_SEND;
        }
        if (_rights_counter[_RIGHT_CLASSIFY] == 0) {
            result |= BUILTIN_RIGHT_CLASSIFY;
        }
        if (_rights_counter[_RIGHT_SHARE] == 0) {
            result |= BUILTIN_RIGHT_SHARE;
        }
        if (_rights_counter[_RIGHT_DOWNLOAD] == 0) {
            result |= BUILTIN_RIGHT_DOWNLOAD;
        }
    }

    return result;
}

void process_forbidden_rights::clear()
{
    memset(&_rights_counter, 0, sizeof(_rights_counter));
}

process_forbidden_rights& process_forbidden_rights::operator = (const process_forbidden_rights& other)
{
    if (this != &other) {
        memcpy(_rights_counter, other.get_rights_counter(), sizeof(_rights_counter));
    }
    return *this;
}


process_record::process_record() : _process_id(0), _session_id(0xFFFFFFFF), _flags(0)
{
}

process_record::process_record(unsigned long process_id, unsigned __int64 flags) : _session_id(process_record::get_session_id_from_pid(process_id)),
    _process_id((0xFFFFFFFF == _session_id) ? 0 : process_id),
    _flags((0 == _process_id) ? 0 : flags),
    _image_path((0 == _process_id) ? std::wstring() : process_record::get_image_path_from_pid(_process_id))
{
    std::shared_ptr<NX::win::pe_file> existing_info_ptr = PE_CACHE.find(_image_path);
    if (existing_info_ptr == nullptr) {
        _pe_file_info = std::shared_ptr<NX::win::pe_file>(new NX::win::pe_file(_image_path));
        if (!_pe_file_info->empty()) {
            PE_CACHE.insert(_image_path, _pe_file_info);
        }
    }
    else {
        _pe_file_info = existing_info_ptr;
    }
}

process_record::process_record(unsigned long process_id, unsigned long session_id, const std::wstring& image_path, unsigned __int64 flags) : _session_id(session_id),
    _process_id(process_id),
    _flags((0 == _process_id) ? 0 : flags),
    _image_path(process_record::normalize_image_path(image_path))
{
    std::shared_ptr<NX::win::pe_file> existing_info_ptr = PE_CACHE.find(_image_path);
    if (existing_info_ptr == nullptr) {
        _pe_file_info = std::shared_ptr<NX::win::pe_file>(new NX::win::pe_file(_image_path));
        if (!_pe_file_info->empty()) {
            PE_CACHE.insert(_image_path, _pe_file_info);
        }
    }
    else {
        _pe_file_info = existing_info_ptr;
    }
}

process_record::~process_record()
{
    clear();
}

unsigned long process_record::get_session_id_from_pid(unsigned long process_id)
{
    unsigned long session_id = 0xFFFFFFFF;
    if (!ProcessIdToSessionId(process_id, &session_id)) {
        session_id = 0xFFFFFFFF;
    }
    return session_id;
}

std::wstring process_record::normalize_image_path(const std::wstring& image_path)
{
    NX::fs::dos_filepath result(image_path);
    if (std::wstring::npos == result.path().find(L'~')) {
        // Cannot be short path name
        return result.path();
    }

    std::wstring s;
    if (0 == GetLongPathNameW(result.path().c_str(), NX::string_buffer<wchar_t>(s, MAX_PATH), MAX_PATH)) {
        return result.path();
    }

    return std::move(s);
}

std::wstring process_record::get_image_path_from_pid(unsigned long process_id)
{
    std::wstring image_path;

    if (0 == process_id) {
        GetModuleFileNameW(NULL, NX::string_buffer<wchar_t>(image_path, MAX_PATH), MAX_PATH);
    }
    else {
        HANDLE h = ::OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, process_id);
        if (NULL != h) {
			GetModuleFileNameEx(h, NULL, NX::string_buffer<wchar_t>(image_path, MAX_PATH), MAX_PATH);
            CloseHandle(h);
        }
    }

    std::transform(image_path.begin(), image_path.end(), image_path.begin(), tolower);
    return std::move(image_path);
}

process_record& process_record::operator = (const process_record& other)
{
    if (this != &other) {
        _process_id = other.get_process_id();
        _session_id = other.get_session_id();
        _flags = other.get_flags();
        _image_path = other.get_image_path();
        _pe_file_info = other.get_pe_file_info();

        _forbidden_rights = other.get_forbidden_rights();
        _protected_windows = other.get_protected_windows();
    }
    return *this;
}

void process_record::clear()
{
    _process_id = 0;
    _session_id = 0xFFFFFFFF;
    _flags = 0;
    _image_path.clear();
    _pe_file_info.reset();
    _forbidden_rights.clear();
    _protected_windows.clear();
	_map_file_rights_watermark.clear();
}

void process_record::insert_file_rights(const std::wstring& filePath, const std::wstring& strJsonRightsWatermark)
{
	std::wstring strFile = filePath;
	strFile.erase(0, strFile.find_first_not_of(L" "));
	strFile.erase(strFile.find_last_not_of(L" ") + 1);
	std::transform(strFile.begin(), strFile.end(), strFile.begin(), ::towlower);

	auto itFind = _map_file_rights_watermark.find(strFile);
	if (_map_file_rights_watermark.end() != itFind)
		return;

	_map_file_rights_watermark.insert( std::make_pair(strFile, strJsonRightsWatermark) );
}

void process_record::remove_file_rights(const std::wstring& filePath)
{
	std::wstring strFile = filePath;
	strFile.erase(0, strFile.find_first_not_of(L" "));
	strFile.erase(strFile.find_last_not_of(L" ") + 1);
	std::transform(strFile.begin(), strFile.end(), strFile.begin(), ::towlower);

	auto itFind = _map_file_rights_watermark.find(strFile);
	if (_map_file_rights_watermark.end() != itFind)
		_map_file_rights_watermark.erase(itFind);
}

std::map<std::wstring, std::wstring> process_record::get_file_rights()
{
	std::map<std::wstring, std::wstring> mapFileRights;
	for (auto item : _map_file_rights_watermark)
	{
		mapFileRights.insert( std::make_pair(item.first, item.second));
	}

	return std::move(mapFileRights);
}

process_cache::process_cache()
{
}

process_cache::~process_cache()
{
    clear();
}

bool process_cache::empty() const
{
    bool result = false;
    NX::utility::CRwSharedLocker locker(&_lock);
    result = _map.empty();
    return result;
}

void process_cache::clear()
{
    NX::utility::CRwExclusiveLocker locker(&_lock);
    _map.clear();
}

process_record process_cache::find(unsigned long process_id)
{
    process_record record;
    NX::utility::CRwSharedLocker locker(&_lock);
    auto pos = _map.find(process_id);
    if (pos != _map.end()) {
        record = (*pos).second;
    }
    return record;
}

void process_cache::insert(const process_record& record)
{
    NX::utility::CRwExclusiveLocker locker(&_lock);
    _map[record.get_process_id()] = record;
}

void process_cache::remove(unsigned long process_id)
{
    NX::utility::CRwExclusiveLocker locker(&_lock);
    _map.erase(process_id);
}

void process_cache::reset_process_flags(unsigned long process_id, unsigned __int64 f)
{
    NX::utility::CRwExclusiveLocker locker(&_lock);
    auto pos = _map.find(process_id);
    if (pos != _map.end()) {
        (*pos).second.reset_flags(f);
    }
}

void process_cache::set_process_flags(unsigned long process_id, unsigned __int64 f)
{
    NX::utility::CRwExclusiveLocker locker(&_lock);
    auto pos = _map.find(process_id);
    if (pos != _map.end()) {
        (*pos).second.set_flags(f);
    }
}

void process_cache::clear_process_flags(unsigned long process_id, unsigned __int64 f)
{
    NX::utility::CRwExclusiveLocker locker(&_lock);
    auto pos = _map.find(process_id);
    if (pos != _map.end()) {
        (*pos).second.clear_flags(f);
    }
}

void process_cache::forbid_process_right(unsigned long process_id, RIGHT_ID id)
{
    NX::utility::CRwExclusiveLocker locker(&_lock);
    auto pos = _map.find(process_id);
    if (pos != _map.end()) {
        (*pos).second.get_forbidden_rights().disable_right(id);
    }
}

void process_cache::enable_process_right(unsigned long process_id, RIGHT_ID id, bool force)
{
    NX::utility::CRwExclusiveLocker locker(&_lock);
    auto pos = _map.find(process_id);
    if (pos != _map.end()) {
        (*pos).second.get_forbidden_rights().enable_right(id, force);
    }
}

void process_cache::set_process_rights(unsigned long process_id, unsigned __int64 desired_rights)
{
    NX::utility::CRwExclusiveLocker locker(&_lock);
    auto pos = _map.find(process_id);
    if (pos != _map.end()) {
        (*pos).second.get_forbidden_rights().set_rights(desired_rights);
    }
}

void process_cache::insert_protected_window(unsigned long process_id, unsigned long hwnd, PROTECT_MODE mode)
{
    NX::utility::CRwExclusiveLocker locker(&_lock);
    auto pos = _map.find(process_id);
    if (pos != _map.end()) {
        auto wndpos = (*pos).second.get_protected_windows().find(hwnd);
        if (wndpos != (*pos).second.get_protected_windows().end()) {
            if (pm_yes != (*wndpos).second.get_mode()) {
                (*wndpos).second.change_mode(mode);
            }
        }
        else {
            (*pos).second.get_protected_windows()[hwnd] = process_protected_window(hwnd, mode);
        }
    }
}

void process_cache::change_protected_window_mode(unsigned long process_id, unsigned long hwnd, PROTECT_MODE mode)
{
    NX::utility::CRwExclusiveLocker locker(&_lock);
    auto pos = _map.find(process_id);
    if (pos != _map.end()) {
        (*pos).second.get_protected_windows()[hwnd].change_mode(mode);
    }
}

void process_cache::remove_protected_window(unsigned long process_id, unsigned long hwnd)
{
    NX::utility::CRwExclusiveLocker locker(&_lock);
    auto pos = _map.find(process_id);
    if (pos != _map.end()) {
        (*pos).second.get_protected_windows().erase(hwnd);
    }
}

void process_cache::clear_protected_windows(unsigned long process_id)
{
    NX::utility::CRwExclusiveLocker locker(&_lock);
    if (0 == process_id) {
        std::for_each(_map.begin(), _map.end(), [](std::pair<unsigned long, process_record> item) {
            item.second.get_protected_windows().clear();
        });
    }
    else {
        auto pos = _map.find(process_id);
        if (pos != _map.end()) {
            (*pos).second.get_protected_windows().clear();
        }
    }
}

std::vector<process_protected_window> process_cache::get_protected_windows(unsigned long process_id)
{
    std::vector<process_protected_window> protected_windows;

    NX::utility::CRwSharedLocker locker(&_lock);
    if (0 == process_id) {
        std::for_each(_map.begin(), _map.end(), [&](const std::pair<unsigned long, process_record>& item) {
            const unsigned __int64 flags = item.second.get_flags();
            const bool is_process_with_nxl_opened = (NXRM_PROCESS_FLAG_WITH_NXL_OPENED == (flags & NXRM_PROCESS_FLAG_WITH_NXL_OPENED));
            const bool is_process_with_overlay_integrated = (NXRM_PROCESS_FLAG_HAS_OVERLAY_INTEGRATION == (flags & NXRM_PROCESS_FLAG_HAS_OVERLAY_INTEGRATION));
            const bool is_process_with_overlay_obligation = (NXRM_PROCESS_FLAG_WITH_OVERLAY_OBLIGATION == (flags & NXRM_PROCESS_FLAG_WITH_OVERLAY_OBLIGATION));
            std::for_each(item.second.get_protected_windows().begin(), item.second.get_protected_windows().end(), [&](const std::pair<unsigned long, process_protected_window>& windows_item) {
                if (is_process_with_overlay_integrated) {
                    if (windows_item.second.get_mode() == pm_yes) {
                        protected_windows.push_back(windows_item.second);
                    }
                }
                else {
                    if (is_process_with_overlay_obligation && windows_item.second.get_mode() != pm_no) {
                        protected_windows.push_back(windows_item.second);
                    }
                }
            });
        });
    }
    else {
        auto pos = _map.find(process_id);
        if (pos != _map.end()) {
            const unsigned __int64 flags = (*pos).second.get_flags();
            const bool is_process_with_nxl_opened = (NXRM_PROCESS_FLAG_WITH_NXL_OPENED == (flags & NXRM_PROCESS_FLAG_WITH_NXL_OPENED));
            const bool is_process_with_overlay_integrated = (NXRM_PROCESS_FLAG_HAS_OVERLAY_INTEGRATION == (flags & NXRM_PROCESS_FLAG_HAS_OVERLAY_INTEGRATION));
            std::for_each((*pos).second.get_protected_windows().begin(), (*pos).second.get_protected_windows().end(), [&](const std::pair<unsigned long, process_protected_window>& windows_item) {
                if (is_process_with_overlay_integrated) {
                    if (windows_item.second.get_mode() == pm_yes) {
                        protected_windows.push_back(windows_item.second);
                    }
                }
                else {
                    if (is_process_with_nxl_opened && windows_item.second.get_mode() != pm_no) {
                        protected_windows.push_back(windows_item.second);
                    }
                }
            });
        }
    }

    return protected_windows;
}

std::vector<std::pair<unsigned long, std::wstring>> process_cache::find_all_protected_process(unsigned long session_id)
{
    std::vector<std::pair<unsigned long, std::wstring>> protected_processes;
    NX::utility::CRwSharedLocker locker(&_lock);
    std::for_each(_map.begin(), _map.end(), [&](const std::pair<unsigned long, process_record>& item) {
        const bool session_matched = ((unsigned long)-1 == session_id) ? true : (session_id == item.second.get_session_id());
        if (session_matched && NXRM_PROCESS_FLAG_WITH_NXL_OPENED == (item.second.get_flags() & NXRM_PROCESS_FLAG_WITH_NXL_OPENED)) {
            protected_processes.push_back(std::pair<unsigned long, std::wstring>(item.first, item.second.get_image_path()));
        }
    });
    return std::move(protected_processes);
}

bool process_cache::does_protected_process_exist(unsigned long session_id)
{
    NX::utility::CRwSharedLocker locker(&_lock);
    return (_map.end() != std::find_if(_map.begin(), _map.end(), [&](const std::pair<unsigned long, process_record>& item) -> bool {
        const bool session_matched = ((unsigned long)-1 == session_id) ? true : (session_id == item.second.get_session_id());
        return (session_matched && NXRM_PROCESS_FLAG_WITH_NXL_OPENED == (item.second.get_flags() & NXRM_PROCESS_FLAG_WITH_NXL_OPENED));
    }));
}

void process_cache::insert_process_file_rights(unsigned long process_id, const std::wstring& file_path, const std::wstring& strJsonFileRightsWatermark)
{
	NX::utility::CRwSharedLocker locker(&_lock);
	auto itFind = _map.find(process_id);
	if (_map.end() == itFind)
		return;

	itFind->second.insert_file_rights(file_path, strJsonFileRightsWatermark);

	// special code for adobe reader because of its protected mode
	auto process_path = NX::win::get_process_path(process_id);
	std::transform(process_path.begin(), process_path.end(), process_path.begin(), ::tolower);
	if (process_path.find(L"acrord32.exe") != process_path.npos)
	{

		DWORD dwParentProcessID = NX::win::get_parent_processid(process_id);
		auto parentPath = NX::win::get_process_path(dwParentProcessID);
		if (parentPath.empty())
			return;

		std::transform(parentPath.begin(), parentPath.end(), parentPath.begin(), ::tolower);
		if (parentPath.find(L"acrord32.exe") != parentPath.npos)
		{
			auto itparent = _map.find(dwParentProcessID);
			if (_map.end() == itparent)
				return;

			// its parent process is still adobe reader and trusted
			itparent->second.insert_file_rights(file_path, strJsonFileRightsWatermark);
		}
	}
}

void process_cache::remove_process_file_rights(unsigned long process_id, const std::wstring& file_path)
{
	NX::utility::CRwSharedLocker locker(&_lock);
	auto itFind = _map.find(process_id);
	if (_map.end() == itFind)
		return;

	itFind->second.remove_file_rights(file_path);
}

bool process_cache::get_process_file_rights(unsigned long process_id, std::map<std::wstring, std::wstring>& mapFileRights)
{
	NX::utility::CRwSharedLocker locker(&_lock);
	auto itFind = _map.find(process_id);
	if (_map.end() == itFind)
		return false;

	mapFileRights.clear();
	std::map<std::wstring, std::wstring>&mapProcessFileRights = itFind->second.get_file_rights();
	for (auto item : mapProcessFileRights)
	{
		mapFileRights.insert( std::make_pair(item.first, item.second) );
	}
	return true;
}