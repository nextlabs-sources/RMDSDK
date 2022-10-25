

#include <Windows.h>
#include <assert.h>
#include <time.h>
#include <Wtsapi32.h>

#include <string>
#include <iostream>

#include <boost/algorithm/string.hpp>

#include <nudf\eh.hpp>
#include <nudf\debug.hpp>
#include <nudf\filesys.hpp>
#include <nudf\winutil.hpp>
#include <nudf\string.hpp>
#include <nudf\ntapi.hpp>
#include <nudf\dbglog.hpp>
#include <nudf\shared\officelayout.h>


#include "serv.hpp"
#include "global.hpp"

#include "..\SDWL\SDWRmcLib\Winutil\securefile.h"

nxrm_global GLOBAL;

extern rmserv* SERV;

nxrm_global::nxrm_global()
    : _ea_admin(SECURITY_NT_AUTHORITY, SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS, GENERIC_ALL, SUB_CONTAINERS_AND_OBJECTS_INHERIT),
    _ea_everyone_ro(SECURITY_WORLD_SID_AUTHORITY, SECURITY_WORLD_RID, GENERIC_READ, SUB_CONTAINERS_AND_OBJECTS_INHERIT),
    _ea_everyone_rw(SECURITY_WORLD_SID_AUTHORITY, SECURITY_WORLD_RID, GENERIC_READ | GENERIC_WRITE, SUB_CONTAINERS_AND_OBJECTS_INHERIT),
    _filetype_blacklist(L"^[c-zC-Z]{1}:\\\\windows\\\\.*|.*\\.exe$|.*\\.dll$|.*\\.ttf$|.*\\.zip$|.*\\.rar"),
    _resdll(NULL)

{
}

nxrm_global::~nxrm_global()
{
    clear();
}

void nxrm_global::initialize()
{
    init_dirs();
	initkey();

    _file_version = NX::win::file_version(_module_path);
    assert(_file_version.is_application());
}

void nxrm_global::initkey()
{
	SDWLResult res = _key_manager.Load(true);
	if (!res) {
		// If there is no global client key, try to load it from current user's cert store
		res = _key_manager.Load(false, true);
	}
}

void nxrm_global::clear()
{
}

BOOL GrantFullAccess(LPWSTR lpszOwnFile) {
	BOOL bRetval = FALSE;

	PSID pSIDEveryone = NULL;
	PACL pACL = NULL;
	SID_IDENTIFIER_AUTHORITY SIDAuthWorld = SECURITY_WORLD_SID_AUTHORITY;
	EXPLICIT_ACCESS ea;
	const DWORD ACCESS_DIRECTORY_PROTECTED
		= READ_CONTROL
		| GENERIC_READ
		| GENERIC_WRITE
		| GENERIC_EXECUTE
		| SYNCHRONIZE
		| FILE_LIST_DIRECTORY
		| FILE_ADD_FILE
		| FILE_ADD_SUBDIRECTORY
		| FILE_READ_EA
		| FILE_TRAVERSE
		| FILE_DELETE_CHILD
		| FILE_READ_ATTRIBUTES;

	try { 
		  // Create a SID for the Everyone group.
		if (AllocateAndInitializeSid(&SIDAuthWorld, 1, SECURITY_WORLD_RID, 0, 0, 0, 0, 0, 0, 0, &pSIDEveryone) == false)
			return FALSE;

		// Set full access for Everyone.
		ZeroMemory(&ea, sizeof(EXPLICIT_ACCESS));
		ea.grfAccessPermissions = ACCESS_DIRECTORY_PROTECTED;
		ea.grfAccessMode = SET_ACCESS;
		ea.grfInheritance = SUB_CONTAINERS_AND_OBJECTS_INHERIT;
		ea.Trustee.TrusteeForm = TRUSTEE_IS_SID;
		ea.Trustee.TrusteeType = TRUSTEE_IS_WELL_KNOWN_GROUP;
		ea.Trustee.ptstrName = (LPTSTR)pSIDEveryone;

		if (ERROR_SUCCESS != SetEntriesInAcl(1, &ea, NULL, &pACL))
			return FALSE;

		// Try to modify the object's DACL.
		if (ERROR_SUCCESS != SetNamedSecurityInfo(
			lpszOwnFile,                 // name of the object
			SE_FILE_OBJECT,              // type of object
			DACL_SECURITY_INFORMATION | UNPROTECTED_DACL_SECURITY_INFORMATION,   // change only the object's DACL
			NULL, NULL,                  // do not change owner or group
			pACL,                        // DACL specified
			NULL)                       // do not change SACL
			)
			return FALSE;

		TreeSetNamedSecurityInfo(lpszOwnFile, SE_FILE_OBJECT, DACL_SECURITY_INFORMATION, NULL, NULL, pACL, NULL, TREE_SEC_INFO_SET, NULL, ProgressInvokeNever, NULL);
	}
	catch (...) {}

	if (pSIDEveryone)
		FreeSid(pSIDEveryone);
	if (pACL)
		LocalFree(pACL);

	return TRUE;
}

void nxrm_global::init_dirs()
{
    NX::fs::module_path mod_path(NULL);
    NX::fs::dos_filepath parent_dir(mod_path.file_dir());

    _module_path = mod_path.path();
    _product_dir = parent_dir.file_dir();
    _bin_dir = parent_dir.path();
#ifdef _AMD64_
    _bin32_dir = _bin_dir + L"\\x86";
#endif
    _working_dir = _bin_dir;
    _config_dir = _product_dir + L"\\conf";
    _profiles_dir = _product_dir + L"\\profiles";

	LOGDEBUG(NX::string_formater(L"nxrm_global::init_dirs:  _profiles_dir: %s", _profiles_dir.c_str()));
    CreateDirectoryW(_config_dir.c_str(), NULL);
    CreateDirectoryW(_profiles_dir.c_str(), NULL);

	if (SERV->get_service_conf().get_no_vhd())
	{
		// change profiles dir's security to allow anyone to read/write/create directory
		GrantFullAccess((LPWSTR)_profiles_dir.c_str());
	}
}

void nxrm_global::init_modules()
{
    const std::wstring resdll_file(_bin_dir + L"\\nxrmres.dll");
    _resdll = ::LoadLibraryW(resdll_file.c_str());
    if (NULL == _resdll) {
        LOGWARNING(NX::string_formater(L"Fail to load resource DLL (%s)", resdll_file.c_str()));
    }
}

void nxrm_global::init_office_customized_ui_settings()
{
    std::wstring custom_ui_dir = _config_dir + L"\\custom_ui";

    CreateDirectoryW(custom_ui_dir.c_str(), NULL);

    (VOID)generate_file(std::wstring(custom_ui_dir + L"\\office_layout_default.xml"), OFFICE_LAYOUT_XML, false);
    (VOID)generate_file(std::wstring(custom_ui_dir + L"\\office_layout_word14.xml"), WORD_LAYOUT_XML_14, false);
    (VOID)generate_file(std::wstring(custom_ui_dir + L"\\office_layout_excel14.xml"), EXCEL_LAYOUT_XML_14, false);
    (VOID)generate_file(std::wstring(custom_ui_dir + L"\\office_layout_powerpnt14.xml"), POWERPNT_LAYOUT_XML_14, false);
    (VOID)generate_file(std::wstring(custom_ui_dir + L"\\office_layout_word15.xml"), WORD_LAYOUT_XML_15, false);
    (VOID)generate_file(std::wstring(custom_ui_dir + L"\\office_layout_excel15.xml"), EXCEL_LAYOUT_XML_15, false);
    (VOID)generate_file(std::wstring(custom_ui_dir + L"\\office_layout_powerpnt15.xml"), POWERPNT_LAYOUT_XML_15, false);
}

std::wstring nxrm_global::get_temp_file_name(const std::wstring& dir)
{
    std::wstring temp_file;
    if (0 == GetTempFileNameW(dir.c_str(), L"RMC", 0, NX::string_buffer<wchar_t>(temp_file, MAX_PATH))) {
        temp_file.clear();
    }
    else {
        ::DeleteFileW(temp_file.c_str());
    }
    return std::move(temp_file);
}

std::wstring nxrm_global::get_temp_folder_name(const std::wstring& dir, LPSECURITY_ATTRIBUTES sa)
{
    std::wstring temp_folder;

    srand((unsigned int)time(NULL));
    while (temp_folder.empty()) {
        const std::wstring random_name = NX::string_formater(L"%08X", (unsigned int)rand());
        std::wstring available_path = dir;
        if (!boost::algorithm::iends_with(dir, L"\\")) {
            available_path += L"\\";
        }
        available_path += L"RMC";
        available_path += random_name;

        if (::CreateDirectoryW(available_path.c_str(), sa)) {
            temp_folder = available_path;
        }

        DWORD last_error = GetLastError();
        if (ERROR_ALREADY_EXISTS != last_error) {
            break;
        }
    }

    return std::move(temp_folder);
}

bool nxrm_global::generate_file(const std::wstring& file, const std::string& content, bool force)
{
	if (_key_manager.GetClientKey().empty() == false)
	{
		NX::RmSecureFile sf(file, _key_manager.GetClientKey());
		SDWLResult res = sf.Write((const UCHAR*)content.c_str(), (ULONG)content.length());
		if (res)
			return true;
	}
	
	HANDLE h = INVALID_HANDLE_VALUE;
    bool result = false;

    do {

        h = ::CreateFileW(file.c_str(), GENERIC_READ | GENERIC_WRITE, 0, NULL, force ? CREATE_ALWAYS : CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
        if (INVALID_HANDLE_VALUE == h) {
            break;
        }

        DWORD dwWritten = 0;
        if (!WriteFile(h, content.c_str(), (DWORD)content.length(), &dwWritten, NULL)) {
            CloseHandle(h);
            h = INVALID_HANDLE_VALUE;
            ::DeleteFileW(file.c_str());
            break;
        }

        result = true;

    } while (FALSE);

    if (INVALID_HANDLE_VALUE != h) {
        CloseHandle(h);
        h = INVALID_HANDLE_VALUE;
    }

    return result;
}


bool nxrm_global::nt_generate_file(const std::wstring& file, const std::string& content, bool force)
{
	if (_key_manager.GetClientKey().empty() == false)
	{
		NX::RmSecureFile sf(file, _key_manager.GetClientKey());
		SDWLResult res = sf.Write((const UCHAR*)content.c_str(), (ULONG)content.length());
		if (res)
			return true;
	}
	
	HANDLE h = NULL;
    bool result = false;
	DWORD ret = 0;

    do {
		if (SERV->get_service_conf().get_no_vhd())
		{
			h = NT::CreateFile(file.c_str(), FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, FILE_ATTRIBUTE_NORMAL, 0, CREATE_ALWAYS, 0, NULL);
		}
		else
		{
			h = NT::CreateFile(file.c_str(), GENERIC_READ | GENERIC_WRITE, NULL, FILE_ATTRIBUTE_NORMAL, 0, force ? FILE_OVERWRITE_IF : FILE_CREATE, 0, NULL);
		}
		if (NULL == h) {
			ret = GetLastError();
			//if (ret != ERROR_ALREADY_EXISTS) // 183
			{
				LOGERROR(NX::string_formater(L"CreateFile error: %d, %s", ret, file.c_str()));
				break;
			}
        }

		std::wstring _sfile = L"abc";
		std::vector<unsigned char> key;
		NX::RmSecureFile sf(_sfile, key);

        DWORD dwWritten = 0;
        LARGE_INTEGER Offset = { 0, 0 };
        if (!NT::WriteFile(h, (PVOID)content.c_str(), (DWORD)content.length(), &dwWritten, &Offset)) {
            CloseHandle(h);
            h = NULL;
            NT::DeleteFile(file.c_str());
            break;
        }

        result = true;

    } while (FALSE);

    if (NULL != h) {
        NT::CloseHandle(h);
        h = NULL;
    }

    return result;
}

bool nxrm_global::load_file(const std::wstring& file, std::string& content)
{
	if (_key_manager.GetClientKey().empty() == false)
	{
		NX::RmSecureFile sf(file, _key_manager.GetClientKey());
		SDWLResult res = sf.Read(content);
		if (res)
			return true;
	}
	
	HANDLE h = INVALID_HANDLE_VALUE;
    bool result = false;

    do {

        h = ::CreateFileW(file.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        if (INVALID_HANDLE_VALUE == h) {
            break;
        }

        DWORD dwSize = GetFileSize(h, NULL);
        if (INVALID_FILE_SIZE == dwSize) {
            break;
        }

        if (0 == dwSize) {
            result = true;
            break;
        }

        content.resize(dwSize, 0);

        DWORD dwWRead = 0;
        if (!ReadFile(h, (LPVOID)content.c_str(), dwSize, &dwWRead, NULL)) {
            break;
        }

        result = true;

    } while (FALSE);

    if (INVALID_HANDLE_VALUE != h) {
        CloseHandle(h);
        h = INVALID_HANDLE_VALUE;
    }

    return result;
}

bool nxrm_global::nt_load_file(const std::wstring& file, std::string& content)
{
	if (_key_manager.GetClientKey().empty() == false)
	{
		NX::RmSecureFile sf(file, _key_manager.GetClientKey());
		SDWLResult res = sf.Read(content);
		if (res)
			return true;
	}

	// else, it is the old format (non-encrypted content)
    HANDLE h = NULL;
    bool result = false;

    do {


        h = NT::CreateFile(file.c_str(), GENERIC_READ, NULL, FILE_ATTRIBUTE_NORMAL, FILE_SHARE_READ | FILE_SHARE_WRITE, FILE_OPEN, 0, NULL);
        if (INVALID_HANDLE_VALUE == h) {
			DWORD err = GetLastError();
			if (GetLastError() != ERROR_ALREADY_EXISTS)
				break;
        }

        LARGE_INTEGER liSize = { 0, 0 };
        if(!NT::GetFileSize(h, &liSize)) {
            break;
        }
        if (liSize.HighPart != 0) {
            SetLastError(ERROR_BUFFER_OVERFLOW);
            break;
        }

        if (0 == liSize.LowPart) {
            result = true;
            break;
        }

        content.resize(liSize.LowPart, 0);

        DWORD dwRead = 0;
        LARGE_INTEGER Offset = { 0, 0 };
        if (!NT::ReadFile(h, (PVOID)content.c_str(), liSize.LowPart, &dwRead, &Offset)) {
            break;
        }

        result = true;

    } while (FALSE);

    if (NULL != h) {
        NT::CloseHandle(h);
        h = INVALID_HANDLE_VALUE;
    }

    return result;
}

process_record nxrm_global::safe_find_process(unsigned long process_id)
{
    process_record proc_record = get_process_cache().find(process_id);
    if (proc_record.empty()) {
        if (NULL != SERV) { // SERV should NEVER be NULL
            const std::wstring& process_path = SERV->get_coreserv().drvctl_query_process_info(process_id);
            if (process_path.empty()) {
                LOGERROR(NX::string_formater(L"Fail to get process (id: %d) information from driver", process_id));
            }
            else {
                unsigned long session_id = 0;
                if (!ProcessIdToSessionId(process_id, &session_id)) {
                    session_id = 0;
                }
                proc_record = process_record(process_id, session_id, process_path, 0);
                if (!proc_record.empty()) {
                    GLOBAL.get_process_cache().insert(proc_record);
                }
            }
        }
    }
    return proc_record;
}