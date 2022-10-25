

#include <winsock2.h>
#include <Windows.h>
#include <assert.h>
#include <Shellapi.h>
#include <iphlpapi.h>
#include <Ws2tcpip.h>
#include <Wtsapi32.h>
#define SECURITY_WIN32
#include <security.h>
#include <Sddl.h>
#include <Shlobj.h>
#include <Shlwapi.h>

#include <versionhelpers.h>

#include <set>
#include <boost/algorithm/string.hpp>

#include <nudf\eh.hpp>
#include <nudf\debug.hpp>
#include <nudf\string.hpp>
#include <nudf\filesys.hpp>
#include <nudf\crypto.hpp>

#include <nudf\winutil.hpp>

#include <intrin.h>
#include <winternl.h>
#include <Psapi.h>


using namespace NX;
using namespace NX::win;


std::wstring NX::win::get_windows_directory()
{
    static std::wstring dir;
    if (dir.empty()) {
        if (0 == GetWindowsDirectoryW(NX::string_buffer<wchar_t>(dir, MAX_PATH), MAX_PATH)) {
            dir.clear();
        }
    }
    return dir;
}

std::wstring NX::win::get_windows_temp_directory()
{
    static std::wstring dir;
    if (dir.empty()) {
        dir = NX::win::get_windows_directory();
        if (!dir.empty()) {
            if (!boost::algorithm::ends_with(dir, L"\\")) {
                dir += L"\\";
            }
            dir += L"Temp";
        }
    }
    return dir;
}

std::wstring NX::win::get_system_directory()
{
    static std::wstring dir;
    if (dir.empty()) {
        if (0 == GetSystemDirectoryW(NX::string_buffer<wchar_t>(dir, MAX_PATH), MAX_PATH)) {
            dir.clear();
        }
    }
    return dir;
}

unsigned long NX::win::get_parent_processid(unsigned long processId)
{
	NTSTATUS	status;
	DWORD		dwParentPID = 0;
	DWORD		dwErr = 0;
	HANDLE		hProcess = NULL;
	PROCESS_BASIC_INFORMATION	pbi;

	typedef LONG(WINAPI *PNTQUERYINFORMATIONPROCESS)(HANDLE, UINT, PVOID, ULONG, PULONG);
	static PNTQUERYINFORMATIONPROCESS  NtQueryInformationProcess = (PNTQUERYINFORMATIONPROCESS)GetProcAddress(GetModuleHandleA("ntdll.dll"), "NtQueryInformationProcess");

	do
	{
		hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, processId); //PROCESS_QUERY_INFORMATION
		if (!hProcess)
		{
			dwErr = ::GetLastError();
			break;
		}

		status = NtQueryInformationProcess(hProcess, SystemBasicInformation, (PVOID)&pbi, sizeof(PROCESS_BASIC_INFORMATION), NULL);
		if (NT_SUCCESS(status))
		{
			dwParentPID = (DWORD)((ULONG_PTR)pbi.Reserved3);
		}
		else
		{
			dwErr = ::GetLastError();
		}

	} while (FALSE);

	::CloseHandle(hProcess);
	return dwParentPID;
}

std::wstring NX::win::get_process_path(unsigned long processId)
{
	DWORD		dwErr = 0;
	HANDLE		hProcess = NULL;
	std::wstring	strAppImagePath;

	do
	{
	    hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, processId);
		if (hProcess == NULL) {
			dwErr = ::GetLastError();

			std::wstring log = std::wstring(L"OpenProcess dwErr is:") + std::to_wstring(dwErr);
			OutputDebugStringW(log.c_str());

			break;
		}

		WCHAR nameBuf[MAX_PATH];
		DWORD dwSize _countof(nameBuf);
		DWORD ret;

		// Discard using GetModuleFileNameExW, which will failed in some machine or scene.
		ret = QueryFullProcessImageName(hProcess, 0, nameBuf, &dwSize);

		if (ret == 0) {

			DWORD dwExitCode = 0;
			BOOL bRet = ::GetExitCodeProcess(hProcess, &dwExitCode);
			dwErr = ::GetLastError();

			std::wstring log = std::wstring(L"QueryFullProcessImageName dwErr is:") + std::to_wstring(dwErr);
			log += L" , GetExitCodeProcess ret = " + std::to_wstring(bRet) + L" exitcode = " + std::to_wstring(dwExitCode) + L"\n";
			OutputDebugStringW(log.c_str());

			break;
		}

		strAppImagePath = std::wstring(nameBuf);

	} while (FALSE);

	::CloseHandle(hProcess);

	return strAppImagePath;
}

std::wstring NX::win::get_current_process_path(void)
{
	WCHAR pathBuf[MAX_PATH];
	DWORD ret;
	ret = GetModuleFileName(NULL, pathBuf, _countof(pathBuf));
	if (ret > 0 && ret < _countof(pathBuf))
	{
		return pathBuf;
	}
	else
	{
		// Buffer too small or other error.
		return L"";
	}
}

void NX::win::get_current_process_info(std::wstring& strBaseName, std::wstring& strPath, DWORD& dwProcessId, BOOL& bRemoteSession)
{
	HANDLE phandle = GetCurrentProcess();
	WCHAR _basename[MAX_PATH];
	GetModuleBaseName(phandle, NULL, _basename, MAX_PATH);
	strBaseName = std::wstring(_basename);

	dwProcessId = GetCurrentProcessId();
	strPath = get_current_process_path();

	bRemoteSession = is_remote_session();
}


BOOL NX::win::is_remote_session()
{
	BOOL fIsRemoteable = FALSE;

	if (GetSystemMetrics(SM_REMOTESESSION))
	{
		fIsRemoteable = TRUE;
	}
	else
	{
		HKEY hRegKey = NULL;
		LONG lResult;

		lResult = RegOpenKeyEx(
			HKEY_LOCAL_MACHINE,
			L"SYSTEM\\CurrentControlSet\\Control\\Terminal Server\\",
			0, // ulOptions
			KEY_READ,
			&hRegKey
		);

		if (lResult == ERROR_SUCCESS)
		{
			DWORD dwGlassSessionId;
			DWORD cbGlassSessionId = sizeof(dwGlassSessionId);
			DWORD dwType;

			lResult = RegQueryValueEx(
				hRegKey,
				L"GlassSessionId",
				NULL, // lpReserved
				&dwType,
				(BYTE*)&dwGlassSessionId,
				&cbGlassSessionId
			);

			if (lResult == ERROR_SUCCESS)
			{
				DWORD dwCurrentSessionId;

				if (ProcessIdToSessionId(GetCurrentProcessId(), &dwCurrentSessionId))
				{
					fIsRemoteable = (dwCurrentSessionId != dwGlassSessionId);
				}
			}
		}

		if (hRegKey)
		{
			RegCloseKey(hRegKey);
		}
	}

	return fIsRemoteable;
}

windows_info::windows_info() : _is_server(IsWindowsServer()), _windows_version(get_windows_version()), _platform_id(get_platform_id()), _cpu_arch(get_cpu_arch()), _build_number((WINBUILDNUM) get_build_number()), _windows_name(get_windows_name()), _edition_name(get_edition_name()), _windows_dir(get_windows_dir()), _system_windows_dir(get_system_windows_dir()), _program_files_dir(get_program_files_dir()), _program_files_x86_dir(get_program_files_x86_dir())
{
}

windows_info::~windows_info()
{
}

unsigned long windows_info::get_cpu_arch()
{
    SYSTEM_INFO         sysinf;

    memset(&sysinf, 0, sizeof(SYSTEM_INFO));
    GetNativeSystemInfo(&sysinf);
    return sysinf.wProcessorArchitecture;
}

std::wstring windows_info::get_windows_version()
{
    std::wstring version;

    try {

        reg_local_machine rlm;
        rlm.open(L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", reg_key::reg_wow64_64, true);
        rlm.read_value(L"CurrentVersion", version);
        rlm.close();

        if (version == L"6.3") {
            if (IsWindows10OrGreater()) {
                version = L"10.0";
            }
        }
    }
    catch (const NX::exception& e) {
        UNREFERENCED_PARAMETER(e);
        version.clear();
    }

    return std::move(version);
}

unsigned long windows_info::get_platform_id()
{
    unsigned long id = PLATFORM_WINDOWS;
    const bool is_server = IsWindowsServer();

    try {

        std::wstring winver_str;
        reg_local_machine rlm;
        rlm.open(L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", reg_key::reg_wow64_64, true);
        rlm.read_value(L"CurrentVersion", winver_str);
        rlm.close();

        if (winver_str == L"5.0") {
            id = PLATFORM_WIN2000;
        }
        else if (winver_str == L"5.1") {
            id = PLATFORM_WINXP;
        }
        else if (winver_str == L"5.2") {
            if (is_server) {
                id = (0 == GetSystemMetrics(SM_SERVERR2)) ? PLATFORM_WINDOWS_SERVER_2003 : PLATFORM_WINDOWS_SERVER_2003_R2;
            }
            else {
                id = PLATFORM_WINXP;
            }
        }
        else if (winver_str == L"6.0") {
            id = is_server ? PLATFORM_WINDOWS_SERVER_2008 : PLATFORM_WINVISTA;
        }
        else if (winver_str == L"6.1") {
            id = is_server ? PLATFORM_WINDOWS_SERVER_2008_R2 : PLATFORM_WIN7;
        }
        else if (winver_str == L"6.2") {
            id = is_server ? PLATFORM_WINDOWS_SERVER_2012 : PLATFORM_WIN8;
        }
        else if (winver_str == L"6.3") {
            if (IsWindows10OrGreater()) {
                id = is_server ? PLATFORM_WINDOWS_SERVER_2016 : PLATFORM_WIN10;
            }
            else {
                id = is_server ? PLATFORM_WINDOWS_SERVER_2012_R2 : PLATFORM_WIN8_1;
            }
        }
        else {
            id = PLATFORM_WINDOWS;
        }
    }
    catch (const NX::exception& e) {
        UNREFERENCED_PARAMETER(e);
        id = PLATFORM_WINDOWS;
    }

	id = PLATFORM_WINDOWS;
    return id;
}

unsigned long windows_info::get_build_number()
{
    unsigned long build_number = 0;
    try {
        reg_local_machine rlm;
        std::wstring    str_build_number;
        rlm.open(L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", reg_key::reg_wow64_64, true);
        rlm.read_value(L"CurrentBuildNumber", str_build_number);
        rlm.close();
        build_number = std::stoul(str_build_number);
    }
    catch (const NX::exception& e) {
        UNREFERENCED_PARAMETER(e);
        build_number = 0;
    }
    return build_number;
}

std::wstring windows_info::get_windows_name()
{
    std::wstring    winproduct_name;
    try {
        reg_local_machine rlm;
        rlm.open(L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", reg_key::reg_wow64_64, true);
        rlm.read_value(L"ProductName", winproduct_name);
        rlm.close();
    }
    catch (const NX::exception& e) {
        UNREFERENCED_PARAMETER(e);
        winproduct_name.clear();
    }
    return std::move(winproduct_name);
}

std::wstring windows_info::get_edition_name()
{
    std::wstring    edition_name;
    try {
        reg_local_machine rlm;
        rlm.open(L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", reg_key::reg_wow64_64, true);
        rlm.read_value(L"EditionID", edition_name);
        rlm.close();
    }
    catch (const NX::exception& e) {
        UNREFERENCED_PARAMETER(e);
        edition_name.clear();
    }
    return std::move(edition_name);
}

std::wstring windows_info::get_windows_dir()
{
    wchar_t buf[MAX_PATH + 1] = { 0 };
    GetWindowsDirectoryW(buf, MAX_PATH);
    return buf;
}

std::wstring windows_info::get_system_windows_dir()
{
    wchar_t buf[MAX_PATH + 1] = { 0 };
    GetSystemWindowsDirectoryW(buf, MAX_PATH);
    return buf;
}

std::wstring windows_info::get_program_files_dir()
{
    PWSTR pszPath;
    std::wstring ret;
    if (SHGetKnownFolderPath(FOLDERID_ProgramFiles, 0, NULL, &pszPath) == S_OK) {
        ret = pszPath;
        CoTaskMemFree(pszPath);
    }
    return ret;
}

std::wstring windows_info::get_program_files_x86_dir()
{
    PWSTR pszPath;
    std::wstring ret;
    if (SHGetKnownFolderPath(FOLDERID_ProgramFilesX86, 0, NULL, &pszPath) == S_OK) {
        ret = pszPath;
        CoTaskMemFree(pszPath);
    }
    return ret;
}


language::language()
{
}

language::language(unsigned short lgid)
{
    set_lgid(lgid);
}

language::language(LCID lcid)
{
    set_lcid(lcid);
}

language::~language()
{
}

language& language::operator = (const language& other)
{
    if (this != &other) {
        _lcid = other.id();
        _name = other.name();
    }
    return *this;
}

void language::set_lcid(LCID lcid)
{
    clear();
    if (0 != LCIDToLocaleName(lcid, NX::string_buffer<wchar_t>(_name, LOCALE_NAME_MAX_LENGTH), LOCALE_NAME_MAX_LENGTH, 0)) {
        _lcid = LOWORD(lcid);
    }
}

void language::set_lgid(unsigned short lgid)
{
    set_lcid(MAKELCID(lgid, SORT_DEFAULT));
}

system_default_language::system_default_language() : language(GetSystemDefaultLCID())
{
}

system_default_language::~system_default_language()
{
}

user_default_language::user_default_language() : language(GetUserDefaultLCID())
{
}

user_default_language::~user_default_language()
{
}


installation::software::software()
{
}

installation::software::software(const std::wstring& n, bool is_64bit, std::wstring ver, std::wstring pub, unsigned short lgid, const std::wstring& date, const std::wstring& dir)
    : _name(n), _x64(is_64bit), _version(ver), _publisher(pub), _lang(lgid), _install_date(date), _install_dir(dir)
{
}

installation::software::~software()
{
}

installation::software& installation::software::operator = (const installation::software& other)
{
    if (this != &other) {
        _x64 = other.x64();
        _name = other.name();
        _version = other.version();
        _publisher = other.publisher();
        _lang = other.lang();
        _install_date = other.install_date();
        _install_dir = other.install_dir();
    }
    return *this;
}

static void inter_get_installed_software(std::vector<installation::software>& items, bool x64)
{
    // HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall
    try {

        reg_key rk;
        reg_key::reg_position rpos = x64 ? reg_key::reg_wow64_64 : reg_key::reg_wow64_32;
        rk.open(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall", rpos, true);
        rk.enum_sub_keys([&](const wchar_t* name) {
            try {
                reg_key rk_software;
                rk_software.open(rk, name, rpos, true);

                std::wstring software_name;
                std::wstring software_version;
                std::wstring software_publisher;
                unsigned long software_lang_id;
                std::wstring software_install_date;
                std::wstring software_install_dir;

                rk_software.read_value(L"DisplayName", software_name);
                rk_software.read_value(L"DisplayVersion", software_version);
                rk_software.read_value(L"Publisher", software_publisher);
                rk_software.read_value(L"Language", &software_lang_id);
                rk_software.read_value(L"InstallDate", software_install_date);
                rk_software.read_value(L"InstallLocation", software_install_dir);

                items.push_back(installation::software(software_name, x64, software_version, software_publisher, (unsigned short)software_lang_id, software_install_date, software_install_dir));
            }
            catch (const std::exception& e) {
                UNREFERENCED_PARAMETER(e);
            }
        });
    }
    catch (const std::exception& e) {
        UNREFERENCED_PARAMETER(e);
    }
}

std::vector<installation::software> installation::get_installed_software(unsigned long flags)
{
    std::vector<installation::software> installed_software;
    if (flags & INSTALLED_SOFTWARE_64BIT) {
        inter_get_installed_software(installed_software, true);     // 64 bit
    }
    if (flags & INSTALLED_SOFTWARE_32BIT) {
        inter_get_installed_software(installed_software, false);    // 32 bit
    }
    return installed_software;
}

std::vector<std::wstring> installation::get_installed_kbs()
{
    std::vector<std::wstring> kbs;
    std::set<std::wstring> kbs_set;

    try {

        reg_key rk;
        rk.open(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Component Based Servicing\\Packages", reg_key::reg_wow64_64, true);
        rk.enum_sub_keys([&](const wchar_t* name) {

            if (name == NULL || name[0] == 0) {
                return;
            }

            std::wstring sub_key_name(name);
            // Package_1_for_KB2894852~31bf3856ad364e35~amd64~~6.3.2.0
            if (!boost::algorithm::istarts_with(sub_key_name, L"Package_")) {
                return;
            }

            std::transform(sub_key_name.begin(), sub_key_name.end(), sub_key_name.begin(), toupper);
            auto pos = sub_key_name.find(L"_FOR_KB");
            if (pos == std::wstring::npos) {
                return;
            }

            // Get KB start: KB2894852~31bf3856ad364e35~amd64~~6.3.2.0
            sub_key_name = sub_key_name.substr(pos + 5);
            pos = sub_key_name.find_first_of(L"~_- .");
            if (pos != std::wstring::npos) {
                sub_key_name = sub_key_name.substr(0, pos); // KB2894852
            }

            // add KB name
            kbs_set.insert(sub_key_name);
        });

        std::for_each(kbs_set.begin(), kbs_set.end(), [&](const std::wstring& name) {
            kbs.push_back(name);
        });
    }
    catch (const std::exception& e) {
        UNREFERENCED_PARAMETER(e);
    }

    return std::move(kbs);
}





//
//  class file_version::
//

file_version::file_version() : _product_version({ 0, 0 }), _file_version({ 0, 0 }), _file_time({ 0, 0 }), _file_flags(0), _file_os(VOS_UNKNOWN), _file_type(0), _file_subtype(0)
{
}

file_version::file_version(const std::wstring& file) : _product_version({ 0, 0 }), _file_version({ 0, 0 }), _file_time({ 0, 0 }), _file_flags(0), _file_os(VOS_UNKNOWN), _file_type(0), _file_subtype(0)
{
    if (file.empty()) {
        NX::fs::module_path mod_path(NULL);
        load(mod_path.path());
    }
    else {
        load(file);
    }
}

file_version::~file_version()
{
}

void file_version::clear()
{
    _company_name.clear();
    _product_name.clear();
    _product_version_string.clear();
    _file_name.clear();
    _file_description.clear();
    _file_version_string.clear();
    _product_version = { 0, 0 };
    _file_version = { 0, 0 };
    _file_time = { 0, 0 };
    _file_flags = 0;
    _file_os = 0;
    _file_type = 0;
    _file_subtype = 0;
}

file_version& file_version::operator = (const file_version& other)
{
    if (this != &other) {
        _company_name = other.get_company_name();
        _product_name = other.get_product_name();
        _product_version_string = other.get_product_version_string();
        _file_name = other.get_file_name();
        _file_description = other.get_file_description();
        _file_version_string = other.get_file_version_string();
        _product_version = other.get_product_version();
        _file_time = other.get_file_time();
        _file_version = other.get_file_version();
        _file_flags = other.get_file_flags();
        _file_os = other.get_file_os();
        _file_type = other.get_file_type();
        _file_subtype = other.get_file_subtype();
    }
    return *this;
}

bool file_version::operator == (const file_version& other) const
{
    return (_file_time.dwHighDateTime == other.get_file_time().dwHighDateTime
        && _file_time.dwLowDateTime == other.get_file_time().dwLowDateTime
        && _product_version.QuadPart == other.get_product_version().QuadPart
        && _file_version.QuadPart == other.get_file_version().QuadPart
        && _file_flags == other.get_file_flags()
        && _file_os == other.get_file_os()
        && _file_type == other.get_file_type()
        && _file_subtype == other.get_file_subtype()
        && _product_name == other.get_product_name()
        && _company_name == other.get_company_name()
        && _file_name == other.get_file_name()
        );
}

void file_version::load(const std::wstring& file)
{
    typedef struct LANGANDCODEPAGE {
        WORD wLanguage;
        WORD wCodePage;
    } LANGANDCODEPAGE, *LPLANGANDCODEPAGE;


    try {

        const unsigned long version_info_size = GetFileVersionInfoSizeW(file.c_str(), NULL);
        if (0 == version_info_size) {
            throw std::exception("fail to get version size");
        }

        std::vector<unsigned char> info;
        info.resize(version_info_size, 0);
        if (!GetFileVersionInfoW(file.c_str(), 0, version_info_size, info.data())) {
            throw std::exception("fail to get version data");
        }

        VS_FIXEDFILEINFO* fixed_info = NULL;
        unsigned int data_size = 0;

        if (!VerQueryValueW(info.data(), L"\\", (LPVOID*)&fixed_info, &data_size)) {
            throw std::exception("fail to get fixed file info");
        }
        _product_version.HighPart = fixed_info->dwProductVersionMS;
        _product_version.LowPart = fixed_info->dwProductVersionLS;
        _file_version.HighPart = fixed_info->dwFileVersionMS;
        _file_version.LowPart = fixed_info->dwFileVersionLS;
        _file_time.dwHighDateTime = fixed_info->dwFileDateMS;
        _file_time.dwLowDateTime = fixed_info->dwFileDateLS;
        _file_flags = fixed_info->dwFileFlags;
        _file_os = fixed_info->dwFileOS;
        _file_type = fixed_info->dwFileType;
        _file_subtype = fixed_info->dwFileSubtype;

        // get language & codepage
        LPLANGANDCODEPAGE Translate;
        if (!VerQueryValueW(info.data(), L"\\VarFileInfo\\Translation", (LPVOID*)&Translate, &data_size)) {
            throw std::exception("fail to get translate info");
        }
        if (NULL == Translate || data_size < sizeof(LANGANDCODEPAGE)) {
            throw std::exception("empty translate info");
        }

        // now get all the string information in default language
        //    -> CompanyName
        _company_name = load_string(info.data(), L"CompanyName", Translate[0].wLanguage, Translate[0].wCodePage);
        //    -> ProductName
        _product_name = load_string(info.data(), L"ProductName", Translate[0].wLanguage, Translate[0].wCodePage);
        //    -> OriginalFilename
        _file_name = load_string(info.data(), L"OriginalFilename", Translate[0].wLanguage, Translate[0].wCodePage);
        //    -> FileDescription
        _file_description = load_string(info.data(), L"FileDescription", Translate[0].wLanguage, Translate[0].wCodePage);
        //    -> ProductVersion
        _product_version_string = load_string(info.data(), L"ProductVersion", Translate[0].wLanguage, Translate[0].wCodePage);
        //    -> FileVersion
        _file_version_string = load_string(info.data(), L"FileVersion", Translate[0].wLanguage, Translate[0].wCodePage);
    }
    catch (const std::exception& e) {
        UNREFERENCED_PARAMETER(e);
        clear();
    }
}

std::wstring file_version::load_string(void* data, const std::wstring& name, unsigned short language, unsigned short codepage)
{
    unsigned int cch = 0;
    wchar_t* str = NULL;
    const std::wstring resource_name = NX::string_formater(L"\\StringFileInfo\\%04x%04x\\%s", language, codepage, name.c_str());
    if (!VerQueryValueW(data, resource_name.c_str(), (LPVOID*)&str, &cch)) {
        return std::wstring();
    }
    if (0 == cch || NULL == str) {
        return std::wstring();
    }
    std::wstring s = str;
    return std::move(s);
}


//
//
//

NX::win::hardware::processor_information::processor_information() : _hyperthreads(false), _cores(0), _logical_processors(0)
{
    load();
}

NX::win::hardware::processor_information::~processor_information()
{
}

void NX::win::hardware::processor_information::load()
{
    cpu_id_data cpui;

    // Calling __cpuid with 0x0 as the function_id argument
    // gets the number of the highest valid function ID.
    __cpuid(cpui.data(), 0);
    const int cpu_id_count = cpui[0];

    for (int i = 0; i <= cpu_id_count; ++i) {
        __cpuidex(cpui.data(), i, 0);
        _data.push_back(cpui);
    }

    // Capture vendor string
    char vendor_name[0x20];
    memset(vendor_name, 0, sizeof(vendor_name));
    *reinterpret_cast<int*>(vendor_name) = _data[0][1];
    *reinterpret_cast<int*>(vendor_name + 4) = _data[0][3];
    *reinterpret_cast<int*>(vendor_name + 8) = _data[0][2];
    if (0 == _stricmp(vendor_name, "GenuineIntel")) {
        _vendor = CV_INTEL;
    }
    else if (0 == _stricmp(vendor_name, "AuthenticAMD")) {
        _vendor = CV_AMD;
    }
    else {
        _vendor = CV_UNKNOWN;
    }

    // logical processors
    _logical_processors = (_data[1][1] >> 16) & 0xFF;
    _cores = _logical_processors;
    const unsigned long cpu_features = _data[1][3];


    // load bitset with flags for function 0x00000001
    if (cpu_id_count >= 1) {
        f_1_ECX_ = _data[1][2];
        f_1_EDX_ = _data[1][3];
    }

    // load bitset with flags for function 0x00000007
    if (cpu_id_count >= 7) {
        f_7_EBX_ = _data[7][1];
        f_7_ECX_ = _data[7][2];
    }

    // Calling __cpuid with 0x80000000 as the function_id argument
    // gets the number of the highest valid extended ID.
    __cpuid(cpui.data(), 0x80000000);
    const int cpu_extra_id_count = cpui[0];

    char brand_name[0x40];
    memset(brand_name, 0, sizeof(brand_name));

    for (int i = 0x80000000; i <= cpu_extra_id_count; ++i) {
        __cpuidex(cpui.data(), i, 0);
        _extdata.push_back(cpui);
    }

    // load bitset with flags for function 0x80000001
    if (cpu_extra_id_count >= 0x80000001) {
        f_81_ECX_ = _extdata[1][2];
        f_81_EDX_ = _extdata[1][3];
    }

    // Interpret CPU brand string if reported
    if (cpu_extra_id_count >= 0x80000004) {
        memcpy(brand_name, _extdata[2].data(), sizeof(cpui));
        memcpy(brand_name + 16, _extdata[3].data(), sizeof(cpui));
        memcpy(brand_name + 32, _extdata[4].data(), sizeof(cpui));
        _brand = NX::conversion::utf8_to_utf16(brand_name);
    }

    // Get cores        
    if (is_vender_intel()) {
        _cores = ((_data[4][0] >> 26) & 0x3f) + 1;
    }
    else if (is_vender_amd()) {
        _cores = ((unsigned)(_extdata[8][2] & 0xff)) + 1;
    }

    // is hyperthreads enabled
    _hyperthreads = ((cpu_features & (1 << 28)) && (_cores < _logical_processors));
}


//
//
//

NX::win::hardware::memory_information::memory_information()
{
    memset(&_status, 0, sizeof(_status));
    _status.dwLength = sizeof(_status);
    GlobalMemoryStatusEx(&_status);
}

NX::win::hardware::memory_information::~memory_information()
{
}


//
//
//

NX::win::hardware::network_adapter_information::network_adapter_information() : _if_type(0), _oper_status(0), _ipv4_enabled(false), _ipv6_enabled(false)
{
}

NX::win::hardware::network_adapter_information::network_adapter_information(const std::wstring& adapter_name,
    const std::wstring& friendly_name,
    const std::wstring& description,
    const std::wstring& physical_address,
    unsigned long if_type,
    unsigned long oper_status,
    bool ipv4_enabled,
    bool ipv6_enabled,
    const std::vector<std::wstring>& ipv4_addresses,
    const std::vector<std::wstring>& ipv6_addresses
    ) : _adapter_name(adapter_name),
    _friendly_name(friendly_name),
    _description(description),
    _physical_address(physical_address),
    _if_type(if_type),
    _oper_status(oper_status),
    _ipv4_enabled(ipv4_enabled),
    _ipv6_enabled(ipv6_enabled),
    _ipv4_addresses(ipv4_addresses),
    _ipv6_addresses(ipv6_addresses)
{
}

NX::win::hardware::network_adapter_information::network_adapter_information(const void* adapter_data)
{
    load(adapter_data);
}

NX::win::hardware::network_adapter_information::~network_adapter_information()
{
}

NX::win::hardware::network_adapter_information& NX::win::hardware::network_adapter_information::operator = (const NX::win::hardware::network_adapter_information& other)
{
    if (this != &other) {
        _adapter_name = other.get_adapter_name();
        _friendly_name = other.get_friendly_name();
        _description = other.get_description();
        _physical_address = other.get_mac_address();
        _if_type = other.get_if_type();
        _oper_status = other.get_oper_status();
        _ipv4_enabled = other.is_ipv4_enabled();
        _ipv6_enabled = other.is_ipv6_enabled();
        _ipv4_addresses = other.get_ipv4_addresses();
        _ipv6_addresses = other.get_ipv6_addresses();
    }
    return *this;
}

void NX::win::hardware::network_adapter_information::clear()
{
    _adapter_name.clear();
    _friendly_name.clear();
    _description.clear();
    _physical_address.clear();
    _if_type = 0;
    _oper_status = 0;
    _ipv4_enabled = false;
    _ipv6_enabled = false;
    _ipv4_addresses.clear();
    _ipv6_addresses.clear();
}

typedef PWSTR(NTAPI* FpRtlIpv4AddressToString)(_In_ const IN_ADDR *Addr, _Out_ PWSTR S);
typedef PWSTR(NTAPI* FpRtlIpv6AddressToString)(_In_ const IN6_ADDR *Addr, _Out_ PWSTR S);

class RtlIpAddressToStringW
{
public:
    RtlIpAddressToStringW() : _fp_ipv4(NULL), _fp_ipv6(NULL)
    {
        HMODULE mod = GetModuleHandleW(L"ntdll.dll");
        if (NULL != mod) {
            _fp_ipv4 = (FpRtlIpv4AddressToString)GetProcAddress(mod, "RtlIpv4AddressToStringW");
            _fp_ipv6 = (FpRtlIpv6AddressToString)GetProcAddress(mod, "RtlIpv6AddressToStringW");
        }
    }

    ~RtlIpAddressToStringW() {}

    PWSTR operator () (_In_ const IN_ADDR *Addr, _Out_ PWSTR S)
    {
        return (NULL != _fp_ipv4) ? _fp_ipv4(Addr, S) : NULL;
    }

    PWSTR operator () (_In_ const IN6_ADDR *Addr, _Out_ PWSTR S)
    {
        return (NULL != _fp_ipv6) ? _fp_ipv6(Addr, S) : NULL;
    }

private:
    FpRtlIpv4AddressToString    _fp_ipv4;
    FpRtlIpv6AddressToString    _fp_ipv6;
};

static bool is_ipv6_link_local_or_special_use(const std::wstring& ipv6_address)
{
    if (0 == ipv6_address.find(L"fe")) {
        wchar_t c = ipv6_address[2];
        if (c == '8' || c == '9' || c == 'a' || c == 'b') {
            // local link
            return true;
        }
    }
    else if (0 == ipv6_address.find(L"2001:0:")) {
        // special use
        return true;
    }
    else {
        ; // nothing
    }

    return false;
}

void NX::win::hardware::network_adapter_information::load(const void* adapter_data)
{
    PIP_ADAPTER_ADDRESSES pAdapter = (PIP_ADAPTER_ADDRESSES)adapter_data;

    // adapter name
    _adapter_name = NX::conversion::ansi_to_utf16(pAdapter->AdapterName);

    // adapter friendly name
    if (NULL != pAdapter->FriendlyName) {
        _friendly_name = pAdapter->FriendlyName;
    }

    // adapter description
    if (NULL != pAdapter->Description) {
        _description = pAdapter->Description;
    }

    // adapter mac address
    if (NULL != pAdapter->PhysicalAddress && 0 != pAdapter->PhysicalAddressLength) {
        std::for_each(pAdapter->PhysicalAddress, pAdapter->PhysicalAddress + pAdapter->PhysicalAddressLength, [&](const BYTE v) {
            if (!_physical_address.empty()) _physical_address += L"-";
            _physical_address += NX::string_formater(L"%02X", v);
        });
    }

    // adapter IfType and OperStatus
    _if_type = pAdapter->IfType;
    _oper_status = pAdapter->OperStatus;
    // ipv4/ipv6 status
    _ipv4_enabled = pAdapter->Ipv4Enabled;
    _ipv6_enabled = pAdapter->Ipv6Enabled;

    // ip addresses
    static RtlIpAddressToStringW ip_conv;
    PIP_ADAPTER_UNICAST_ADDRESS ip_address = pAdapter->FirstUnicastAddress;
    while (NULL != ip_address) {
        if (ip_address->Address.lpSockaddr->sa_family == AF_INET) {
            SOCKADDR_IN* ipv4 = reinterpret_cast<SOCKADDR_IN*>(ip_address->Address.lpSockaddr);
            wchar_t str_buffer[INET_ADDRSTRLEN] = { 0 };
            ip_conv(&(ipv4->sin_addr), str_buffer);
            if (L'\0' != str_buffer[0]) {
                _ipv4_addresses.push_back(std::wstring(str_buffer));
            }
        }
        else if (ip_address->Address.lpSockaddr->sa_family == AF_INET6) {
            SOCKADDR_IN6* ipv6 = reinterpret_cast<SOCKADDR_IN6*>(ip_address->Address.lpSockaddr);
            wchar_t str_buffer[INET6_ADDRSTRLEN] = { 0 };
            ip_conv(&(ipv6->sin6_addr), str_buffer);
            std::wstring ws_ip(str_buffer);
            if (!ws_ip.empty() && !is_ipv6_link_local_or_special_use(ws_ip)) {
                _ipv6_addresses.push_back(std::wstring(ws_ip));
            }
        }
        else {
            ; // unknown
        }

        ip_address = ip_address->Next;
    }
}


std::wstring NX::win::hardware::network_adapter_information::get_if_type_name() const
{
    std::wstring s;

    switch (_if_type)
    {
    case IF_TYPE_OTHER:
        s = L"Others";
        break;
    case IF_TYPE_ETHERNET_CSMACD:
        s = L"Ethernet network interface";
        break;
    case IF_TYPE_ISO88025_TOKENRING:
        s = L"Token ring network interface";
        break;
    case IF_TYPE_PPP:
        s = L"PPP network interface";
        break;
    case IF_TYPE_SOFTWARE_LOOPBACK:
        s = L"Software loopback network interface";
        break;
    case IF_TYPE_ATM:
        s = L"ATM network interface";
        break;
    case IF_TYPE_IEEE80211:
        s = L"IEEE 802.11 wireless network interface";
        break;
    case IF_TYPE_TUNNEL:
        s = L"Tunnel type encapsulation network interface";
        break;
    case IF_TYPE_IEEE1394:
        s = L"IEEE 1394 (Firewire) high performance serial bus network interface";
        break;
    default:
        s = L"Unknown";
        break;
    }

    return std::move(s);
}

std::wstring NX::win::hardware::network_adapter_information::get_oper_status_name() const
{
    std::wstring s;

    switch (_oper_status)
    {
    case IfOperStatusUp:
        s = L"Up";
        break;
    case IfOperStatusDown:
        s = L"Down";
        break;
    case IfOperStatusTesting:
        s = L"Testing Mode";
        break;
    case IfOperStatusDormant:
        s = L"Pending";
        break;
    case IfOperStatusNotPresent:
        s = L"Down (Component not present)";
        break;
    case IfOperStatusLowerLayerDown:
        s = L"Down (Lower layer interface is down)";
        break;
    case IfOperStatusUnknown:
    default:
        s = L"Unknown";
        break;
    }

    return std::move(s);
}

bool NX::win::hardware::network_adapter_information::is_ethernet_adapter() const
{
    return (IF_TYPE_ETHERNET_CSMACD == get_if_type());
}

bool NX::win::hardware::network_adapter_information::is_ppp_adapter() const
{
    return (IF_TYPE_PPP == get_if_type());
}

bool NX::win::hardware::network_adapter_information::is_80211_adapter() const
{
    return (IF_TYPE_IEEE80211 == get_if_type());
}

bool NX::win::hardware::network_adapter_information::is_1394_adapter() const
{
    return (IF_TYPE_IEEE1394 == get_if_type());
}

bool NX::win::hardware::network_adapter_information::is_network_adapter() const
{
    return (is_ethernet_adapter() || is_ppp_adapter() || is_80211_adapter());
}

bool NX::win::hardware::network_adapter_information::is_active() const
{
    return (IfOperStatusUp == get_oper_status());
}

bool NX::win::hardware::network_adapter_information::is_connected() const
{
    return (IfOperStatusUp == get_oper_status());
}

std::vector<NX::win::hardware::network_adapter_information> NX::win::hardware::get_all_adapters()
{
    std::vector<NX::win::hardware::network_adapter_information> adapters;
    std::vector<unsigned char> buf;
    PIP_ADAPTER_ADDRESSES pAddresses = NULL;
    PIP_ADAPTER_ADDRESSES pCurAddress = NULL;
    ULONG dwSize = sizeof(IP_ADAPTER_ADDRESSES);
    ULONG dwRetVal = 0;

    buf.resize(dwSize, 0);
    pAddresses = (PIP_ADAPTER_ADDRESSES)buf.data();
    memset(pAddresses, 0, dwSize);

    dwRetVal = GetAdaptersAddresses(AF_UNSPEC, 0, NULL, pAddresses, &dwSize);
    if (ERROR_SUCCESS != dwRetVal) {
        if (ERROR_BUFFER_OVERFLOW != dwRetVal) {
            return std::move(adapters);
        }

        dwSize += sizeof(IP_ADAPTER_ADDRESSES);
        buf.resize(dwSize, 0);
        pAddresses = (PIP_ADAPTER_ADDRESSES)buf.data();
        memset(pAddresses, 0, dwSize);
    }

    if (NULL == pAddresses) {
        return std::move(adapters);
    }

    dwRetVal = GetAdaptersAddresses(AF_UNSPEC, 0, NULL, pAddresses, &dwSize);
    if (ERROR_SUCCESS != dwRetVal) {
        return std::move(adapters);
    }

    pCurAddress = pAddresses;
    do {
        if (IF_TYPE_SOFTWARE_LOOPBACK == pCurAddress->IfType) {
            continue;
        }
        adapters.push_back(NX::win::hardware::network_adapter_information(pCurAddress));
    } while (NULL != (pCurAddress = pCurAddress->Next));

    return std::move(adapters);
}

std::vector<NX::win::hardware::network_adapter_information> NX::win::hardware::get_all_network_adapters()
{
    std::vector<NX::win::hardware::network_adapter_information> active_adapters;
    const std::vector<NX::win::hardware::network_adapter_information>& all_adapters = NX::win::hardware::get_all_adapters();
    std::for_each(all_adapters.begin(), all_adapters.end(), [&](const NX::win::hardware::network_adapter_information& adapter) {
        if (adapter.is_network_adapter()) {
            active_adapters.push_back(adapter);
        }
    });
    return std::move(active_adapters);
}

std::vector<NX::win::hardware::network_adapter_information> NX::win::hardware::get_active_network_adapters()
{
    std::vector<NX::win::hardware::network_adapter_information> active_adapters;
    const std::vector<NX::win::hardware::network_adapter_information>& all_adapters = NX::win::hardware::get_all_adapters();
    std::for_each(all_adapters.begin(), all_adapters.end(), [&](const NX::win::hardware::network_adapter_information& adapter) {
        if (adapter.is_network_adapter() && adapter.is_connected()) {
            active_adapters.push_back(adapter);
        }
    });
    return std::move(active_adapters);
}



//
//  class file_association
//

file_association::file_association()
{
}

file_association::file_association(const std::wstring& fullpath_or_filename) : _extension(normalize_extension(fullpath_or_filename))
{
    get_association();
}

file_association::file_association(const std::wstring& fullpath_or_filename, const std::wstring& user_sid) : _extension(normalize_extension(fullpath_or_filename))
{
    get_association2(user_sid);
}

file_association::file_association(const file_association& other) : _extension(other._extension), _executable(other._executable), _commandline(other._commandline)
{
}

file_association::file_association(file_association&& other) : _extension(std::move(other._extension)), _executable(std::move(other._executable)), _commandline(std::move(other._commandline))
{
}

file_association::~file_association()
{
}

std::wstring file_association::normalize_extension(const std::wstring& fullpath_or_filename)
{
    const wchar_t* p = wcsrchr(fullpath_or_filename.c_str(), L'\\');
	std::wstring filename;
    filename = (p == nullptr) ? fullpath_or_filename.c_str() : (p + 1);
    p = wcsrchr(filename.c_str(), L'.');
	if (nullptr == p) { //some applications like outlook create special temporary file name which is not using .
		//the following code may false detection. however, the wrong extension may not find associated app. which should 
		std::replace(filename.begin(), filename.end(), L' ', L'.');//replace all space with dot
		p = wcsrchr(filename.c_str(), L'.');
		if (p && (*(p + 1) == L'(' || *(p + 1) == L'[' || *(p + 1) == L'{')) {//this could be index for duplicate name, try one more.
			std::wstring::size_type pos = filename.find_last_of(L".");
			filename = filename.substr(0, pos);
			p = wcsrchr(filename.c_str(), L'.');
		}
	}
    return  std::wstring((nullptr == p) ? L"" : p);
}

void file_association::clear()
{
    _extension.clear();
    _executable.clear();
    _commandline.clear();
}

file_association& file_association::operator = (const file_association& other)
{
    if (this != &other) {
        _extension = other._extension;
        _executable = other._executable;
        _commandline = other._commandline;
    }
    return *this;
}

file_association& file_association::operator = (file_association&& other)
{
    if (this != &other) {
        _extension = std::move(other._extension);
        _executable = std::move(other._executable);
        _commandline = std::move(other._commandline);
    }
    return *this;
}

static std::wstring query_association_string(_In_ ASSOCF flags, _In_ ASSOCSTR str, _In_ LPCWSTR pszAssoc, _In_opt_ LPCWSTR pszExtra)
{
    std::wstring s;

    DWORD dwSize = 0;
    HRESULT hr = ::AssocQueryStringW(flags, str, pszAssoc, pszExtra, NULL, &dwSize);
    if (FAILED(hr) || 0 == dwSize) {
        return s;
    }

    hr = AssocQueryStringW(flags, str, pszAssoc, pszExtra, NX::string_buffer<wchar_t>(s, dwSize), &dwSize);
    if (FAILED(hr)) {
        s.clear();
        return s;
    }

    return std::move(s);
}

std::wstring file_association::get_user_choice_progid(_In_ const std::wstring& file_extension, _In_ const std::wstring& user_sid)
{
    std::wstring prog_id;

    try {
        NX::win::reg_users target_key;
        const std::wstring regpath(user_sid + L"\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\FileExts\\" + file_extension + L"\\UserChoice");
        const std::wstring progidname(L"ProgId");
        target_key.open(regpath, NX::win::reg_key::reg_wow64_64, true);
        target_key.read_value(progidname, prog_id);
    }
    catch (const std::exception& e) {
        UNREFERENCED_PARAMETER(e);
        prog_id.clear();
    }

    return std::move(prog_id);
}

std::wstring file_association::get_user_class_progid(_In_ const std::wstring& file_extension, _In_ const std::wstring& user_sid)
{
    std::wstring prog_id;

    try {
    	NX::win::reg_users target_key;
    	const std::wstring regpath(user_sid + L"\\SOFTWARE\\Classes\\" + file_extension);
    	target_key.open(regpath, NX::win::reg_key::reg_wow64_64, true);
        prog_id = target_key.read_default_value();
    }
    catch (const std::exception& e) {
    	UNREFERENCED_PARAMETER(e);
        prog_id.clear();
    }

    return std::move(prog_id);
}

std::wstring file_association::get_user_shell_command(_In_ const std::wstring& prog_id, _In_ const std::wstring& operation, _In_ const std::wstring& user_sid)
{
    std::wstring commandline;

    try {
        NX::win::reg_users target_key;
        const std::wstring regpath(user_sid + L"\\SOFTWARE\\Classes\\" + prog_id + L"\\Shell\\" + operation + L"\\Command");
        target_key.open(regpath, NX::win::reg_key::reg_wow64_64, true);
        commandline = target_key.read_default_value();
    }
    catch (const std::exception& e) {
        UNREFERENCED_PARAMETER(e);
        commandline.clear();
    }

    return std::move(commandline);
}

std::wstring file_association::get_user_content_type(_In_ const std::wstring& file_extension, _In_ const std::wstring& user_sid)
{
    std::wstring content_type;

    try {
        NX::win::reg_users target_key;
        const std::wstring regpath(user_sid + L"\\SOFTWARE\\Classes\\" + file_extension);
        target_key.open(regpath, NX::win::reg_key::reg_wow64_64, true);
        target_key.read_value(L"Content Type", content_type);
    }
    catch (const std::exception& e) {
        UNREFERENCED_PARAMETER(e);
        content_type.clear();
    }

    return std::move(content_type);
}

std::wstring file_association::get_root_class_progid(_In_ const std::wstring& file_extension)
{
    std::wstring prog_id;

    try {
        NX::win::reg_local_machine target_key;
        const std::wstring regpath(L"SOFTWARE\\Classes\\" + file_extension);
        target_key.open(regpath, NX::win::reg_key::reg_wow64_64, true);
        prog_id = target_key.read_default_value();
    }
    catch (const std::exception& e) {
        UNREFERENCED_PARAMETER(e);
        prog_id.clear();
    }

    return std::move(prog_id);
}

std::wstring file_association::get_root_shell_command(_In_ const std::wstring& prog_id, _In_ const std::wstring& operation)
{
    std::wstring commandline;

    try {
        NX::win::reg_local_machine target_key;
        const std::wstring regpath(L"SOFTWARE\\Classes\\" + prog_id + L"\\Shell\\" + operation + L"\\Command");
        target_key.open(regpath, NX::win::reg_key::reg_wow64_64, true);
        commandline = target_key.read_default_value();
    }
    catch (const std::exception& e) {
        UNREFERENCED_PARAMETER(e);
        commandline.clear();
    }

    return std::move(commandline);
}

std::wstring file_association::get_root_content_type(_In_ const std::wstring& file_extension)
{
    std::wstring content_type;

    try {
        NX::win::reg_local_machine target_key;
        const std::wstring regpath(L"SOFTWARE\\Classes\\" + file_extension);
        target_key.open(regpath, NX::win::reg_key::reg_wow64_64, true);
        target_key.read_value(L"Content Type", content_type);
    }
    catch (const std::exception& e) {
        UNREFERENCED_PARAMETER(e);
        content_type.clear();
    }

    return std::move(content_type);
}

std::wstring file_association::get_shell_command(_In_ const std::wstring& prog_id, _In_ const std::wstring& user_sid)
{
    std::wstring command;

    if(!user_sid.empty()) {
        command = get_user_shell_command(prog_id, L"open", user_sid);
        if (command.empty()) {
            command = get_user_shell_command(prog_id, L"read", user_sid);
            if (command.empty()) {
                command = get_user_shell_command(prog_id, L"edit", user_sid);
            }
        }
    }

    if (command.empty()) {
        command = get_root_shell_command(prog_id, L"open");
        if (command.empty()) {
            command = get_root_shell_command(prog_id, L"read");
            if (command.empty()) {
                command = get_root_shell_command(prog_id, L"edit");
            }
        }
    }

    return std::move(command);
}

std::wstring file_association::get_content_type(_In_ const std::wstring& file_extension, _In_ const std::wstring& user_sid)
{
    std::wstring content_type;

    if(!user_sid.empty()) {
        content_type = get_user_content_type(file_extension, user_sid);
    }

    if (content_type.empty()) {
        content_type = get_root_content_type(file_extension);
    }

    return std::move(content_type);
}

std::vector<std::wstring> file_association::extract_command(const std::wstring& command)
{
    std::vector<std::wstring> arguments;
    int argc = 0;

    if (command.empty()) {
        return arguments;
    }

    LPWSTR* argv = CommandLineToArgvW(command.c_str(), &argc);

    if (L'\"' != command[0]) {

        bool exe_completed = false;
        std::wstring exe_path;

        for (int i = 0; i < argc; i++) {

            if (!exe_completed) {
                if(!exe_path.empty()) exe_path.append(L" ");
                exe_path.append(argv[i]);
                if (boost::algorithm::iends_with(exe_path, L".exe")) {
                    exe_completed = true;
                    arguments.push_back(exe_path);
                }
            }
            else {
                arguments.push_back(argv[i]);
            }
        }
    }
    else {
        for (int i = 0; i < argc; i++) {
            arguments.push_back(argv[i]);
        }
    }

    return std::move(arguments);
}

void file_association::get_association()
{
    static std::wstring openwith_executable;

    if (_extension.empty()) {
        return;
    }

    _commandline = query_association_string(ASSOCF_NONE, ASSOCSTR_COMMAND, _extension.c_str(), L"open");
    _content_type = query_association_string(ASSOCF_NONE, ASSOCSTR_CONTENTTYPE, _extension.c_str(), NULL);
    _prog_id = query_association_string(ASSOCF_NONE, ASSOCSTR_PROGID, _extension.c_str(), NULL);
    _executable = query_association_string(ASSOCF_NONE, ASSOCSTR_EXECUTABLE, _extension.c_str(), L"open");

    if (openwith_executable.empty()) {
        openwith_executable = NX::win::get_system_directory();
        if (!openwith_executable.empty()) {
            openwith_executable += L"\\OpenWith.exe";
        }
    }
    if (!openwith_executable.empty() && 0 == _wcsicmp(_executable.c_str(), openwith_executable.c_str())) {
        _executable.clear();
    }

    if (_executable.empty()) {
        return;
    }

}

void file_association::get_association2(_In_ const std::wstring& user_sid)
{
    if (user_sid.empty()) {
        get_association();
    }
    else {

        _prog_id = get_user_choice_progid(_extension, user_sid);
        if (_prog_id.empty()) {
            _prog_id = get_user_class_progid(_extension, user_sid);
        }
        if (!_prog_id.empty()) {
            // get command line
            _commandline = get_shell_command(_prog_id, user_sid);
            if (!_commandline.empty()) {
                const std::vector<std::wstring>& arguments = extract_command(_commandline);
                if (!arguments.empty()) {
                    _executable = arguments[0];
                }
            }
        }

        if (_executable.empty()) {
            // Cannot find executable from current user, then use default functions
            get_association();
        }
        else {
            _content_type = get_content_type(_extension, user_sid);
        }
    }
}





//
//  class pe_file
//

pe_file::pe_file() : _machine(0), _characteristics(0), _subsystem(0), _image_checksum(0), _image_base(0), _base_of_code(0), _address_of_entry(0)
{
}

pe_file::pe_file(const std::wstring& file) : _machine(0), _characteristics(0), _subsystem(0), _image_checksum(0), _image_base(0), _base_of_code(0), _address_of_entry(0)
{
    load(file);
}

pe_file::~pe_file()
{
}

void pe_file::load(const std::wstring& file)
{
    if (!load_pe_header(file)) {
        return;
    }

    load_signature(file);
    _file_version = file_version(file);
}

void pe_file::clear()
{
    _image_publisher.clear();
    _machine = 0;
    _characteristics = 0;
    _subsystem = 0;
    _image_checksum = 0;
    _image_base = 0;
    _base_of_code = 0;
    _address_of_entry = 0;
    _file_version.clear();
}

pe_file& pe_file::operator = (const pe_file& other)
{
    if (this != &other) {
        _image_publisher = other.get_image_publisher();
        _machine = other.get_machine_code();
        _characteristics = other.get_characteristics();
        _subsystem = other.get_subsystem();
        _image_checksum = other.get_image_checksum();
        _image_base = other.get_image_base();
        _base_of_code = other.get_base_of_code();
        _address_of_entry = other.get_address_of_entry();
        _file_version = other.get_file_version();
    }
    return *this;
}

bool pe_file::load_pe_header(const std::wstring& file)
{
    bool result = false;
    HANDLE h = INVALID_HANDLE_VALUE;
    HANDLE hmap = NULL;
    LPVOID p = NULL;

    h = CreateFileW(file.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    if (h == INVALID_HANDLE_VALUE) {
        return false;
    }

    do {

        IMAGE_DOS_HEADER dos_header = { 0 };
        IMAGE_NT_HEADERS nt_headers = { 0 };
        unsigned long bytes_read = 0;

        memset(&dos_header, 0, sizeof(dos_header));
        memset(&nt_headers, 0, sizeof(nt_headers));

        if (!ReadFile(h, &dos_header, (unsigned long)sizeof(dos_header), &bytes_read, NULL)) {
            break;
        }
        if (bytes_read != (unsigned long)sizeof(dos_header)) {
            break;
        }
        if (IMAGE_DOS_SIGNATURE != dos_header.e_magic) {
            break;
        }

        if (INVALID_SET_FILE_POINTER == SetFilePointer(h, dos_header.e_lfanew, NULL, FILE_BEGIN)) {
            break;
        }
        if (!ReadFile(h, &nt_headers, (unsigned long)sizeof(nt_headers), &bytes_read, NULL)) {
            break;
        }
        CloseHandle(h); h = INVALID_HANDLE_VALUE;
        if (bytes_read != (unsigned long)sizeof(nt_headers)) {
            break;
        }
        if (0x00004550 != nt_headers.Signature) {
            break;
        }

        // Good
        result = true;
        _machine = nt_headers.FileHeader.Machine;
        _characteristics = nt_headers.FileHeader.Characteristics;

        _image_checksum = nt_headers.OptionalHeader.CheckSum;
        _image_base = nt_headers.OptionalHeader.ImageBase;
        _base_of_code = nt_headers.OptionalHeader.BaseOfCode;
        _address_of_entry = nt_headers.OptionalHeader.AddressOfEntryPoint;
        _subsystem = nt_headers.OptionalHeader.Subsystem;

    } while (false);

    // clean up
    if (INVALID_HANDLE_VALUE != h) {
        CloseHandle(h); h = INVALID_HANDLE_VALUE;
    }

    return result;
}

void pe_file::load_signature(const std::wstring& file)
{
    NX::cert::cert_context ccontext;
    if (ccontext.create(file)) {
        const NX::cert::cert_info& certinf = ccontext.get_cert_info();
        _image_publisher = certinf.get_subject_name();
    }
}


//
//
//

reg_key::reg_key() : _h(NULL)
{
}

reg_key::~reg_key()
{
    close();
}

void reg_key::create(HKEY root, const std::wstring& path, reg_position pos)
{
    unsigned long disposition = 0;
    unsigned long desired_access = KEY_ALL_ACCESS;
    switch (pos)
    {
    case reg_wow64_32:
        desired_access |= KEY_WOW64_32KEY;
        break;
    case reg_wow64_64:
        desired_access |= KEY_WOW64_64KEY;
        break;
    case reg_default:
    default:
        break;
    }
    long result = ::RegCreateKeyExW(root, path.c_str(), 0, NULL, REG_OPTION_NON_VOLATILE, desired_access, NULL, &_h, &disposition);
    if (ERROR_SUCCESS != result) {
        _h = NULL;
        throw NX::exception(WIN32_ERROR_MSG(result, "win_registry::create, RegCreateKeyExW"));
    }
}

void reg_key::open(HKEY root, const std::wstring& path, reg_position pos, bool read_only)
{
    unsigned long desired_access = read_only ? KEY_READ : (KEY_READ | KEY_WRITE);
    switch (pos)
    {
    case reg_wow64_32:
        desired_access |= KEY_WOW64_32KEY;
        break;
    case reg_wow64_64:
        desired_access |= KEY_WOW64_64KEY;
        break;
    default:
        break;
    }
    long result = ::RegOpenKeyExW(root, path.c_str(), 0, desired_access, &_h);
    if (ERROR_SUCCESS != result) {
        _h = NULL;
        throw NX::exception(WIN32_ERROR_MSG(result, "win_registry::open, RegOpenKeyExW"));
    }
}

void reg_key::close()
{
    if (NULL != _h) {
        RegCloseKey(_h);
        _h = NULL;
    }
}

void reg_key::remove(HKEY root, const std::wstring& path)
{
    long result = RegDeleteKey(root, path.c_str());
    if (ERROR_SUCCESS != result) {
        throw NX::exception(WIN32_ERROR_MSG(result, "win_registry::remove, RegDeleteKey"));
    }
}

bool reg_key::exist(HKEY root, const std::wstring& path) noexcept
{
    HKEY h = NULL;
    long result = ::RegOpenKeyExW(root, path.c_str(), 0, KEY_READ, &h);
    if (ERROR_SUCCESS == result) {
        RegCloseKey(h);
        return true;
    }
    else {
        return (ERROR_FILE_NOT_FOUND == result) ? false : true;
    }
}


std::wstring reg_key::read_default_value()
{
    std::wstring value;
    read_value(L"", value);
    return std::move(value);
}

void reg_key::read_value(const std::wstring& name, unsigned long* value)
{
    unsigned long value_type = 0;
    const std::vector<unsigned char>& buf = internal_read_value(name, &value_type);
    switch (value_type)
    {
    case REG_DWORD:
    case REG_QWORD:
        assert(buf.size() >= 4);
        *value = *((unsigned long*)buf.data());
        break;
    case REG_DWORD_BIG_ENDIAN:
        assert(buf.size() >= 4);
        *value = convert_endian(*((unsigned long*)buf.data()));
        break;
    default:
        throw NX::exception(WIN32_ERROR_MSG2(ERROR_INVALID_DATATYPE));
        break;
    }
}

void reg_key::read_value(const std::wstring& name, unsigned __int64* value)
{
    unsigned long value_type = 0;
    const std::vector<unsigned char>& buf = internal_read_value(name, &value_type);
    switch (value_type)
    {
    case REG_DWORD:
        assert(buf.size() >= 4);
        *value = *((unsigned long*)buf.data());
        break;
    case REG_QWORD:
        assert(buf.size() >= 4);
        *value = *((unsigned __int64*)buf.data());
        break;
    case REG_DWORD_BIG_ENDIAN:
        assert(buf.size() >= 4);
        *value = (unsigned __int64)convert_endian(*((unsigned long*)buf.data()));
        break;
    default:
        throw NX::exception(WIN32_ERROR_MSG2(ERROR_INVALID_DATATYPE));
        break;
    }
}

void reg_key::read_value(const std::wstring& name, std::wstring& value)
{
    unsigned long value_type = 0;
    const std::vector<unsigned char>& buf = internal_read_value(name, &value_type);
    switch (value_type)
    {
    case REG_SZ:
        value = buf.empty() ? std::wstring(L"") : ((const wchar_t*)buf.data());
        break;
    case REG_EXPAND_SZ:
        value = buf.empty() ? std::wstring(L"") : expand_env_string((const wchar_t*)buf.data());
        break;
    case REG_MULTI_SZ:
        {
            const std::vector<std::wstring>& strings = expand_multi_strings((const wchar_t*)buf.data());
            value = strings.empty() ? std::wstring(L"") : strings[0];
        }
        break;
    default:
        throw NX::exception(WIN32_ERROR_MSG2(ERROR_INVALID_DATATYPE));
        break;
    }
}

void reg_key::read_value(const std::wstring& name, std::vector<std::wstring>& value)
{
    unsigned long value_type = 0;
    const std::vector<unsigned char>& buf = internal_read_value(name, &value_type);
    switch (value_type)
    {
    case REG_SZ:
        if (!buf.empty()) {
            value.push_back((const wchar_t*)buf.data());
        }
        break;
    case REG_EXPAND_SZ:
        if (!buf.empty()) {
            value.push_back(expand_env_string((const wchar_t*)buf.data()));
        }
        break;
    case REG_MULTI_SZ:
        value = expand_multi_strings((const wchar_t*)buf.data());
        break;
    default:
        throw NX::exception(WIN32_ERROR_MSG2(ERROR_INVALID_DATATYPE));
        break;
    }
}

void reg_key::read_value(const std::wstring& name, std::vector<unsigned char>& value)
{
    unsigned long value_type = 0;
    value = internal_read_value(name, &value_type);
}


void reg_key::set_default_value(const std::wstring& value)
{
    set_value(L"", value, false);
}

void reg_key::set_value(const std::wstring& name, unsigned long value)
{
    internal_set_value(name, REG_DWORD, &value, sizeof(unsigned long));
}

void reg_key::set_value(const std::wstring& name, unsigned __int64 value)
{
    internal_set_value(name, REG_QWORD, &value, sizeof(unsigned __int64));
}

void reg_key::set_value(const std::wstring& name, const std::wstring& value, bool expandable)
{
    unsigned long size = (unsigned long)(sizeof(wchar_t) * (value.length() + 1));
    internal_set_value(name, expandable ? REG_SZ : REG_EXPAND_SZ, value.c_str(), size);
}

void reg_key::set_value(const std::wstring& name, const std::vector<std::wstring>& value)
{
    const std::vector<wchar_t>& buf = create_multi_strings_buffer(value);
    unsigned long size = (unsigned long)(sizeof(wchar_t) * buf.size());
    internal_set_value(name, REG_MULTI_SZ, buf.data(), size);
}

void reg_key::set_value(const std::wstring& name, const std::vector<unsigned char>& value)
{
    internal_set_value(name, REG_BINARY, value.data(), (unsigned long)value.size());
}

std::wstring reg_key::expand_env_string(const std::wstring& s)
{
    static const size_t max_info_buf_size = 32767;
    std::wstring output;
    std::vector<wchar_t> buf;
    buf.resize(max_info_buf_size, 0);
    if (0 != ExpandEnvironmentStrings(s.c_str(), buf.data(), max_info_buf_size)) {
        output = buf.data();
    }
    return std::move(output);
}

std::vector<std::wstring> reg_key::expand_multi_strings(const wchar_t* s)
{
    std::vector<std::wstring> strings;
    while (0 != s[0]) {
        std::wstring ws(s);
        assert(ws.length() > 0);
        s += ws.length() + 1;
        strings.push_back(ws);
    }
    return std::move(strings);
}

std::vector<wchar_t> reg_key::create_multi_strings_buffer(const std::vector<std::wstring>& strings)
{
    std::vector<wchar_t> buf;
    std::for_each(strings.begin(), strings.end(), [&](const std::wstring& s) {
        std::for_each(s.begin(), s.end(), [&](const wchar_t& c) {
            buf.push_back(c);
        });
        buf.push_back(L'\0');
    });
    buf.push_back(L'\0');
    return std::move(buf);
}

unsigned long reg_key::convert_endian(unsigned long u)
{
    return (((u >> 24) & 0xFF) | ((u << 24) & 0xFF000000) | ((u >> 8) & 0xFF00) | ((u << 8) & 0xFF0000));
}

std::vector<unsigned char> reg_key::internal_read_value(const std::wstring& name, unsigned long* value_type)
{
    long result = 0;
    std::vector<unsigned char> buf;
    unsigned long value_size = 1;

    buf.resize(1, 0);
    value_size = 1;

    result = ::RegQueryValueExW(_h, name.empty() ? NULL : name.c_str(), NULL, value_type, (LPBYTE)buf.data(), &value_size);
    if (ERROR_SUCCESS == result) {
        // succeed
        if (0 == value_size) buf.clear();
        return std::move(buf);
    }

    // failed, and error is not ERROR_MORE_DATA
    if (ERROR_MORE_DATA != result) {
        throw NX::exception(WIN32_ERROR_MSG2(result));
    }

    // reset buffer size
    buf.resize(value_size, 0);
    // try to get data again
    result = ::RegQueryValueExW(_h, name.empty() ? NULL : name.c_str(), NULL, value_type, (LPBYTE)buf.data(), &value_size);
    if (ERROR_SUCCESS != result) {
        throw NX::exception(WIN32_ERROR_MSG(result, "win_registry::read_value, RegQueryValueExW"));
    }

    return std::move(buf);
}

void reg_key::internal_set_value(const std::wstring& name, unsigned long value_type, const void* value, unsigned long size)
{
    long result = ::RegSetValueExW(_h, name.empty() ? NULL : name.c_str(), 0, value_type, (LPBYTE)value, size);
    if (ERROR_SUCCESS != result) {
        throw NX::exception(WIN32_ERROR_MSG2(result));
    }
}

sid::sid() : _sid(NULL)
{
}

sid::sid(PSID p) : _sid(NULL)
{
    if (p != NULL && IsValidSid(p)) {
        unsigned long size = GetLengthSid(p);
        if (0 != size) {
            _sid = (PSID)LocalAlloc(LMEM_FIXED, size);
            if (NULL != _sid) {
                if (!CopySid(size, _sid, p)) {
                    LocalFree(_sid);
                    _sid = NULL;
                }
            }
        }
    }
}

sid::sid(const std::wstring& s) : _sid(NULL)
{
    if (!ConvertStringSidToSidW(s.c_str(), &_sid)) {
        _sid = NULL;
    }
}

sid::~sid()
{
    clear();
}


const SID_IDENTIFIER_AUTHORITY sid::null_authority   = { 0,0,0,0,0,0 };
const SID_IDENTIFIER_AUTHORITY sid::world_authority  = { 0,0,0,0,0,1 };
const SID_IDENTIFIER_AUTHORITY sid::nt_authority     = { 0,0,0,0,0,5 };

sid sid::create(PSID_IDENTIFIER_AUTHORITY pIdentifierAuthority,
                BYTE nSubAuthorityCount,
                DWORD dwSubAuthority0,
                DWORD dwSubAuthority1,
                DWORD dwSubAuthority2,
                DWORD dwSubAuthority3,
                DWORD dwSubAuthority4,
                DWORD dwSubAuthority5,
                DWORD dwSubAuthority6,
                DWORD dwSubAuthority7)
{
    sid s;
    PSID p = NULL;
    if (AllocateAndInitializeSid(pIdentifierAuthority,
                                 nSubAuthorityCount,
                                 dwSubAuthority0,
                                 dwSubAuthority1,
                                 dwSubAuthority2,
                                 dwSubAuthority3,
                                 dwSubAuthority4,
                                 dwSubAuthority5,
                                 dwSubAuthority6,
                                 dwSubAuthority7, &p) && p != NULL) {
        s = p;
        FreeSid(p);
        p = NULL;
    }
    return s;
}

std::wstring sid::serialize(PSID psid)
{
    if (NULL == psid) {
        return std::wstring();
    }

    std::wstring s;
    LPWSTR str = NULL;
    if (ConvertSidToStringSid(psid, &str)) {
        s = str;
        LocalFree(str);
        str = NULL;
    }
    assert(NULL == str);
    return std::move(s);
}

bool sid::is_null_auth(PSID psid)
{
    PSID_IDENTIFIER_AUTHORITY pauth = GetSidIdentifierAuthority(psid);
    return (NULL != pauth && 0 == memcmp(pauth, &sid::null_authority, sizeof(SID_IDENTIFIER_AUTHORITY)));
}

bool sid::is_world_auth(PSID psid)
{
    PSID_IDENTIFIER_AUTHORITY pauth = GetSidIdentifierAuthority(psid);
    return (NULL != pauth && 0 == memcmp(pauth, &sid::world_authority, sizeof(SID_IDENTIFIER_AUTHORITY)));
}

bool sid::is_nt_auth(PSID psid)
{
    PSID_IDENTIFIER_AUTHORITY pauth = GetSidIdentifierAuthority(psid);
    return (NULL != pauth && 0 == memcmp(pauth, &sid::nt_authority, sizeof(SID_IDENTIFIER_AUTHORITY)));
}

bool sid::is_everyone_sid(PSID psid)
{
    return IsWellKnownSid(psid, WinWorldSid) ? true : false;
}

bool sid::is_nt_local_system_sid(PSID psid)
{
    return IsWellKnownSid(psid, WinLocalSystemSid) ? true : false;
}

bool sid::is_nt_local_service_sid(PSID psid)
{
    return IsWellKnownSid(psid, WinLocalServiceSid) ? true : false;
}

bool sid::is_nt_network_service_sid(PSID psid)
{
    return IsWellKnownSid(psid, WinNetworkServiceSid) ? true : false;
}

bool sid::is_nt_domain_sid(PSID psid)
{
    return (sid::is_nt_auth(psid) && (*GetSidSubAuthorityCount(psid) != 0) && (SECURITY_NT_NON_UNIQUE == *GetSidSubAuthority(psid, 0)));
}

bool sid::is_nt_builtin_sid(PSID psid)
{
    return (sid::is_nt_auth(psid) && (*GetSidSubAuthorityCount(psid) != 0) && (SECURITY_BUILTIN_DOMAIN_RID == *GetSidSubAuthority(psid, 0)));
}

void sid::clear()
{
    if (NULL != _sid) {
        LocalFree(_sid);
        _sid = NULL;
    }
}

unsigned long sid::length() const
{
    return empty() ? 0UL : GetLengthSid(_sid);
}

sid& sid::operator = (const sid& other)
{
    if (this != &other) {
        clear();
        unsigned long size = other.length();
        if (0 != size) {
            _sid = (PSID)LocalAlloc(LMEM_FIXED, size);
            if (NULL != _sid) {
                if (!CopySid(size, _sid, other)) {
                    LocalFree(_sid);
                    _sid = NULL;
                }
            }
        }
    }
    return *this;
}

sid& sid::operator = (PSID other)
{
    clear();
    if (NULL != other) {
        unsigned long size = GetLengthSid(other);
        if (0 != size) {
            _sid = (PSID)LocalAlloc(LMEM_FIXED, size);
            if (NULL != _sid) {
                if (!CopySid(size, _sid, other)) {
                    LocalFree(_sid);
                    _sid = NULL;
                }
            }
        }
    }
    return *this;
}

std::wstring sid::serialize() const
{
    return sid::serialize(_sid);
}

bool sid::operator == (const sid& other) const
{
    if (empty() || other.empty()) {
        return (empty() && other.empty());
    }
    return EqualSid(_sid, other) ? true : false;
}

bool sid::operator == (PSID other) const
{
    if (empty() || NULL == other) {
        return (empty() && NULL == other);
    }
    return EqualSid(_sid, other) ? true : false;
}

//
//
//

explicit_access::explicit_access()
{
    memset(this, 0, sizeof(EXPLICIT_ACCESS));
}

explicit_access::explicit_access(const explicit_access& other)
{
    memcpy(this, &other, sizeof(EXPLICIT_ACCESS));
    Trustee.ptstrName = NULL;
    if (NULL != other.Trustee.ptstrName) {
        NX::win::sid temp_sid((PSID)other.Trustee.ptstrName);
        Trustee.ptstrName = (LPWSTR)temp_sid.detach();
    }
}

explicit_access::explicit_access(explicit_access&& other)
{
    memcpy(this, &other, sizeof(EXPLICIT_ACCESS));
    memset(&other, 0, sizeof(EXPLICIT_ACCESS));
}

explicit_access::explicit_access(PSID sid, unsigned long access_permissions, TRUSTEE_TYPE trustee_type, unsigned long inheritance)
{
    NX::win::sid temp_sid(sid);
    if (!temp_sid.empty()) {
        init(temp_sid.detach(), access_permissions, trustee_type, inheritance);
    }
}

explicit_access::explicit_access(const std::wstring& sid, unsigned long access_permissions, TRUSTEE_TYPE trustee_type, unsigned long inheritance)
{
    NX::win::sid temp_sid(sid);
    if (!temp_sid.empty()) {
        init(temp_sid.detach(), access_permissions, trustee_type, inheritance);
    }
}

explicit_access::explicit_access(SID_IDENTIFIER_AUTHORITY authority, unsigned long rid, unsigned long access_permissions, unsigned long inheritance)
{
    PSID psid = NULL;
    if (AllocateAndInitializeSid(&authority, 1, rid, 0, 0, 0, 0, 0, 0, 0, &psid)) {
        NX::win::sid temp_sid(psid);
        if (!temp_sid.empty()) {
            init(temp_sid.detach(), access_permissions, TRUSTEE_IS_WELL_KNOWN_GROUP, inheritance);
        }
        FreeSid(psid);
    }
}

explicit_access::explicit_access(SID_IDENTIFIER_AUTHORITY authority, unsigned long rid1, unsigned long rid2, unsigned long access_permissions, unsigned long inheritance)
{
    PSID psid = NULL;
    if (AllocateAndInitializeSid(&authority, 2, rid1, rid2, 0, 0, 0, 0, 0, 0, &psid)) {
        NX::win::sid temp_sid(psid);
        if (!temp_sid.empty()) {
            init(temp_sid.detach(), access_permissions, TRUSTEE_IS_WELL_KNOWN_GROUP, inheritance);
        }
        FreeSid(psid);
    }
}

explicit_access::~explicit_access()
{
    if (NULL != Trustee.ptstrName) {
        LocalFree((PSID)Trustee.ptstrName);
    }
    memset(this, 0, sizeof(EXPLICIT_ACCESS));
}

explicit_access& explicit_access::operator = (const explicit_access& other)
{
    if (this != &other) {
        memcpy(this, &other, sizeof(EXPLICIT_ACCESS));
        Trustee.ptstrName = NULL;
        if (NULL != other.Trustee.ptstrName) {
            NX::win::sid temp_sid((PSID)other.Trustee.ptstrName);
            Trustee.ptstrName = (LPWSTR)temp_sid.detach();
        }
    }
    return *this;
}

explicit_access& explicit_access::operator = (explicit_access&& other)
{
    if (this != &other) {
        memcpy(this, &other, sizeof(EXPLICIT_ACCESS));
        memset(&other, 0, sizeof(EXPLICIT_ACCESS));
    }
    return *this;
}

void explicit_access::clear()
{
    if (NULL != Trustee.ptstrName) {
        LocalFree((PSID)Trustee.ptstrName);
    }
}

void explicit_access::init(PSID sid, unsigned long access_permissions, TRUSTEE_TYPE trustee_type, unsigned long inheritance) noexcept
{
    memset(this, 0, sizeof(EXPLICIT_ACCESS));
    grfAccessPermissions = access_permissions;
    grfAccessMode = SET_ACCESS;
    grfInheritance = inheritance;
    Trustee.TrusteeForm = TRUSTEE_IS_SID;
    Trustee.TrusteeType = trustee_type;
    Trustee.ptstrName = (LPWSTR)sid;  // transfer ownership
}

//
//
//

security_attribute::security_attribute() : SECURITY_ATTRIBUTES({ sizeof(SECURITY_ATTRIBUTES), NULL, FALSE }), _dacl(nullptr)
{
}

security_attribute::security_attribute(const explicit_access& ea) : SECURITY_ATTRIBUTES({ sizeof(SECURITY_ATTRIBUTES), NULL, FALSE }), _dacl(nullptr)
{
    init(&ea, 1);
}

security_attribute::security_attribute(const std::vector<explicit_access>& eas) : SECURITY_ATTRIBUTES({ sizeof(SECURITY_ATTRIBUTES), NULL, FALSE }), _dacl(nullptr)
{
    if (!eas.empty()) {
        init(eas.data(), (unsigned long)eas.size());
    }
    else {
        init(NULL, 0);
    }
}

security_attribute::~security_attribute()
{
    clear();
}

void security_attribute::clear() noexcept
{
    if (NULL != _dacl) {
        LocalFree(_dacl);
        _dacl = NULL;
    }
    if (NULL != lpSecurityDescriptor) {
        LocalFree(lpSecurityDescriptor);
        lpSecurityDescriptor = NULL;
    }
    
    nLength = sizeof(SECURITY_ATTRIBUTES);
    lpSecurityDescriptor = NULL;
    bInheritHandle = FALSE;
}

void security_attribute::init(const explicit_access* eas, unsigned long size)
{
    clear();

    try {

        // Initialize SA
        nLength = sizeof(SECURITY_ATTRIBUTES);
        bInheritHandle = FALSE;
        lpSecurityDescriptor = (PSECURITY_DESCRIPTOR)LocalAlloc(LPTR, SECURITY_DESCRIPTOR_MIN_LENGTH);
        if (NULL == lpSecurityDescriptor) {
            SetLastError(ERROR_OUTOFMEMORY);
            throw (int)ERROR_OUTOFMEMORY;
        }
        InitializeSecurityDescriptor(lpSecurityDescriptor, SECURITY_DESCRIPTOR_REVISION);

        if (NULL != eas && 0 != size) {
            // Create ACL
            SetEntriesInAclW(size, (PEXPLICIT_ACCESS)eas, NULL, &_dacl);
            if (NULL == _dacl) {
                throw (int)GetLastError();
            }
        }

        // Set ACL
        if (!SetSecurityDescriptorDacl(lpSecurityDescriptor, TRUE, _dacl, FALSE)) {
            throw (int)GetLastError();
        }
    }
    catch (...) {
        clear();
    }
}


//
//
//


bool file_security::grant_access(const std::wstring& file, const explicit_access& ea)
{
    return file_security::grant_access(file, &ea, 1);
}

bool file_security::grant_access(const std::wstring& file, const explicit_access* eas, unsigned long ea_count)
{
    bool result = false;
    DWORD dwRes = ERROR_SUCCESS;
    PACL pOldDACL = NULL, pNewDACL = NULL;
    PSECURITY_DESCRIPTOR pSD = NULL;

    do {

        dwRes = GetNamedSecurityInfoW(file.c_str(),
            SE_FILE_OBJECT,
            DACL_SECURITY_INFORMATION,
            NULL,
            NULL,
            &pOldDACL,
            NULL,
            &pSD);
        if (ERROR_SUCCESS != dwRes) {
            break;
        }

        // Create a new ACL that merges the new ACE
        // into the existing DACL.
        dwRes = SetEntriesInAclW(ea_count, (PEXPLICIT_ACCESS)eas, pOldDACL, &pNewDACL);
        if (ERROR_SUCCESS != dwRes) {
            break;
        }

        // Attach the new ACL as the object's DACL.
        dwRes = SetNamedSecurityInfoW((LPWSTR)file.c_str(),
            SE_FILE_OBJECT,
            DACL_SECURITY_INFORMATION,
            NULL,
            NULL,
            pNewDACL,
            NULL);
        if (ERROR_SUCCESS != dwRes) {
            break;
        }

        result = true;

    } while (FALSE);

    if (pSD != NULL) {
        LocalFree((HLOCAL)pSD);
        pSD = NULL;
    }
    if (pNewDACL != NULL) {
        LocalFree((HLOCAL)pNewDACL);
        pNewDACL = NULL;
    }

    return result;
}


//
//
//

host::host()
{
    unsigned long size = MAX_PATH;
    if (!GetComputerNameExW(ComputerNameDnsFullyQualified, NX::string_buffer<wchar_t>(_fqdn, MAX_PATH), &size)) {
        _fqdn.clear();
    }
    size = MAX_PATH;
    if (!GetComputerNameExW(ComputerNameDnsHostname, NX::string_buffer<wchar_t>(_host, MAX_PATH), &size)) {
        _host.clear();
    }
    size = MAX_PATH;
    if (!GetComputerNameExW(ComputerNameDnsDomain, NX::string_buffer<wchar_t>(_domain, MAX_PATH), &size)) {
        _domain.clear();
    }

	const std::vector<NX::win::hardware::network_adapter_information>& adapters = NX::win::hardware::get_all_network_adapters();
	for (int i = 0; i < (int)adapters.size(); i++) {
		//if (adapters[i].is_connected()) {
			if (adapters[i].get_ipv4_addresses().empty() == false)
			{
				_ip = adapters[i].get_ipv4_addresses()[0].c_str();
				break;
			}
		//}
	}
}

host::~host()
{
}

void host::clear()
{
    _fqdn.clear();
    _host.clear();
    _domain.clear();
	_ip.clear();
}

host& host::operator = (const host& other)
{
    if (this != &other) {
        _fqdn = other.fqdn_name();
        _host = other.dns_host_name();
        _domain = other.dns_domain_name();
		_ip = other.ip_address();
    }
    return *this;
}

bool host::operator == (const host& other) const
{
    return NX::utility::iequal<wchar_t>(other.fqdn_name(), fqdn_name());
}

bool host::operator == (const std::wstring& other) const
{
    return NX::utility::iequal<wchar_t>(other, fqdn_name());
}

std::wstring win::sam_compatiple_name_to_principle_name(const std::wstring& name)
{
    std::wstring principle_name;
    unsigned long size = MAX_PATH;
    if (!::TranslateNameW(name.c_str(), NameSamCompatible, NameUserPrincipal, NX::string_buffer<wchar_t>(principle_name, MAX_PATH), &size)) {
        principle_name.clear();
    }
    return std::move(principle_name);
}

std::wstring win::principle_name_to_sam_compatiple_name(const std::wstring& name)
{
    std::wstring sam_compatiple_name;
    unsigned long size = MAX_PATH;
    if (!::TranslateNameW(name.c_str(), NameUserPrincipal, NameSamCompatible, NX::string_buffer<wchar_t>(sam_compatiple_name, MAX_PATH), &size)) {
        sam_compatiple_name.clear();
    }
    return std::move(sam_compatiple_name);
}

std::wstring win::get_object_name(PSID psid)
{
    static const unsigned long max_name_size = 64;
    static  const win::host h;

    std::wstring name;
    std::wstring dns_user_name;
    std::wstring dns_domain_name;
    unsigned long dns_user_name_size = max_name_size;
    unsigned long dns_domain_name_size = max_name_size;
    SID_NAME_USE sid_name_use = SidTypeUser;


    if (sid::is_nt_local_system_sid(psid)) {
        return std::wstring(L"SYSTEM");
    }
    else if (sid::is_nt_local_service_sid(psid)) {
        return std::wstring(L"LOCAL SERVICE");
    }
    else if (sid::is_nt_network_service_sid(psid)) {
        return std::wstring(L"NETWORK SERVICE");
    }
    else {
        ; // Nothing
    }

    if (!LookupAccountSidW(NULL,
                           psid,
                           NX::string_buffer<wchar_t>(dns_user_name, max_name_size),
                           &dns_user_name_size,
                           NX::string_buffer<wchar_t>(dns_domain_name, max_name_size),
                           &dns_domain_name_size,
                           &sid_name_use)) {
        if (ERROR_NONE_MAPPED == GetLastError()) {
            return std::wstring();
        }
    }

    switch (sid_name_use)
    {
    case SidTypeUser:
    case SidTypeGroup:
        if (dns_domain_name.empty()) {
            name = std::move(dns_user_name);
        }
        else {
            if (NX::utility::iequal(h.dns_host_name(), dns_domain_name)) {
                name = std::move(dns_user_name);
            }
            else {
                std::wstring sam_compatible_name(dns_domain_name + L"\\" + dns_user_name);
                name = win::sam_compatiple_name_to_principle_name(sam_compatible_name);
            }
        }
        break;
    case SidTypeAlias:
        name = dns_domain_name + L"\\" + dns_user_name;
        break;
    case SidTypeWellKnownGroup:
        if (sid::is_everyone_sid(psid)) {
            name = std::move(dns_user_name);
        }
        break;
    case SidTypeDomain:
    case SidTypeDeletedAccount:
    case SidTypeInvalid:
    case SidTypeUnknown:
    case SidTypeComputer:
    case SidTypeLabel:
        break;
    }

    return std::move(name);
}

//
//
//

user_or_group::user_or_group()
{
}

user_or_group::user_or_group(const std::wstring& uid, const std::wstring& uname) : _id(uid), _name(uname)
{
}

user_or_group::~user_or_group()
{
}

user_or_group& user_or_group::operator = (const user_or_group& other)
{
    if (this != &other) {
        _id = other.id();
        _name = other.name();
    }
    return *this;
}

bool user_or_group::operator == (const user_or_group& other) const
{
    return (NX::utility::iequal<wchar_t>(id(), other.id()) && NX::utility::iequal<wchar_t>(name(), other.name()));
}


token::token() : _h(NULL)
{
}

token::token(HANDLE h) : _h(h)
{
}

token::~token()
{
}

user_or_group token::get_user() const
{
    std::vector<unsigned char> buf;
    PTOKEN_USER token_user = NULL;
    unsigned long size = 0;

    GetTokenInformation(_h, TokenUser, NULL, 0, &size);
    if (0 == size) {
        throw NX::exception(WIN32_ERROR_MSG2(GetLastError()));
    }

    size += sizeof(TOKEN_USER);
    buf.resize(size, 0);
    token_user = (PTOKEN_USER)buf.data();

    if (!GetTokenInformation(_h, TokenUser, token_user, size, &size)) {
        throw NX::exception(WIN32_ERROR_MSG2(GetLastError()));
    }

    const std::wstring& user_sid = sid::serialize(token_user->User.Sid);
    const std::wstring& user_name = get_object_name(token_user->User.Sid);
    return win::user_or_group(user_sid, user_name);
}

std::vector<user_or_group> token::get_user_groups() const
{
    std::vector<user_or_group>  groups;
    std::vector<unsigned char> buf;
    PTOKEN_GROUPS token_groups = NULL;
    unsigned long size = 0;

    GetTokenInformation(_h, TokenGroups, NULL, 0, &size);
    if (0 == size) {
        throw NX::exception(WIN32_ERROR_MSG2(GetLastError()));
    }

    size += sizeof(TOKEN_GROUPS);
    buf.resize(size, 0);
    token_groups = (PTOKEN_GROUPS)buf.data();

    if (!GetTokenInformation(_h, TokenGroups, token_groups, size, &size)) {
        throw NX::exception(WIN32_ERROR_MSG2(GetLastError()));
    }

    for (int i = 0; i < (int)token_groups->GroupCount; i++) {
        const std::wstring& group_sid = sid::serialize(token_groups->Groups[i].Sid);
        const std::wstring& group_name = get_object_name(token_groups->Groups[i].Sid);
        if (!group_name.empty()) {
            groups.push_back(user_or_group(group_sid, group_name));
        }
    }

    return std::move(groups);
}

process_token::process_token() : token()
{
}

process_token::process_token(unsigned long process_id) : token()
{
    if (0 != process_id) {
        HANDLE process_handle = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, process_id);
        if (NULL != process_handle) {
            if (!OpenProcessToken(process_handle, TOKEN_QUERY, &_h)) {
                _h = NULL;
            }
            CloseHandle(process_handle);
        }
    }
    else {
        if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &_h)) {
            _h = NULL;
        }
    }
}

process_token::~process_token()
{
}

session_token::session_token() : token()
{
}

session_token::session_token(unsigned long session_id) : token()
{
    if (!WTSQueryUserToken(session_id, &_h)) {
        _h = NULL;
    }
}

session_token::~session_token()
{
}

static const GUID EMPTY_GUID = { 0, 0, 0, {0, 0, 0, 0, 0, 0, 0, 0 } };

NX::win::guid::guid() : GUID(EMPTY_GUID)
{
}

NX::win::guid::guid(const NX::win::guid& other)
{
    memcpy(this, &other, sizeof(GUID));
}

NX::win::guid::guid(NX::win::guid&& other)
{
    memcpy(this, &other, sizeof(GUID));
    other.clear();
}

NX::win::guid::~guid()
{
}

void NX::win::guid::clear()
{
    memset(this, 0, sizeof(GUID));
}

bool NX::win::guid::empty() const
{
    return (0 == memcmp(this, &EMPTY_GUID, sizeof(GUID)));
}

NX::win::guid& NX::win::guid::operator = (const NX::win::guid& other)
{
    if (this != &other) {
        memcpy(this, &other, sizeof(GUID));
    }
    return *this;
}

NX::win::guid& NX::win::guid::operator = (NX::win::guid&& other)
{
    if (this != &other) {
        memcpy(this, &other, sizeof(GUID));
        other.clear();
    }
    return *this;
}

NX::win::guid NX::win::guid::generate()
{
    static const NX::win::host current_host;
    static const process_token current_process(GetCurrentProcessId());

    typedef struct _GUID_SEED {
        FILETIME        timestamp;
        LARGE_INTEGER   counter0;
        LARGE_INTEGER   counter1;
        ULONG           pid;
        ULONG           tid;
    } GUID_SEED;

    GUID_SEED seed;
    std::wstring guid_msg;
    NX::win::guid new_guid;

    memset(&seed, 0, sizeof(seed));
    QueryPerformanceCounter(&seed.counter0);
    GetSystemTimeAsFileTime(&seed.timestamp);
    guid_msg = current_host.fqdn_name().empty() ? current_host.dns_host_name() : current_host.fqdn_name();
    guid_msg += L"\\" + current_process.get_user().name();
    seed.pid = GetCurrentProcessId();
    seed.tid = GetCurrentThreadId();
    QueryPerformanceCounter(&seed.counter1);

    if (!NX::crypto::hmac_md5((const unsigned char*)guid_msg.c_str(), (unsigned long)guid_msg.length() * 2, (const unsigned char*)&seed, (unsigned long)sizeof(seed), (unsigned char*)&new_guid)) {
        new_guid.clear();
    }

    return std::move(new_guid);
}


NX::win::impersonate_object::impersonate_object(HANDLE token_handle) : _impersonated(ImpersonateLoggedOnUser(token_handle) ? true : false)
{
}

NX::win::impersonate_object::~impersonate_object()
{
    if (_impersonated) {
        RevertToSelf();
        _impersonated = false;
    }
}

//
//
//

user_dirs::user_dirs()
{
}

user_dirs::user_dirs(HANDLE htoken)
{
    load(htoken);
}

user_dirs::user_dirs(const user_dirs& other)
    : _inet_cache(other._inet_cache),
    _local_appdata(other._local_appdata),
    _local_appdata_low(other._local_appdata_low),
    _roaming_appdata(other._roaming_appdata),
    _profile(other._profile),
    _desktop(other._desktop),
    _documents(other._documents),
    _cookies(other._cookies),
    _temp(other._temp),
    _programdata(other._programdata)
{
}

user_dirs::user_dirs(user_dirs&& other)
    : _inet_cache(std::move(other._inet_cache)),
    _local_appdata(std::move(other._local_appdata)),
    _local_appdata_low(std::move(other._local_appdata_low)),
    _roaming_appdata(std::move(other._roaming_appdata)),
    _profile(std::move(other._profile)),
    _desktop(std::move(other._desktop)),
    _documents(std::move(other._documents)),
    _cookies(std::move(other._cookies)),
    _temp(std::move(other._temp)),
    _programdata(other._programdata)
{
}

user_dirs::~user_dirs()
{
}

user_dirs& user_dirs::operator = (const user_dirs& other)
{
    if (this != &other) {
        _inet_cache = other._inet_cache;
        _local_appdata = other._local_appdata;
        _local_appdata_low = other._local_appdata_low;
        _roaming_appdata = other._roaming_appdata;
        _profile = other._profile;
        _desktop = other._desktop;
        _documents = other._documents;
        _cookies = other._cookies;
        _temp = other._temp;
        _programdata = other._programdata;
    }
    return *this;
}

user_dirs& user_dirs::operator = (user_dirs&& other)
{
    if (this != &other) {
        _inet_cache = std::move(other._inet_cache);
        _local_appdata = std::move(other._local_appdata);
        _local_appdata_low = std::move(other._local_appdata_low);
        _roaming_appdata = std::move(other._roaming_appdata);
        _profile = std::move(other._profile);
        _desktop = std::move(other._desktop);
        _documents = std::move(other._documents);
        _cookies = std::move(other._cookies);
        _temp = std::move(other._temp);
        _programdata = std::move(other._programdata);
    }
    return *this;
}

void user_dirs::clear()
{
    _inet_cache.clear();
    _local_appdata.clear();
    _local_appdata_low.clear();
    _roaming_appdata.clear();
    _profile.clear();
    _desktop.clear();
    _documents.clear();
    _cookies.clear();
    _temp.clear();
}

void user_dirs::load(HANDLE htoken)
{
    HRESULT hr = S_OK;
    WCHAR*  pwzPath = NULL;

    // Get FOLDERID_InternetCache
    hr = SHGetKnownFolderPath(FOLDERID_InternetCache, KF_FLAG_DEFAULT_PATH | KF_FLAG_CREATE, htoken, &pwzPath);
    if (SUCCEEDED(hr)) {
        _inet_cache = pwzPath;
        CoTaskMemFree(pwzPath);
        pwzPath = NULL;
    }

    // Get FOLDERID_RoamingAppData
    hr = SHGetKnownFolderPath(FOLDERID_RoamingAppData, KF_FLAG_DEFAULT_PATH | KF_FLAG_CREATE, htoken, &pwzPath);
    if (SUCCEEDED(hr)) {
        _roaming_appdata = pwzPath;
        CoTaskMemFree(pwzPath);
        pwzPath = NULL;
    }

    // Get FOLDERID_LocalAppData
    hr = SHGetKnownFolderPath(FOLDERID_LocalAppData, KF_FLAG_DEFAULT_PATH | KF_FLAG_CREATE, htoken, &pwzPath);
    if (SUCCEEDED(hr)) {
        _local_appdata = pwzPath;
        CoTaskMemFree(pwzPath);
        pwzPath = NULL;
    }

    // Make sure %LocalAppData%\Temp exist
    ensure_user_temp_dir(htoken);

    // Get FOLDERID_LocalAppDataLow
    hr = SHGetKnownFolderPath(FOLDERID_LocalAppDataLow, KF_FLAG_DEFAULT_PATH | KF_FLAG_CREATE, htoken, &pwzPath);
    if (SUCCEEDED(hr)) {
        _local_appdata_low = pwzPath;
        CoTaskMemFree(pwzPath);
        pwzPath = NULL;
    }

    // Get FOLDERID_Profile
    hr = SHGetKnownFolderPath(FOLDERID_Profile, KF_FLAG_DEFAULT_PATH | KF_FLAG_CREATE, htoken, &pwzPath);
    if (SUCCEEDED(hr)) {
        _profile = pwzPath;
        CoTaskMemFree(pwzPath);
        pwzPath = NULL;
    }

    // Get FOLDERID_Desktop
    hr = SHGetKnownFolderPath(FOLDERID_Desktop, KF_FLAG_DEFAULT_PATH | KF_FLAG_CREATE, htoken, &pwzPath);
    if (SUCCEEDED(hr)) {
        _desktop = pwzPath;
        CoTaskMemFree(pwzPath);
        pwzPath = NULL;
    }

    // Get FOLDERID_Documents
    hr = SHGetKnownFolderPath(FOLDERID_Documents, KF_FLAG_DEFAULT_PATH | KF_FLAG_CREATE, htoken, &pwzPath);
    if (SUCCEEDED(hr)) {
        _documents = pwzPath;
        CoTaskMemFree(pwzPath);
        pwzPath = NULL;
    }

    // Get FOLDERID_Cookies
    hr = SHGetKnownFolderPath(FOLDERID_Cookies, KF_FLAG_DEFAULT_PATH | KF_FLAG_CREATE, htoken, &pwzPath);
    if (SUCCEEDED(hr)) {
        _cookies = pwzPath;
        CoTaskMemFree(pwzPath);
        pwzPath = NULL;
    }

    // Get FOLDERID_ProgramData
    hr = SHGetKnownFolderPath(FOLDERID_ProgramData, KF_FLAG_DEFAULT_PATH | KF_FLAG_CREATE, htoken, &pwzPath);
    if (SUCCEEDED(hr)) {
        _programdata = pwzPath;
        CoTaskMemFree(pwzPath);
        pwzPath = NULL;
    }

}

bool user_dirs::ensure_user_temp_dir(HANDLE htoken)
{
    if (_local_appdata.empty()) {
        return false;
    }

    const std::wstring temp_dir(_local_appdata + L"\\Temp");
    if (NX::fs::exists(temp_dir)) {
        _temp = temp_dir;
        return true;
    }

    impersonate_object impo(htoken);
    bool result = ::CreateDirectoryW(temp_dir.c_str(), NULL) ? true : false;
    if (!result && ERROR_ALREADY_EXISTS == GetLastError()) {
        result = true;
    }

    if (result) {
        _temp = temp_dir;
    }
    return result;
}
