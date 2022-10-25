

#include <windows.h>
#include <assert.h>

#include <string>
#include <iostream>

#include <boost/algorithm/string.hpp>

#include <nudf\eh.hpp>
#include <nudf\debug.hpp>
#include <nudf\winutil.hpp>
#include <nudf\string.hpp>
#include <nudf\dbglog.hpp>
#include <nudf\json.hpp>
#include <nudf\crypto.hpp>
#include <nudf\ntapi.hpp>
#include <nudf\shared\rightsdef.h>

#include <nxlfmthlp.hpp>

#include "nxrmserv.hpp"
#include "serv.hpp"
#include "global.hpp"
#include "rsapi.hpp"
#include "logactivity.hpp"


extern rmserv* SERV;

activity_record::activity_record()
{
}

activity_record::activity_record(const std::vector<unsigned char>& duid,
    const std::wstring& owner_id,
    __int64 user_id,
    int operation,
    int result,
    const std::wstring& file_path,
    const std::wstring& app_path,
    const std::wstring& app_publisher,
    const std::wstring& extra_data)
{
    std::wstring s;
    NX::time::datetime tm = NX::time::datetime::current_time();

    // 0: DUID
    //_s = "\"";
    s += NX::conversion::to_wstring(duid);
    //_s += "\"";

    // 1: Owner Id
    s += L",";
    s += owner_id;

    // 2: User Id
    s += L",";
    s += NX::conversion::to_wstring(user_id);

    // 3: Operation
    s += L",";
    s += NX::conversion::to_wstring(operation);

    // 4: Device Id (Client Id)
    s += L",";
    //s += SERV->get_client_id();
    s += GLOBAL.get_host().dns_host_name();

    // 5: Device Type (Platform Id)
    s += L",";
    s += NX::conversion::to_wstring((int)GLOBAL.get_windows_info().platform_id());

    // 6: repositoryId
    s += L",";

    // 7: filePathId
    s += L",";

    // 8: fileName
    s += L",";

    // 9: filePath
    s += L",";
    s += file_path;

    // 10: appName
    s += L",";

    // 11: appPath
    s += L",";
    s += app_path;

    // 12: appPublisher
    s += L",";
    std::wstring fixed_publisher = app_publisher;
    boost::algorithm::replace_all(fixed_publisher, L",", L" ");
    s += fixed_publisher;

    // 13: accessResult
    s += L",";
    s += NX::conversion::to_wstring(result);

    // 14: accessTime
    s += L",";
    s += NX::conversion::to_wstring(tm.to_java_time());

    // 15: accessTime
    s += L",";
    s += extra_data;

    _s += NX::conversion::utf16_to_utf8(s);
}

activity_record::activity_record(const activity_record& other) : _s(other._s)
{
}

activity_record::activity_record(activity_record&& other) : _s(std::move(other._s))
{
}

activity_record::~activity_record()
{
}

activity_record& activity_record::operator = (const activity_record& other)
{
    if (this != &other) {
        _s = other._s;
    }
    return *this;
}

activity_record& activity_record::operator = (activity_record&& other)
{
    if (this != &other) {
        _s = std::move(other._s);
    }
    return *this;
}



activity_logger::activity_logger(rmsession* p) : _rmsession(p), _h(INVALID_HANDLE_VALUE), _count(0)
{
    ::InitializeCriticalSection(&_lock);
}

activity_logger::~activity_logger()
{
    clear();
    ::DeleteCriticalSection(&_lock);
}

bool activity_logger::init()
{
    const std::wstring logfile(_rmsession->get_temp_profile_dir() + L"\\activity.log");
    
    _h = ::CreateFileW(logfile.c_str(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_WRITE_THROUGH, NULL);
    if (INVALID_HANDLE_VALUE == _h) {
        LOGERROR(NX::string_formater(L"Fail to create/open activity log: %s", logfile.c_str()));
        return false;
    }

    std::vector<unsigned char> buf;
    load_from_file(buf);
    if (!buf.empty()) {
        buf.push_back(0);
        const char* s = (const char*)buf.data();
        while (s != nullptr && *s != 0) {

            if (*s == '\n') {
                ++s;
                continue;
            }
            else {
                _count++;
                s = strchr(s, '\n');
                if (nullptr != s) {
                    ++s;
                }
            }
        }
    }

    return true;
}

void activity_logger::clear()
{
    if (INVALID_HANDLE_VALUE != _h) {
        CloseHandle(_h);
        _h = INVALID_HANDLE_VALUE;
    }
}

unsigned long activity_logger::push(const activity_record& record)
{
    if (record.get_log().length() != 0) {
        ::EnterCriticalSection(&_lock);
        save_to_file((const unsigned char*)record.get_log().c_str(), (unsigned long)record.get_log().length());
        ::LeaveCriticalSection(&_lock);
    }

    return get_count();
}

void activity_logger::upload_log()
{
    std::vector<unsigned char> buf;

    ::EnterCriticalSection(&_lock);
    load_from_file(buf);
    if (!buf.empty()) {
        if (_rmsession->upload_activity_log(buf)) {
            clear_file();
        }
    }
    ::LeaveCriticalSection(&_lock);
}

unsigned long activity_logger::get_count()
{
    unsigned long c = 0;
    ::EnterCriticalSection(&_lock);
    c = _count;
    ::LeaveCriticalSection(&_lock);
    return c;
}

bool activity_logger::save_to_file(const unsigned char* logdata, const unsigned long size)
{
    static const CHAR lineBreak = '\n';
    ULONG dwSize = 0;
    DWORD dwBytesWritten = 0;
    bool result = false;

    if (INVALID_HANDLE_VALUE == _h) {
        SetLastError(ERROR_INVALID_HANDLE);
        return false;
    }
    if (NULL == logdata || 0 == size) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return false;
    }
    
    do {

        ULONG dwSize = 0;
        ULONG dwBytesWritten = 0;


        dwSize = GetFileSize(_h, NULL);
        if (INVALID_FILE_SIZE == dwSize) {
            break;
        }

        //  1 MB = 1048576 Bytes (0x100000)
        // 16 MB = 16777216 Bytes (0x1000000)
        // 64 MB = 67108864 Bytes (0x4000000)

        if (dwSize > 16777216) {
            // Clear old logs
            SetFilePointer(_h, 0, NULL, FILE_BEGIN);
            SetEndOfFile(_h);
            _count = 0;
        }

        if (INVALID_SET_FILE_POINTER == SetFilePointer(_h, 0, NULL, FILE_END)) {
            break;
        }

        if (::WriteFile(_h, logdata, size, &dwBytesWritten, NULL)) {
            ::WriteFile(_h, &lineBreak, 1, &dwBytesWritten, NULL);
			::FlushFileBuffers(_h);
			_count++;
            result = true;
        }

    } while (false);

    return result;
}

bool activity_logger::load_from_file(std::vector<unsigned char>& logdata)
{
    bool result = false;

    if (INVALID_HANDLE_VALUE == _h) {
        SetLastError(ERROR_INVALID_HANDLE);
        return false;
    }

    do {

        ULONG dwSize = 0;
        ULONG dwBytesRead = 0;

        dwSize = GetFileSize(_h, NULL);
        if (INVALID_FILE_SIZE == dwSize || 0 == dwSize) {
            break;
        }

        if (INVALID_SET_FILE_POINTER == SetFilePointer(_h, 0, NULL, FILE_BEGIN)) {
            break;
        }

        logdata.resize(dwSize, 0);
        if (!::ReadFile(_h, logdata.data(), dwSize, &dwBytesRead, NULL)) {
            logdata.clear();
            break;
        }

        result = true;

    } while (false);

    return result;
}

bool activity_logger::clear_file()
{
    if (INVALID_HANDLE_VALUE == _h) {
        SetLastError(ERROR_INVALID_HANDLE);
        return false;
    }

    _count = 0;
    SetFilePointer(_h, 0, NULL, FILE_BEGIN);
    SetEndOfFile(_h);
	::FlushFileBuffers(_h);
	return true;
}



metadata_record::metadata_record()
{
}

metadata_record::metadata_record(const std::wstring& duid,
	const std::wstring& otp,
	const std::wstring& adhoc,
	const std::wstring& tags)
{
	std::wstring s;

	{
		NX::json_value json_body = NX::json_value::create_object(std::vector<std::pair<std::wstring, NX::json_value>>({
			std::pair<std::wstring, NX::json_value>(L"parameters", NX::json_value::create_object())
			}), false);
		NX::json_object& json_parameters = json_body.as_object().at(L"parameters").as_object();

		json_parameters[L"fileTags"] = NX::json_value(tags);
		if (tags.size() <= 3) // tags = "", or "{}\0"
			json_parameters[L"protectionType"] = NX::json_value(0);
		else
			json_parameters[L"protectionType"] = NX::json_value(1);
		json_parameters[L"ml"] = NX::json_value(0);
		json_parameters[L"otp"] = NX::json_value(otp);
		json_parameters[L"filePolicy"] = NX::json_value(adhoc);

		const std::wstring& parameters = json_body.serialize();

		s = parameters;
	}

	{
		NX::json_value json_body = NX::json_value::create_object(std::vector<std::pair<std::wstring, NX::json_value>>({
			std::pair<std::wstring, NX::json_value>(L"parameters", NX::json_value::create_object())
			}), false);
		NX::json_object& json_parameters = json_body.as_object().at(L"parameters").as_object();

		json_parameters[L"duid"] = NX::json_value(duid);
		json_parameters[L"projectid"] = NX::json_value(0);
		json_parameters[L"params"] = NX::json_value(s);
		json_parameters[L"type"] = NX::json_value(0);

		const std::wstring& parameters = json_body.serialize();

		_s += NX::conversion::utf16_to_utf8(parameters);
	}
}

metadata_record::metadata_record(const std::wstring& duid,
	const int projectid,
	const std::wstring& params,
	const int type)
{
	NX::json_value json_body = NX::json_value::create_object(std::vector<std::pair<std::wstring, NX::json_value>>({
		std::pair<std::wstring, NX::json_value>(L"parameters", NX::json_value::create_object())
		}), false);
	NX::json_object& json_parameters = json_body.as_object().at(L"parameters").as_object();

	json_parameters[L"duid"] = NX::json_value(duid);
	json_parameters[L"projectid"] = NX::json_value(projectid);
	json_parameters[L"params"] = NX::json_value(params);
	json_parameters[L"type"] = NX::json_value(type);

	const std::wstring& parameters = json_body.serialize();

	_s += NX::conversion::utf16_to_utf8(parameters);
}

metadata_record::metadata_record(const metadata_record& other) : _s(other._s)
{
}

metadata_record::metadata_record(metadata_record&& other) : _s(std::move(other._s))
{
}

metadata_record::~metadata_record()
{
}

metadata_record& metadata_record::operator = (const metadata_record& other)
{
	if (this != &other) {
		_s = other._s;
	}
	return *this;
}

metadata_record& metadata_record::operator = (metadata_record&& other)
{
	if (this != &other) {
		_s = std::move(other._s);
	}
	return *this;
}



metadata_logger::metadata_logger(rmsession* p) : _rmsession(p), _h(INVALID_HANDLE_VALUE), _count(0)
{
	::InitializeCriticalSection(&_lock);
}

metadata_logger::~metadata_logger()
{
	clear();
	::DeleteCriticalSection(&_lock);
}

bool metadata_logger::init()
{
	const std::wstring logfile(_rmsession->get_temp_profile_dir() + L"\\metadata.log");

	_h = ::CreateFileW(logfile.c_str(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_WRITE_THROUGH, NULL);
	if (INVALID_HANDLE_VALUE == _h) {
		LOGERROR(NX::string_formater(L"Fail to create/open metadata log: %s", logfile.c_str()));
		return false;
	}

	std::vector<unsigned char> buf;
	load_from_file(buf);
	if (!buf.empty()) {
		buf.push_back(0);
		const char* s = (const char*)buf.data();
		while (s != nullptr && *s != 0) {

			if (*s == '\n') {
				++s;
				continue;
			}
			else {
				_count++;
				s = strchr(s, '\n');
				if (nullptr != s) {
					++s;
				}
			}
		}
	}

	return true;
}

void metadata_logger::clear()
{
	if (INVALID_HANDLE_VALUE != _h) {
		CloseHandle(_h);
		_h = INVALID_HANDLE_VALUE;
	}
}

unsigned long metadata_logger::push(const metadata_record& record)
{
	if (record.get_log().length() != 0) {

		::EnterCriticalSection(&_lock);
		save_to_file((const unsigned char*)record.get_log().c_str(), (unsigned long)record.get_log().length());
		::LeaveCriticalSection(&_lock);
	}

	return get_count();
}

void metadata_logger::upload_log()
{
	std::vector<std::string> buf;
	std::vector<std::string> retrylogs;

	::EnterCriticalSection(&_lock);
	load_from_file(buf);

	if (!buf.empty()) {
        LOGDEBUG(NX::string_formater(L"upload_log: found metadata which are not synced"));
        
        _rmsession->upload_metadata_log(buf, retrylogs);
        clear_file();
        if (retrylogs.size() > 0) {
			for (size_t i = 0; i < retrylogs.size(); i++)
			{
                LOGDEBUG(NX::string_formater(L"upload_log: still failed to sync (%s)", retrylogs[i].c_str()));
                save_to_file((const unsigned char*)retrylogs[i].c_str(), (unsigned long)retrylogs[i].length());
			}
		}
	}
	::LeaveCriticalSection(&_lock);
}

unsigned long metadata_logger::get_count()
{
	unsigned long c = 0;
	::EnterCriticalSection(&_lock);
	c = _count;
	::LeaveCriticalSection(&_lock);
    LOGDEBUG(NX::string_formater(L"metadata_logger:: total (%d) records waiting for upload", c));
    return c;
}

bool metadata_logger::save_to_file(const unsigned char* logdata, const unsigned long size)
{
	static const CHAR lineBreak = '\n';
	ULONG dwSize = 0;
	DWORD dwBytesWritten = 0;
	bool result = false;

	if (INVALID_HANDLE_VALUE == _h) {
		SetLastError(ERROR_INVALID_HANDLE);
        LOGDEBUG(NX::string_formater(L"metadata_logger::save_to_file failed, no cache file"));
        return false;
	}
	if (NULL == logdata || 0 == size) {
		SetLastError(ERROR_INVALID_PARAMETER);
        LOGDEBUG(NX::string_formater(L"metadata_logger::save_to_file failed, no data"));
        return false;
	}

	do {

		ULONG dwSize = 0;
		ULONG dwBytesWritten = 0;


		dwSize = GetFileSize(_h, NULL);
		if (INVALID_FILE_SIZE == dwSize) {
            LOGDEBUG(NX::string_formater(L"metadata_logger::save_to_file failed, cache file corrupted"));
            break;
		}

		//  1 MB = 1048576 Bytes (0x100000)
		// 16 MB = 16777216 Bytes (0x1000000)
		// 64 MB = 67108864 Bytes (0x4000000)

		if (dwSize > 67108864) {
            LOGDEBUG(NX::string_formater(L"metadata_logger::save_to_file, file size exceeds 67108864 bytes"));
            // Clear old logs
			SetFilePointer(_h, 0, NULL, FILE_BEGIN);
			SetEndOfFile(_h);
			_count = 0;
		}

		if (INVALID_SET_FILE_POINTER == SetFilePointer(_h, 0, NULL, FILE_END)) {
            LOGDEBUG(NX::string_formater(L"metadata_logger::save_to_file failed, SetFilePointer failed as cache file corrupted"));
            break;
		}

		if (::WriteFile(_h, logdata, size, &dwBytesWritten, NULL)) {
			::WriteFile(_h, &lineBreak, 1, &dwBytesWritten, NULL);
			::FlushFileBuffers(_h);
			_count++;
			result = true;
            LOGDEBUG(NX::string_formater(L"metadata_logger::save_to_file OK, %s", logdata));
        }
        else
        {
            LOGDEBUG(NX::string_formater(L"metadata_logger::save_to_file failed, WriteFile failed"));
        }

	} while (false);

	return result;
}

bool metadata_logger::load_from_file(std::vector<unsigned char>& logdata)
{
	bool result = false;

	if (INVALID_HANDLE_VALUE == _h) {
		SetLastError(ERROR_INVALID_HANDLE);
		return false;
	}

	do {

		ULONG dwSize = 0;
		ULONG dwBytesRead = 0;

		dwSize = GetFileSize(_h, NULL);
		if (INVALID_FILE_SIZE == dwSize || 0 == dwSize) {
			break;
		}

		if (INVALID_SET_FILE_POINTER == SetFilePointer(_h, 0, NULL, FILE_BEGIN)) {
			break;
		}

		logdata.resize(dwSize, 0);
		if (!::ReadFile(_h, logdata.data(), dwSize, &dwBytesRead, NULL)) {
			logdata.clear();
			break;
		}

		result = true;

	} while (false);

	return result;
}

bool metadata_logger::load_from_file(std::vector<std::string>& logdata)
{
	bool result = false;

	if (INVALID_HANDLE_VALUE == _h) {
		SetLastError(ERROR_INVALID_HANDLE);
		return false;
	}

	std::vector<unsigned char> buf;
	load_from_file(buf);
	if (!buf.empty()) {
		buf.push_back(0);
		std::string line = "";
		const char* s = (const char*)buf.data();
		while (s != nullptr && *s != 0) {

			if (*s == '\n') {
				//line.push_back('\0');
				logdata.push_back(line);
				line = "";
				++s;
				continue;
			}
			else {
				if (nullptr != s) {
					line.push_back(*s);
					++s;
				}
			}
		}
	}

	return result;
}

bool metadata_logger::clear_file()
{
	if (INVALID_HANDLE_VALUE == _h) {
		SetLastError(ERROR_INVALID_HANDLE);
		return false;
	}

	_count = 0;
	SetFilePointer(_h, 0, NULL, FILE_BEGIN);
	SetEndOfFile(_h);
	::FlushFileBuffers(_h);
	return true;
}
