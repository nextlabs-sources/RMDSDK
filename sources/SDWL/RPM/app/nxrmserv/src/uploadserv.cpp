

#include <Windows.h>


#include <nudf/eh.hpp>
#include <nudf/debug.hpp>
#include <nudf/dbglog.hpp>
#include <nudf/string.hpp>
#include <nudf/ntapi.hpp>
#include <nudf/conversion.hpp>

#include "nxrmserv.hpp"
#include "global.hpp"
#include "session.hpp"
#include "uploadserv.hpp"



#define MAX_UPLOAD_QUEUE_SIZE   1024


upload_object::upload_object() : _type(UPLOAD_NONE)
{
}

upload_object::upload_object(int type, const std::wstring& checksum, const std::wstring& data) : _type(type), _checksum(checksum), _data(data)
{
}

upload_object::upload_object(const upload_object& other) : _type(other._type), _checksum(other._checksum), _data(other._data)
{
}

upload_object::upload_object(upload_object&& other) : _type(other._type), _checksum(std::move(other._checksum)), _data(std::move(other._data))
{
    other._type = UPLOAD_NONE;
}

upload_object::~upload_object()
{
}

upload_object& upload_object::operator = (const upload_object& other)
{
    if (this != &other) {
        _type = other._type;
        _checksum = other._checksum;
        _data = other._data;
    }
    return *this;
}

upload_object& upload_object::operator = (upload_object&& other)
{
    if (this != &other) {
        _type = std::move(other._type);
        _checksum = std::move(other._checksum);
        _data = std::move(other._data);
    }
    return *this;
}

void upload_object::clear()
{
    _type = UPLOAD_NONE;
    _checksum.clear();
    _data.clear();
}


//
//
//

upload_serv::upload_serv() : _b(false)
{
    ::InitializeCriticalSection(&_lock);
    _events[0] = ::CreateEventW(NULL, TRUE, FALSE, NULL);   // stop event
    _events[1] = ::CreateEventW(NULL, TRUE, FALSE, NULL);   // request coming event
    //_events[1] = ::CreateSemaphoreW(NULL, 0, 1, NULL);      // request coming event
}

upload_serv::~upload_serv()
{
    stop();

    if (NULL != _events[0]) {
        CloseHandle(_events[0]);
        _events[0] = NULL;
    }
    if (NULL != _events[1]) {
        CloseHandle(_events[1]);
        _events[1] = NULL;
    }
    ::DeleteCriticalSection(&_lock);
}

bool upload_serv::start()
{
    unsigned long result = 0;


    // Set running flag before start worker thread
    _b = true;

    // start worker thread
    try {
        _t = std::thread(upload_serv::worker, this);
        LOGDEBUG("UPLOADSERV started successfully");
    }
    catch (std::exception& e) {
        _b = false;
        LOGWARNING(ERROR_MSG2("UPLOADSERV", "fail to start worker thread (%s)", e.what()));
    }

    return _b;
}

void upload_serv::stop()
{
    if (!_b) {
        return;
    }

    
    assert(_b);
    _b = false;

    // stop all the request handler threads
    SetEvent(_events[0]);
    if (_t.joinable()) {
        _t.join();
    }
    ResetEvent(_events[0]);

    // If queue is not empty, save to file
}

bool upload_serv::push(const upload_object& object)
{
    return inter_push(object, true);
}

void upload_serv::check_queue()
{
    ::EnterCriticalSection(&_lock);
    SetEvent(_events[1]);
    ::LeaveCriticalSection(&_lock);
}

bool upload_serv::inter_push(const upload_object& object, bool notify)
{
    bool result = false;

    assert(object.is_known_type());
    if (!object.is_known_type()) {
        return false;
    }

    ::EnterCriticalSection(&_lock);
    if (_q.size() < MAX_UPLOAD_QUEUE_SIZE) {
        result = true;
        _q.push(object);
        if (notify) {
            SetEvent(_events[1]);
        }
    }
    ::LeaveCriticalSection(&_lock);

    if (!result) {
        LOGERROR(NX::string_formater("Upload queue (tenant: %S) reach limitation", _s->get_tenant_id().c_str()));
    }
    return result;
}

upload_object upload_serv::pop()
{
    upload_object object;
    ::EnterCriticalSection(&_lock);
    if (!_q.empty()) {
        object = _q.front();
        _q.pop();
    }
    else {
        ResetEvent(_events[1]);
    }
    ::LeaveCriticalSection(&_lock);
    return std::move(object);
}

bool upload_serv::upload_data(const upload_object& object)
{
    return _s->upload_data(object);
}

void upload_serv::save_objects()
{
    NX::json_value objects_array = NX::json_value::create_array();

    std::vector<upload_object> saved_objects;

    ::EnterCriticalSection(&_lock);
    while (!_q.empty()) {

        upload_object uo = _q.front();
        _q.pop();
        if (uo.is_known_type() && !uo.empty()) {
            saved_objects.push_back(uo);
        }

    }
    ::LeaveCriticalSection(&_lock);

    if (!saved_objects.empty()) {

        const std::wstring saved_upload_file = _s->get_protected_profile_dir() + L"\\upload.json";

        for (auto it = saved_objects.begin(); it != saved_objects.end(); ++it) {
            try {
                NX::json_value object = NX::json_value::create_object();
                object[L"type"] = NX::json_value((int)(*it).get_type());
                object[L"checksum"] = NX::json_value((*it).get_checksum());
                object[L"data"] = NX::json_value((*it).get_data());
                objects_array.as_array().push_back(object);
            }
            catch (const std::exception& e) {
                UNREFERENCED_PARAMETER(e);
            }
        }

        const std::wstring& content = objects_array.serialize();

        if (!GLOBAL.nt_generate_file(saved_upload_file, NX::conversion::utf16_to_utf8(content), true)) {
            LOGCRITICAL(NX::string_formater(L"Fail to save upload objects to file (%d): %s", GetLastError(), saved_upload_file.c_str()));
            ::EnterCriticalSection(&_lock);
            std::for_each(saved_objects.begin(), saved_objects.end(), [&](const upload_object& o) {
                // failed? put object back to the queue
                _q.push(o);
            });
            ::LeaveCriticalSection(&_lock);
        }
    }
}

bool upload_serv::load_objects()
{
    bool result = false;
    const std::wstring upload_file = _s->get_protected_profile_dir() + L"\\upload.json";
    std::string content;

    if (!GLOBAL.nt_load_file(upload_file, content)) {
        return false;
    }

    NT::DeleteFile(upload_file.c_str());

    try {

        NX::json_value objects = NX::json_value::parse(content);
        const NX::json_array& objects_array = objects.as_array();
        result = (objects_array.size() != 0);
        std::for_each(objects_array.begin(), objects_array.end(), [&](const NX::json_value& object) {
            const int obj_type = object.as_object().at(L"type").as_int32();
            const std::wstring obj_checksum = object.as_object().at(L"checksum").as_string();
            const std::wstring obj_data = object.as_object().at(L"data").as_string();
            upload_object uo(obj_type, obj_checksum, obj_data);
            if (uo.is_known_type() && !uo.empty()) {
                ::EnterCriticalSection(&_lock);
                _q.push(uo);
                ::LeaveCriticalSection(&_lock);
            }
        });
    }
    catch (const std::exception& e) {
        UNREFERENCED_PARAMETER(e);
    }

    return result;
}

void upload_serv::worker(upload_serv* serv) noexcept
{
    while (serv->_b) {

        // wait
        unsigned wait_result = ::WaitForMultipleObjects(2, serv->_events, FALSE, INFINITE);

        if (wait_result == WAIT_OBJECT_0) {
            // stop event
            LOGDEBUG(NX::string_formater(L"UPLOADSERV worker thread (%d) quit", GetCurrentThreadId()));
            break;
        }

        //
        if (wait_result != (WAIT_OBJECT_0 + 1)) {
            // error
            LOGWARNING(NX::string_formater(L"UPLOADSERV worker thread (%d) wait error (result = %d, error = %d)", GetCurrentThreadId(), wait_result, GetLastError()));
            break;
        }

        assert(wait_result == (WAIT_OBJECT_0 + 1));

        // try to load from disk
        serv->load_objects();

        // new request comes
        upload_object  object = serv->pop();
        if (object.empty()) {
            continue;
        }

        if (!serv->_s->upload_data(object)) {
            // error happened
            LOGDEBUG("Cannot upload, save all the data to disk ...");
            serv->save_objects();
            ResetEvent(serv->_events[1]);
        }
    }
}
