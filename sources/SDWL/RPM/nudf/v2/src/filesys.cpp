

#include <Windows.h>
#include <assert.h>
#include <time.h>
#include <Psapi.h>

#include <boost\algorithm\string.hpp>

#include <nudf\eh.hpp>
#include <nudf\debug.hpp>
#include <nudf\conversion.hpp>
#include <nudf\crypto_sha.hpp>
#include <nudf\string.hpp>
#include <nudf\crc.hpp>
#include <nudf\filesys.hpp>
#include <shlwapi.h>

using namespace NX;
using namespace NX::fs;

drive::drive() : _letter(0), _type(DRIVE_UNKNOWN), _type_name(L"Unknown")
{
}

drive::drive(wchar_t c) : _letter((wchar_t)toupper(c)), _type(get_drive_type(_letter)), _type_name(type_to_name(_type))
{
    normalize();
}

drive::~drive()
{
}

/*
#define DRIVE_UNKNOWN     0
#define DRIVE_NO_ROOT_DIR 1
#define DRIVE_REMOVABLE   2
#define DRIVE_FIXED       3
#define DRIVE_REMOTE      4
#define DRIVE_CDROM       5
#define DRIVE_RAMDISK     6
*/
bool drive::is_valid() const
{
    return (_type > DRIVE_NO_ROOT_DIR);
}

bool drive::is_removable() const
{
    return (_type == DRIVE_REMOVABLE);
}

bool drive::is_fixed() const
{
    return (_type == DRIVE_FIXED);
}

bool drive::is_remote() const
{
    return (_type == DRIVE_REMOTE);
}

bool drive::is_cdrom() const
{
    return (_type == DRIVE_CDROM);
}

bool drive::is_ramdisk() const
{
    return (_type == DRIVE_RAMDISK);
}

void drive::clear()
{
    _type = DRIVE_UNKNOWN;
    _dos_name.clear();
    _nt_name.clear();
    _type_name = L"Unknown";
}

drive& drive::operator = (const drive& other)
{
    if (this != &other) {
        _dos_name = other.dos_name();
        _nt_name = other.nt_name();
        _type = other.type();
    }
    return *this;
}

unsigned int drive::get_drive_type(const wchar_t drive)
{
    const wchar_t drive_path[4] = { drive, L':', L'\\', 0 };
    return GetDriveTypeW(drive_path);
}

std::wstring drive::type_to_name(unsigned int type)
{
    std::wstring name(L"Unknown");

    switch (type)
    {
    case DRIVE_FIXED:
        name = L"Fixed Drive";
        break;
    case DRIVE_REMOVABLE:
        name = L"Removable Drive";
        break;
    case DRIVE_REMOTE:
        name = L"Remote Drive";
        break;
    case DRIVE_CDROM:
        name = L"CD/DVD";
        break;
    case DRIVE_RAMDISK:
        name = L"Ram Disk";
        break;
    case DRIVE_NO_ROOT_DIR:
    case DRIVE_UNKNOWN:
    default:
        break;
    }

    return std::move(name);
}

drive::space drive::get_space() const
{
    WCHAR wzDrive[4] = { _letter, L':', L'\\', 0 };
    ULARGE_INTEGER li_total = { 0, 0 };
    ULARGE_INTEGER li_total_free = { 0, 0 };
    ULARGE_INTEGER li_available_free = { 0, 0 };
    UINT old_state = SetErrorMode(SEM_FAILCRITICALERRORS);
    GetDiskFreeSpaceExW(wzDrive, &li_available_free, &li_total, &li_total_free);
    SetErrorMode(old_state);
    return drive::space(li_total.QuadPart, li_total_free.QuadPart, li_available_free.QuadPart);
}

void drive::normalize()
{
    if (!is_valid()) {
        return;
    }

    const wchar_t drive_name[4] = { _letter, L':', L'\\', 0 };

    if (is_remote()) {
        UNIVERSAL_NAME_INFOW* puni = NULL;
        std::vector<unsigned char> buf;
        buf.resize(2);
        unsigned long bufsize = 2;
        //Call WNetGetUniversalName using UNIVERSAL_NAME_INFO_LEVEL option
        if (WNetGetUniversalNameW(drive_name, UNIVERSAL_NAME_INFO_LEVEL, (LPVOID)buf.data(), &bufsize) == ERROR_MORE_DATA) {
            // now we have the size required to hold the UNC path
            buf.resize(bufsize + 1, 0);
            puni = (UNIVERSAL_NAME_INFOW*)buf.data();
            if (WNetGetUniversalNameW(drive_name, UNIVERSAL_NAME_INFO_LEVEL, (LPVOID)puni, &bufsize) == NO_ERROR) {
                _dos_name = puni->lpUniversalName;
                _nt_name = L"\\Device\\Mup";
                _nt_name += _dos_name.substr(1);
            }
        }
    }
    else {
        _dos_name = std::wstring(drive_name, drive_name + 2);
        if (!QueryDosDeviceW(_dos_name.c_str(), NX::string_buffer<wchar_t>(_nt_name, 1024), 1024)) {
            _nt_name.clear();
        }
    }
}


//
//   class fs::filename
//

filename::filename()
{
}

filename::filename(const std::wstring& s) : _s(s)
{
}

filename::filename(const std::string& s, bool utf8) : _s(utf8 ? NX::conversion::utf8_to_utf16(s) : NX::conversion::ansi_to_utf16(s))
{
}

filename::~filename()
{
}

std::wstring filename::name_part() const
{
    auto pos = _s.find_last_of(L'.');
    return (pos == std::wstring::npos) ? _s : _s.substr(0, pos);
}

std::wstring filename::extension() const
{
    auto pos = _s.find_last_of(L'.');
    return (pos == std::wstring::npos) ? std::wstring(L"") : _s.substr(pos);
}

filename& filename::operator = (const filename& other)
{
    if (this != &other) {
        _s = other.full_name();
    }
    return *this;
}

std::wstring filename::encode(const std::wstring& name)
{
    std::wstring s;

    for (int i = 0; i < (int)name.length(); i++) {
        switch (name[i])
        {
        case L'<':
        case L'>':
        case L':':
        case L'\"':
        case L'/':
        case L'\\':
        case L'|':
        case L'?':
        case L'*':
            s += L"%";
            s += NX::conversion::to_wstring((UCHAR)name[i]);
            break;
        default:
            s += name[i];
            break;
        }
    }

    return std::move(s);
}

std::wstring filename::decode(const std::wstring& encoded_name)
{
    std::wstring s;
    const wchar_t* p = encoded_name.c_str();
    const wchar_t* const end_pos = p + encoded_name.length();

    while (p != end_pos && 0 != *p) {

        const wchar_t ch = *p;

        switch (ch)
        {
        case L'%':
            if (end_pos == (p + 1) || end_pos == (p + 2)) {
                // doesn't have twp extra characters, this must not a escaping character
                s += ch;
                break;
            }
            else {
                if (NX::utility::is_hex<wchar_t>(*(p + 1)) || NX::utility::is_hex<wchar_t>(*(p + 2))) {
                    s += ch;
                }
                else {

                    unsigned char ch = NX::utility::hex_to_uchar<wchar_t>(*(p + 1));
                    ch <<= 4;
                    ch |= NX::utility::hex_to_uchar(*(p + 2));
                    s += ((wchar_t)ch);
                    p += 2;
                }
            }
            break;
        default:
            s += ch;
            break;
        }
        // Move to next
        ++p;
    }

    return std::move(s);
}


//
//  class fs::nt_filepath
//

nt_filepath::nt_filepath()
{
}

nt_filepath::nt_filepath(const std::wstring& s) : filepath(normalize(s))
{
}

nt_filepath::nt_filepath(HANDLE h) : filepath(handle_to_nt_path(h))
{
}

nt_filepath::~nt_filepath()
{
}

nt_filepath& nt_filepath::operator = (const nt_filepath& other)
{
    if (this != &other) {
        set_path(other.path());
    }
    return *this;
}

std::wstring nt_filepath::normalize(const std::wstring& s)
{
    std::wstring input_path;
    std::wstring final_path;

    auto pos = s.find_last_of(L'\\');
    if (pos == std::wstring::npos) {
        // name only, use current directory
        GetCurrentDirectoryW(MAX_PATH, NX::string_buffer<wchar_t>(input_path, MAX_PATH));
        input_path += L"\\";
        input_path += s;
    }
    else {
        input_path = s;
    }

    if (boost::algorithm::starts_with(s, L"\\??\\") || boost::algorithm::starts_with(s, L"\\\\?\\")) {
        // This is a DOS Path:
        //      "\\?\D:\test.docx"
        //      "\\?\UNC\nextlabs.com\share\data\test.zip"
        //      "\??\D:\test.docx"
        //      "\??\UNC\nextlabs.com\share\data\test.zip"
        const std::wstring& temp_path = s.substr(4);
        if (fs::is_dos_path(temp_path)) {
            // "D:\test.docx"
            fs::drive drv(temp_path[0]);
            if (drv.is_valid()) {
                // "D:\test.docx" ==> "\Device\harddiskVolume1\test.docx"
                final_path = drv.nt_name();
                final_path += temp_path.substr(2);
            }
        }
        else if (boost::algorithm::istarts_with(temp_path, L"UNC\\")) {
            // "UNC\nextlabs.com\share\data\test.zip"  ==>  "\Device\Mup\nextlabs.com\share\data\test.zip"
            final_path = L"\\Device\\Mup";
            final_path += temp_path.substr(3);
        }
        else {
            ; // Unknown
        }
    }
    else {

        // Local Drive Dos Path:    D:\test.docx
        // Remote Drive Dos Path:   S:\test.zip  (S: drive is a mapped drive: ==> "\\nextlabs.com\share\data")
        // UNC Path:                \\nextlabs.com\share\data\test.zip
        // Local Drive NT Path:     \Device\HarddiskVolume1\test.docx
        // Remote Drive NT Path:    \Device\Mup\nextlabs.com\share\data\test.zip

        if (fs::is_dos_path(s)) {
            // Dos path
            fs::drive drv(s[0]);
            if (drv.is_valid()) {
                final_path = drv.nt_name();
                final_path += s.substr(2);
            }
        }
        else if (fs::is_unc_path(s)) {
            final_path = L"\\Device\\Mup";
            final_path += s.substr(1);
        }
        else if (fs::is_nt_path(s)) {
            final_path = s;
        }
        else {
            ; // Invalid path
        }
    }

    return std::move(final_path);
}



//
//  class fs::dos_filepath
//
dos_filepath::dos_filepath()
{
}

dos_filepath::dos_filepath(HANDLE h) : filepath(normalize(handle_to_nt_path(h)))
{
}

dos_filepath::dos_filepath(const std::wstring& s) : filepath(normalize(s))
{
}

dos_filepath::~dos_filepath()
{
}

dos_filepath& dos_filepath::operator = (const dos_filepath& other)
{
    if (this != &other) {
        set_path(other.path());
    }
    return *this;
}

dos_filepath dos_filepath::get_current_directory()
{
    std::wstring s;
    GetCurrentDirectoryW(MAX_PATH, NX::string_buffer<wchar_t>(s, MAX_PATH));
    return dos_filepath(s);
}

std::wstring dos_filepath::normalize(const std::wstring& s)
{
    std::wstring input_path;
    std::wstring final_path;

    auto pos = s.find_last_of(L'\\');
    if (pos == std::wstring::npos) {
        // name only, use current directory
        GetCurrentDirectoryW(MAX_PATH, NX::string_buffer<wchar_t>(input_path, MAX_PATH));
        input_path += L"\\";
        input_path += s;
    }
    else {
        input_path = s;
    }

    if (boost::algorithm::starts_with(s, L"\\??\\") || boost::algorithm::starts_with(s, L"\\\\?\\")) {
        // This is a DOS Path:
        //      "\\?\D:\test.docx"
        //      "\\?\UNC\nextlabs.com\share\data\test.zip"
        //      "\??\D:\test.docx"
        //      "\??\UNC\nextlabs.com\share\data\test.zip"
        const std::wstring& temp_path = s.substr(4);
        if (fs::is_dos_path(temp_path)) {
            // "D:\test.docx"
            fs::drive drv(temp_path[0]);
            if (drv.is_valid()) {
                if (drv.is_remote()) {
                    final_path = drv.dos_name();
                    final_path += temp_path.substr(2);
                }
                else {
                    final_path = temp_path;
                }
            }
        }
        else if (boost::algorithm::istarts_with(temp_path, L"UNC\\")) {
            // "UNC\nextlabs.com\share\data\test.zip"  ==>  "\\nextlabs.com\share\data\test.zip"
            final_path = L"\\";
            final_path += temp_path.substr(3);
        }
        else {
            ; // Unknown
        }
    }
    else {

        // Local Drive Dos Path:    D:\test.docx
        // Remote Drive Dos Path:   S:\test.zip  (S: drive is a mapped drive: ==> "\\nextlabs.com\share\data")
        // UNC Path:                \\nextlabs.com\share\data\test.zip
        // Local Drive NT Path:     \Device\HarddiskVolume1\test.docx
        // Remote Drive NT Path:    \Device\Mup\nextlabs.com\share\data\test.zip

        if (fs::is_dos_path(s)) {
            // Dos path
            fs::drive drv(s[0]);
            if (drv.is_valid()) {
                if (drv.is_remote()) {
                    final_path = drv.dos_name();
                    final_path += s.substr(2);
                }
                else {
                    final_path = s;
                }
            }
        }
        else if (fs::is_unc_path(s)) {
            final_path = s;
        }
        else if (fs::is_nt_path(s)) {
            final_path = nt_path_to_dos_path(s);
        }
        else if (fs::is_systemroot_path(s)) {
			final_path = change_systemroot_path_to_dos_path(s);
		}
		else {
			; // Invalid path
		}
    }

    return std::move(final_path);
}


//
//  class fs::dos_filepath
//
dos_fullfilepath::dos_fullfilepath()
{
}

dos_fullfilepath::dos_fullfilepath(HANDLE h) : filepath(normalize(handle_to_nt_path(h)))
{
}

dos_fullfilepath::dos_fullfilepath(const std::wstring& s, bool _lowcase) : filepath(case_normalize(s, _lowcase))
{
}

dos_fullfilepath::~dos_fullfilepath()
{
}

dos_fullfilepath& dos_fullfilepath::operator = (const dos_fullfilepath& other)
{
	if (this != &other) {
		set_path(other.path());
	}
	return *this;
}

dos_fullfilepath dos_fullfilepath::get_current_directory()
{
	std::wstring s;
	GetCurrentDirectoryW(MAX_PATH, NX::string_buffer<wchar_t>(s, MAX_PATH));
	return dos_fullfilepath(s);
}

std::wstring dos_fullfilepath::case_normalize(const std::wstring& s, bool _lowcase)
{
	lowcase = _lowcase;
	return dos_fullfilepath::normalize(s);
}

std::wstring dos_fullfilepath::normalize(const std::wstring& s)
{
	std::wstring input_path(s);
	std::wstring final_path;

	size_t find = input_path.find(L"/");
	if (find != std::wstring::npos) {
		std::replace(input_path.begin(), input_path.end(), '/', '\\');
	}

	// If input path does not contain any backslash, we assume that it is a
	// path relative to the current directory.  However, if input path
	// contains only a drive designator (e.g. "D:"), we assume that it refers
	// to the root directory of that drive instead of the current directory of
	// that drive.
	//
	// Right now we don't handle cases like "D:file.txt" or "SubDir\file.txt".
	// We can add support for them later.
	auto pos = input_path.find_last_of(L'\\');
	if (pos == std::wstring::npos && !fs::is_dos_drive_only_path(input_path)) {
		// name only, use current directory
		GetCurrentDirectoryW(MAX_PATH, NX::string_buffer<wchar_t>(input_path, MAX_PATH));
		input_path += L"\\";
		input_path += s;
	}

	auto found = input_path.rfind(L"\\");
    if (found == input_path.size() - 1)
    {
        input_path.replace(found, 1, L"");
        if (fs::is_dos_drive_only_path(input_path))
            input_path += L"\\";
    }
    else if (fs::is_dos_drive_only_path(input_path))
        input_path += L"\\";

	bool bIsInputGlobalPath = false;
	if (fs::is_global_path(input_path)) {
		bIsInputGlobalPath = true;
	}

	// convert to long path
	long     length = 0;
	TCHAR*   buffer = NULL;
	// First obtain the size needed by passing NULL and 0.
	length = GetLongPathName(input_path.c_str(), NULL, 0);
	if (length > (long)(input_path.size() + 1))
	{
		buffer = new TCHAR[length];
		length = GetLongPathName(input_path.c_str(), buffer, length);
		if (length > 0)
			input_path = buffer;
		delete[] buffer;
	}


	// for junction folder or sybolic link file, GetFinalPathNameByHandleW to get exact file path
	bool input_has_nxl = false;
	NX::fs::filename input_filename(input_path);
	if (input_filename.extension() == L".nxl")
		input_has_nxl = true;

	std::wstring global_input_path = input_path;
	if (fs::is_dos_path(input_path))
	{
		// if file path is too long > 260, convert it to global dos path
		global_input_path = L"\\\\?\\" + input_path;
	}

	TCHAR  existingTarget[2048];
	HANDLE hFile = ::CreateFileW(global_input_path.c_str(), 0, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (INVALID_HANDLE_VALUE == hFile)
	{
		// in case of directory
		hFile = ::CreateFileW(global_input_path.c_str(), 0, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_BACKUP_SEMANTICS, NULL);
	}

	if (INVALID_HANDLE_VALUE != hFile)
	{
		// following call will redirect to exact NXL or normal file if current process is trusted/untrusted
		// we shall append or remove .NXL back
		DWORD dwret = ::GetFinalPathNameByHandleW(hFile, existingTarget, 2048, FILE_NAME_OPENED);

		if (dwret == 0)
		{
			DWORD ret = GetLastError();
			if (ret == ERROR_PATH_NOT_FOUND)
			{
				// not a dos path, let's try with GUID path
				dwret = ::GetFinalPathNameByHandleW(hFile, existingTarget, 2048, FILE_NAME_OPENED | VOLUME_NAME_GUID);
			}
		}

		if (dwret < 2048)
		{
			CloseHandle(hFile);
			input_path = existingTarget;
		}
		else
		{
			//
			// too long file path > 2048
			// allocate more buffer 
			TCHAR  *_2existingTarget = new WCHAR[dwret + 1];
			DWORD _2dwret = ::GetFinalPathNameByHandleW(hFile, _2existingTarget, (dwret + 1), FILE_NAME_OPENED);
			CloseHandle(hFile);

			if (_2dwret < (dwret + 1))
			{
				input_path = _2existingTarget;
			}
			else
			{
				// error, do nothing
			}
			delete _2existingTarget;
		}
	}

	bool output_has_nxl = false;
	NX::fs::filename output_filename(input_path);
	if (output_filename.extension() == L".nxl")
		output_has_nxl = true;

	if (input_has_nxl && output_has_nxl == false) // append NXL, trusted process want to access a NXL file
		input_path += L".nxl";

	if (input_has_nxl == false && output_has_nxl && input_path.size() > 4) // remove .nxl, untrsuted process want to access a NXL file
		input_path = input_path.substr(0, input_path.size() - 4);

	// Remove any trailing backslash in the path.  However, don't remove it if
	// the path refers to the root directory of a drive, because the trailing
	// backslash is required in that case.
	found = input_path.rfind(L"\\");
	if (found == input_path.size() - 1 && input_path[found - 1] != ':')
		input_path.replace(found, 1, L"");

	if (boost::algorithm::starts_with(input_path, L"\\??\\") || boost::algorithm::starts_with(input_path, L"\\\\?\\")) {

		if (bIsInputGlobalPath) {
			final_path = input_path;
		}
		else {
			// This is a DOS Path:
			//      "\\?\D:\" (must have trailing backslash for root directory)
			//      "\\?\D:\myDir"
			//      "\\?\D:\test.docx"
			//      "\\?\UNC\nextlabs.com\share\data\test.zip"
			//      "\??\D:\" (must have trailing backslash for root directory)
			//      "\??\D:\myDir"
			//      "\??\D:\test.docx"
			//      "\??\UNC\nextlabs.com\share\data\test.zip"
			const std::wstring& temp_path = input_path.substr(4);
			if (fs::is_dos_path(temp_path)) {
				// "D:\test.docx"
				fs::drive drv(temp_path[0]);
				if (drv.is_valid()) {
					if (drv.is_remote()) {
						final_path = drv.dos_name();
						final_path += temp_path.substr(2);
					}
					else {
						final_path = temp_path;
					}
				}
			}
			else if (boost::algorithm::istarts_with(temp_path, L"UNC\\")) {
				// "UNC\nextlabs.com\share\data\test.zip"  ==>  "\\nextlabs.com\share\data\test.zip"
				final_path = L"\\";
				final_path += temp_path.substr(3);
			}
			else {
				/* GUID path \\?\Volume{fa7b7142-b702-11e8-86fd-000c29c23f2d}\ */
				final_path = L"\\\\?\\";
				final_path += temp_path;
			}
		}
	}
	else {

		// Local Drive Dos Path:    D:\test.docx
		// Remote Drive Dos Path:   S:\test.zip  (S: drive is a mapped drive: ==> "\\nextlabs.com\share\data")
		// UNC Path:                \\nextlabs.com\share\data\test.zip
		// Local Drive NT Path:     \Device\HarddiskVolume1\test.docx
		// Remote Drive NT Path:    \Device\Mup\nextlabs.com\share\data\test.zip

		if (fs::is_dos_path(input_path)) {
			// Dos path
			fs::drive drv(input_path[0]);
			if (drv.is_valid()) {
				if (drv.is_remote()) {
					final_path = drv.dos_name();
					final_path += input_path.substr(2);
				}
				else {
					final_path = input_path;
				}
			}
		}
		else if (fs::is_unc_path(input_path)) {
			final_path = input_path;
		}
		else if (fs::is_nt_path(input_path)) {
			final_path = nt_path_to_dos_path(input_path);
		}
		else {
			; // Invalid path
		}
	}

	return final_path;
}

std::wstring dos_fullfilepath::global_dos_path()
{
	if (fs::is_dos_path(path()))
	{
		return L"\\\\?\\" + path();
	}

	return path();
}


//
//   class fd::module_path
//

module_path::module_path() : dos_filepath()
{
}

module_path::module_path(HMODULE h) : dos_filepath()
{
    std::wstring s;
    if (0 != GetModuleFileNameW(h, NX::string_buffer<wchar_t>(s, MAX_PATH), MAX_PATH)) {
        set_path(dos_filepath::normalize(s));
    }
}

module_path::~module_path()
{
}

module_path& module_path::operator = (const module_path& other)
{
    if (this != &other) {
        dos_filepath::operator=(other);
    }
    return *this;
}


//
// fs
//
bool fs::exists(const std::wstring& file)
{
    WIN32_FIND_DATAW fd = { 0 };
    HANDLE h = FindFirstFileW(file.c_str(), &fd);
    if (h == INVALID_HANDLE_VALUE) {
        return false;
    }
    FindClose(h); h = INVALID_HANDLE_VALUE;
    return true;
}

bool fs::has_file_attributes(unsigned long attributes, unsigned long flags)
{
    return ((INVALID_FILE_ATTRIBUTES != attributes) && (flags == (attributes & flags)));
}

bool fs::remove_file_attributes(const std::wstring& file, unsigned long flags)
{
    DWORD attributes = GetFileAttributesW(file.c_str());
    if (INVALID_FILE_ATTRIBUTES == attributes) {
        return false;
    }
    if (0 != (attributes & flags)) {
        attributes &= (~flags);
        if (0 == attributes) {
            attributes = FILE_ATTRIBUTE_NORMAL;
        }
        return SetFileAttributesW(file.c_str(), attributes) ? true : false;
    }

    return true;
}

bool fs::is_readonly(const std::wstring& file)
{
    return has_file_attributes(GetFileAttributesW(file.c_str()), FILE_ATTRIBUTE_READONLY);
}

bool fs::is_nt_path(const std::wstring& s)
{
    return boost::algorithm::istarts_with(s, L"\\Device\\");
}

bool fs::is_remote_nt_path(const std::wstring& s)
{
    return boost::algorithm::istarts_with(s, L"\\Device\\Mup\\");
}

bool fs::is_unc_path(const std::wstring& s)
{
    return (L'\\' == s[0] && L'\\' == s[1] && L'\\' != s[2]);
}

bool fs::is_dos_drive_only_path(const std::wstring& s)
{
    return (s.size() == 2 && NX::utility::is_alphabet<wchar_t>(s[0]) && L':' == s[1]);
}

bool fs::is_dos_path(const std::wstring& s)
{
    return (s.size() > 2 && NX::utility::is_alphabet<wchar_t>(s[0]) && L':' == s[1] && L'\\' == s[2]);
}

bool fs::is_global_path(const std::wstring& s)
{
    return (boost::algorithm::starts_with(s, L"\\??\\") || boost::algorithm::starts_with(s, L"\\\\?\\"));
}

bool fs::is_global_dos_path(const std::wstring& s)
{
    return (is_global_path(s) && NX::utility::is_alphabet<wchar_t>(s[4]) && L':' == s[5] && L'\\' == s[6]);
}

bool fs::is_global_unc_path(const std::wstring& s)
{
    return (is_global_path(s)
            && NX::utility::iequal<wchar_t>(s[4], L'U')
            && NX::utility::iequal<wchar_t>(s[5], L'N')
            && NX::utility::iequal<wchar_t>(s[6], L'C')
            && L'\\' == s[7]
            && L'\\' != s[8]);
}

bool fs::is_systemroot_path(const std::wstring& s)
{
	return boost::algorithm::istarts_with(s, L"\\SystemRoot\\");
}

std::wstring fs::change_systemroot_path_to_dos_path(const std::wstring& s) {
	std::wstring result(s);

	// Change path, ex. \SystemRoot\System32\Conhost.exe => C:\Windows\System32\Conhost.exe
	boost::algorithm::replace_first(result, L"\\SystemRoot\\", L"C:\\Windows\\");
	return result;
}

std::wstring fs::handle_to_nt_path(HANDLE h)
{
    std::wstring path;
    GetFinalPathNameByHandleW(h, NX::string_buffer<wchar_t>(path, 1024), 1024, VOLUME_NAME_NT);
    return std::move(path);
}

std::wstring fs::handle_to_dos_path(HANDLE h)
{
    std::wstring final_path;
    const std::wstring& ntpath = handle_to_nt_path(h);
    if (!ntpath.empty()) {
        final_path = nt_path_to_dos_path(ntpath);
    }
    return std::move(final_path);
}

std::wstring fs::nt_path_to_dos_path(const std::wstring& s)
{
    std::wstring final_path;
    if (s.empty()) {
        return final_path;
    }
    const std::vector<fs::drive>& drives = get_logic_drives();
    auto pos = std::find_if(drives.begin(), drives.end(), [s](const fs::drive& drv)->bool {
        std::wstring nt_root_name = drv.nt_name();
        nt_root_name += L"\\";
        return boost::algorithm::istarts_with(s, nt_root_name);
    });
    if (pos != drives.end()) {
        final_path = (*pos).dos_name();
        final_path += s.substr((*pos).nt_name().length());
    }
    return std::move(final_path);
}

std::vector<fs::drive> fs::get_logic_drives()
{
    std::vector<fs::drive> drives;
    wchar_t drive_names[512] = { 0 };
    memset(drive_names, 0, sizeof(drive_names));
    if (GetLogicalDriveStrings(511, drive_names)) {
        const wchar_t* p = drive_names;
        while (0 != *p) {
            const wchar_t dos_drive_letter = (wchar_t)toupper(*p);
            p += (wcslen(p) + 1);
            drives.push_back(fs::drive(dos_drive_letter));
        }
    }
    return std::move(drives);
}


//
//  file
//

file_object::file_object() : _h(INVALID_HANDLE_VALUE)
{
}

file_object::~file_object()
{
    close();
}

void file_object::close()
{
    if (INVALID_HANDLE_VALUE != _h) {
        CloseHandle(_h);
        _h = INVALID_HANDLE_VALUE;
    }
}

void file_object::inter_open_to_read(const std::wstring& file)
{
    close();
    _h = ::CreateFileW(file.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (!opened()) {
        throw NX::exception(WIN32_ERROR_MSG2(GetLastError()));
    }
}

void file_object::inter_open_to_write(const std::wstring& file, bool write_through)
{
    close();
    _h = ::CreateFileW(file.c_str(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL | (write_through ? FILE_FLAG_WRITE_THROUGH : 0), NULL);
    if (!opened()) {
        throw NX::exception(WIN32_ERROR_MSG2(GetLastError()));
    }
}

void file_object::inter_create(const std::wstring& file, bool write_through)
{
    close();
    _h = ::CreateFileW(file.c_str(), GENERIC_READ | GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL | (write_through ? FILE_FLAG_WRITE_THROUGH : 0), NULL);
    if (!opened()) {
        throw NX::exception(WIN32_ERROR_MSG2(GetLastError()));
    }
}

__int64 file_object::seek_from_begin(__int64 n)
{
    LARGE_INTEGER li_to_move = { 0, 0 };
    LARGE_INTEGER li_new_pos = { 0, 0 };

    li_to_move.QuadPart = n;

    if (!SetFilePointerEx(_h, li_to_move, &li_new_pos, FILE_BEGIN)) {
        throw NX::exception(WIN32_ERROR_MSG2(GetLastError()));
    }

    // And never beyond file size
    return (li_new_pos.QuadPart > size()) ? seek_to_end() : li_new_pos.QuadPart;
}

__int64 file_object::seek_from_end(__int64 n)
{
    LARGE_INTEGER li_to_move = { 0, 0 };
    LARGE_INTEGER li_new_pos = { 0, 0 };

    if (n > 0) {
        // Never seek beyond file size
        seek_to_end();
    }

    li_to_move.QuadPart = n;
    if (!SetFilePointerEx(_h, li_to_move, &li_new_pos, FILE_END)) {
        // Failed
        if (ERROR_NEGATIVE_SEEK != GetLastError()) {
            throw NX::exception(WIN32_ERROR_MSG2(GetLastError()));
        }
        // An attempt was made to move the file pointer before the beginning of the file.
        // So just move to beginning
        return seek_to_begin();
    }

    return li_new_pos.QuadPart;;
}

__int64 file_object::seek_from_current(__int64 n)
{
    LARGE_INTEGER li_to_move = { 0, 0 };
    LARGE_INTEGER li_new_pos = { 0, 0 };

    if (!SetFilePointerEx(_h, li_to_move, &li_new_pos, FILE_CURRENT)) {
        if (ERROR_NEGATIVE_SEEK != GetLastError()) {
            throw NX::exception(WIN32_ERROR_MSG2(GetLastError()));
        }
        // An attempt was made to move the file pointer before the beginning of the file.
        // So just move to beginning
        return seek_to_begin();
    }

    // Never seek beyond file size
    return (li_new_pos.QuadPart > size()) ? seek_to_end() : li_new_pos.QuadPart;
}

__int64 file_object::seek_to_begin()
{
    LARGE_INTEGER li_to_move = { 0, 0 };
    LARGE_INTEGER li_new_pos = { 0, 0 };
    if (!SetFilePointerEx(_h, li_to_move, &li_new_pos, FILE_BEGIN)) {
        // Failed
        throw NX::exception(WIN32_ERROR_MSG2(GetLastError()));
    }
    return li_new_pos.QuadPart;
}

__int64 file_object::seek_to_end()
{
    LARGE_INTEGER li_to_move = { 0, 0 };
    LARGE_INTEGER li_new_pos = { 0, 0 };
    if (!SetFilePointerEx(_h, li_to_move, &li_new_pos, FILE_END)) {
        // Failed
        throw NX::exception(WIN32_ERROR_MSG2(GetLastError()));
    }
    return li_new_pos.QuadPart;
}

__int64 file_object::seek_to_current()
{
    LARGE_INTEGER li_to_move = { 0, 0 };
    LARGE_INTEGER li_new_pos = { 0, 0 };
    (VOID)SetFilePointerEx(_h, li_to_move, &li_new_pos, FILE_END);
    return li_new_pos.QuadPart;
}

__int64 file_object::size()
{
    LARGE_INTEGER li_size = { 0, 0 };
    if (!GetFileSizeEx(_h, &li_size)) {
        throw NX::exception(WIN32_ERROR_MSG2(GetLastError()));
    }
    return li_size.QuadPart;
}

void file_object::resize(__int64 new_size)
{
    LARGE_INTEGER li_to_move = { 0, 0 };
    LARGE_INTEGER li_new_pos = { 0, 0 };
    li_to_move.QuadPart = new_size;
    if (SetFilePointerEx(_h, li_to_move, &li_new_pos, FILE_BEGIN)) {
        // Succeeded
        SetEndOfFile(_h);
    }
    // update current pos
    seek_to_current();
}

std::vector<unsigned char> file_object::inter_read(unsigned long bytes_to_read)
{
    std::vector<unsigned char> buf;
    buf.resize(bytes_to_read, 0);
    unsigned long bytes_read = inter_read(buf.data(), bytes_to_read);
    if (bytes_read != bytes_to_read) {
        buf.resize(bytes_read);
    }
    return std::move(buf);
}

unsigned long file_object::inter_read(unsigned char* buf, unsigned long bytes_to_read)
{
    unsigned long bytes_read = 0;
    if (!::ReadFile(_h, buf, bytes_to_read, &bytes_read, NULL)) {
        throw NX::exception(WIN32_ERROR_MSG2(GetLastError()));
    }
    return bytes_read;
}

unsigned long file_object::inter_write(const std::vector<unsigned char>& data)
{
    unsigned long bytes_written = 0;
    if (!::WriteFile(_h, data.data(), (unsigned long)data.size(), &bytes_written, NULL)) {
        throw NX::exception(WIN32_ERROR_MSG2(GetLastError()));
    }
    return bytes_written;
}


std::wstring NX::fs::create_unique_directory(const std::wstring& parent_dir, const std::wstring& name_prefix, bool hide)
{
    std::wstring dir;
    bool created = false;

    DWORD dwAttrs = GetFileAttributesW(parent_dir.c_str());
    if (INVALID_FILE_ATTRIBUTES == dwAttrs || FILE_ATTRIBUTE_DIRECTORY != (dwAttrs & FILE_ATTRIBUTE_DIRECTORY)) {
        return dir;
    }

    std::wstring tmp_dir_root(parent_dir);
    if (!boost::algorithm::ends_with(tmp_dir_root, L"\\")) {
        tmp_dir_root.append(L"\\");
    }
    tmp_dir_root += name_prefix;

    int count = 0;

    do {

        ++count;

        // generate random value
        srand((unsigned int)time(nullptr));
        std::wstring tmp_dir_path = tmp_dir_root + NX::string_formater(L"%08X", (unsigned long)rand());
        
        if (INVALID_FILE_ATTRIBUTES != GetFileAttributesW(tmp_dir_path.c_str())) {
            continue;
        }
        if (!::CreateDirectoryW(tmp_dir_path.c_str(), NULL)) {
            continue;
        }

        dir = tmp_dir_path;
        created = true;
        
    } while (!created && 20 > count);

    return std::move(dir);
}

bool NX::fs::delete_file(const std::wstring& file, bool force)
{
    DWORD attributes = GetFileAttributesW(file.c_str());
    const bool read_only = (FILE_ATTRIBUTE_READONLY == (attributes & FILE_ATTRIBUTE_READONLY));

    if (read_only) {

        if (!force) {
            SetLastError(ERROR_ACCESS_DENIED);
            return false;
        }

        if (!SetFileAttributesW(file.c_str(), attributes & (~FILE_ATTRIBUTE_READONLY))) {
            return false;
        }
    }

    bool result = ::DeleteFileW(file.c_str()) ? true : false;

    if (read_only) {
        assert(force);
        SetFileAttributesW(file.c_str(), attributes);
    }

    return result;
}

bool NX::fs::remove_directory(const std::wstring& dir, bool force)
{
    bool result = true;
    HANDLE h = INVALID_HANDLE_VALUE;
    std::wstring find_str(dir);
    WIN32_FIND_DATAW wfd = { 0 };

    std::wstring dir_path(dir);
    if (boost::algorithm::iends_with(find_str, L"\\")) {
        dir_path = dir_path.substr(0, dir_path.length() - 1);
    }
    find_str = dir_path ;
    find_str.append(L"\\*");

    memset(&wfd, 0, sizeof(wfd));
    h = ::FindFirstFileW(find_str.c_str(), &wfd);
    if (INVALID_HANDLE_VALUE == h) {
        return false;
    }

    do {

        if (0 == _wcsicmp(wfd.cFileName, L".") || 0 == _wcsicmp(wfd.cFileName, L"..")) {
            continue;
        }

        const std::wstring sub_file(dir_path + L"\\" + wfd.cFileName);

        if (FILE_ATTRIBUTE_DIRECTORY == (wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
            if (!NX::fs::remove_directory(sub_file, force)) {
                result = false;
            }
        }
        else {
            if (!NX::fs::delete_file(sub_file, force)) {
                result = false;
            }
        }

    } while (FindNextFileW(h, &wfd));
    FindClose(h);
    h = INVALID_HANDLE_VALUE;

    if (result) {
        result = ::RemoveDirectoryW(dir_path.c_str()) ? true : false;
    }

    return result;
}

bool NX::fs::remove_directory(const std::wstring& dir, bool force, std::vector<std::wstring>& undeleted_files)
{
    bool result = true;
    HANDLE h = INVALID_HANDLE_VALUE;
    std::wstring find_str(dir);
    WIN32_FIND_DATAW wfd = { 0 };

    std::wstring dir_path(dir);
    if (boost::algorithm::iends_with(find_str, L"\\")) {
        dir_path = dir_path.substr(0, dir_path.length() - 1);
    }
    find_str = dir_path;
    find_str.append(L"\\*");

    memset(&wfd, 0, sizeof(wfd));
    h = ::FindFirstFileW(find_str.c_str(), &wfd);
    if (INVALID_HANDLE_VALUE == h) {
        return false;
    }

    do {

        if (0 == _wcsicmp(wfd.cFileName, L".") || 0 == _wcsicmp(wfd.cFileName, L"..")) {
            continue;
        }

        const std::wstring sub_file(dir_path + L"\\" + wfd.cFileName);

        if (FILE_ATTRIBUTE_DIRECTORY == (wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
            if (!NX::fs::remove_directory(sub_file, force, undeleted_files)) {
                result = false;
            }
        }
        else {
            if (!NX::fs::delete_file(sub_file, force)) {
                result = false;
                undeleted_files.push_back(sub_file);
            }
        }

    } while (FindNextFileW(h, &wfd));
    FindClose(h);
    h = INVALID_HANDLE_VALUE;

    if (result) {
        result = ::RemoveDirectoryW(dir_path.c_str()) ? true : false;
    }

    return result;
}

bool NX::fs::IsFileClosed(_In_ const std::wstring& file)
{
    HANDLE h = ::CreateFileW(file.c_str(), GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (INVALID_HANDLE_VALUE != h) {
        CloseHandle(h);
        return true;
    }
    return false;
}

bool NX::fs::open_file_exclusively(_In_ const std::wstring& file, _In_ DWORD wait_milliseconds)
{
    do {

        HANDLE h = ::CreateFileW(file.c_str(), GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        if (INVALID_HANDLE_VALUE != h) {
            CloseHandle(h);
            return true;
        }
        // Failed
        if (ERROR_ACCESS_DENIED != GetLastError()) {
            return true;
        }

        // It is access denied, this might because the file is opened by another program
        // Wait and try again
        Sleep(200);
        if (INFINITE != wait_milliseconds) {
            wait_milliseconds = (wait_milliseconds > 200) ? (wait_milliseconds - 200) : 0;
        }

    } while (0 != wait_milliseconds);

    return false;
}

bool file_checksum::load_file_content(const std::wstring& file, std::vector<unsigned char>& content)
{
    content.clear();

    HANDLE h = ::CreateFileW(file.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (INVALID_HANDLE_VALUE == h) {
        return false;
    }

    bool result = false;

    do {

        const DWORD dwSize = GetFileSize(h, NULL);
        if (INVALID_FILE_SIZE == dwSize) {
            break;
        }

        if (dwSize == 0) {
            result = true;
            break;
        }

        DWORD dwBytesRead = 0;
        content.resize(dwSize, 0);
        if (!::ReadFile(h, content.data(), dwSize, &dwBytesRead, NULL)) {
            break;
        }
        if (dwBytesRead != dwSize) {
            break;
        }

        result = true;

    } while (false);
    if (INVALID_HANDLE_VALUE != h) {
        CloseHandle(h);
        h = INVALID_HANDLE_VALUE;
    }

    if (!result) {
        content.clear();
    }
    return result;
}

file_checksum_crc32::file_checksum_crc32() : file_checksum(), _checksum(0)
{
}

file_checksum_crc32::file_checksum_crc32(const file_checksum_crc32& other) : _checksum(other._checksum)
{
}

file_checksum_crc32::~file_checksum_crc32()
{
}

bool file_checksum_crc32::empty() const
{
    return (0 == _checksum);
}

bool file_checksum_crc32::generate(const std::wstring& file)
{
    std::vector<unsigned char> buf;
    if (!load_file_content(file, buf)) {
        return false;
    }
    if (buf.empty()) {
        return true;
    }
    LARGE_INTEGER li = { 0, 0 };
    li.LowPart = NX::utility::calculate_crc32(buf.data(), (unsigned long)buf.size());
    _checksum = li.QuadPart;
    return true;
}

std::wstring file_checksum_crc32::serialize() const
{
    return NX::conversion::to_wstring(_checksum);
}

file_checksum_crc32& file_checksum_crc32::operator = (const file_checksum_crc32& other)
{
    if (this != &other) {
        _checksum = other._checksum;
    }
    return *this;
}


file_checksum_sha1::file_checksum_sha1() : file_checksum()
{
}

file_checksum_sha1::file_checksum_sha1(const file_checksum_sha1& other) : _checksum(other._checksum)
{
}

file_checksum_sha1::~file_checksum_sha1()
{
}

bool file_checksum_sha1::empty() const
{
    return _checksum.empty();
}

bool file_checksum_sha1::generate(const std::wstring& file)
{
    std::vector<unsigned char> buf;
    if (!load_file_content(file, buf)) {
        return false;
    }
    if (buf.empty()) {
        return true;
    }

    _checksum.resize(20, 0);
    if (!NX::crypto::sha1(buf.data(), (unsigned long)buf.size(), _checksum.data())) {
        _checksum.clear();
        return false;
    }

    return true;
}

std::wstring file_checksum_sha1::serialize() const
{
    return NX::conversion::to_wstring(_checksum);
}

file_checksum_sha1& file_checksum_sha1::operator = (const file_checksum_sha1& other)
{
    if (this != &other) {
        _checksum = other._checksum;
    }
    return *this;
}