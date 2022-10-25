

#ifndef __NXRM_UPLOAD_SERV__
#define __NXRM_UPLOAD_SERV__


#include <string>
#include <queue>
#include <thread>


#include "rsapi.hpp"

typedef enum UPLOADTYPE {
    UPLOAD_NONE = 0,
    UPLOAD_SHARING_TRANSITION,
    UPLOAD_MAX
} UPLOADTYPE;

class upload_object
{
public:
    upload_object();
    upload_object(int type, const std::wstring& checksum, const std::wstring& data);
    upload_object(const upload_object& other);
    upload_object(upload_object&& other);
    ~upload_object();

    upload_object& operator = (const upload_object& other);
    upload_object& operator = (upload_object&& other);
    void clear();


    bool is_known_type() const { return (_type > UPLOAD_NONE && _type < UPLOAD_MAX); }

    inline bool empty() const { return (UPLOAD_NONE == _type || _data.empty()); }
    inline UPLOADTYPE get_type() const { return (UPLOADTYPE)_type; }
    inline const std::wstring& get_checksum() const { return _checksum; }
    inline const std::wstring& get_data() const { return _data; }

private:
    int             _type;
    std::wstring    _checksum;
    std::wstring    _data;

    friend class upload_serv;
};

class rmsession;

class upload_serv
{
public:
    upload_serv();
    ~upload_serv();

    bool start();
    void stop();
    
    bool push(const upload_object& object);
    void check_queue();

    static void worker(upload_serv* serv) noexcept;

private:
    bool inter_push(const upload_object& object, bool notify);
    upload_object pop();

    bool upload_data(const upload_object& object);

    void save_objects();
    bool load_objects();

private:
    rmsession* _s;
    bool _b;
    std::thread _t;
    std::queue<upload_object> _q;
    CRITICAL_SECTION   _lock;
    HANDLE _events[2];  // 0: stop event, 1: request coming event
    NX::RS::executor _rs_executor;

    friend class rmsession;
};



#endif