

#ifndef __LOG_ACTIVITY_HPP__
#define __LOG_ACTIVITY_HPP__


#include <string>
#include <list>

#include <boost/noncopyable.hpp>
// Forward class rmsession

class rmsession;


#define ACTIVITY_PROTECT        L"protect"
#define ACTIVITY_SHARE          L"share"
#define ACTIVITY_VIEW           L"view"
#define ACTIVITY_PRINT          L"print"
#define ACTIVITY_DOWNLOAD       L"download"
#define ACTIVITY_EDIT           L"edit/save"
#define ACTIVITY_MODIFY_SHARE   L"modify_share"
#define ACTIVITY_REVOKE         L"revoke"
#define ACTIVITY_DECRYPT        L"decrypt"

typedef enum ACTIVITY_OPERATION {
    ActProtect = 1,
    ActShare,
    ActModifyShare,
    ActView,
    ActPrint,
    ActDownload,
    ActEdit,
    ActRevoke,
    ActDecrypt,
    ActCopyContent,
    ActCaptureScreen,
    ActClassify
} ACTIVITY_OPERATION;

typedef enum ACTIVITY_RESULT {
    ActDenied = 0,
    ActAllowed
} ACTIVITY_RESULT;

class activity_record
{
public:
    activity_record();
    activity_record(const std::vector<unsigned char>& duid,
        const std::wstring& owner_id,
        __int64 user_id,
        int operation,
        int result,
        const std::wstring& file_path,
        const std::wstring& app_path,
        const std::wstring& app_publisher,
        const std::wstring& extra_data);
    activity_record(const activity_record& other);
    activity_record(activity_record&& other);
    ~activity_record();

    inline bool empty() const { return _s.empty(); }
    inline void clear() { _s.clear(); }
    inline const std::string& get_log() const { return _s; }

    activity_record& operator = (const activity_record& other);
    activity_record& operator = (activity_record&& other);

private:
    std::string _s;
};

class activity_logger : public boost::noncopyable
{
public:
    activity_logger(rmsession* p);
    ~activity_logger();

    bool init();
    void clear();

    unsigned long push(const activity_record& record);
    void upload_log();

protected:
    bool save_to_file(const unsigned char* logdata, const unsigned long size);
    bool load_from_file(std::vector<unsigned char>& logdata);
    bool clear_file();
    unsigned long get_count();

private:
    rmsession*  _rmsession;
    HANDLE _h;
    ULONG _count;
    CRITICAL_SECTION _lock;

};

class metadata_record
{
public:
	metadata_record();
	metadata_record(const std::wstring& duid,
		const std::wstring& otp,
		const std::wstring& adhoc,
		const std::wstring& tags);
	metadata_record(const std::wstring& duid,
		const int projectid,
		const std::wstring& params,
		const int type); // 0: new token; 1: update token for tenant; 2: update token for project
	metadata_record(const metadata_record& other);
	metadata_record(metadata_record&& other);
	~metadata_record();

	inline bool empty() const { return _s.empty(); }
	inline void clear() { _s.clear(); }
	inline const std::string& get_log() const { return _s; }

	metadata_record& operator = (const metadata_record& other);
	metadata_record& operator = (metadata_record&& other);

private:
	std::string _s;
};

class metadata_logger : public boost::noncopyable
{
public:
	metadata_logger(rmsession* p);
	~metadata_logger();

	bool init();
	void clear();

	unsigned long push(const metadata_record& record);
	void upload_log();

protected:
	bool save_to_file(const unsigned char* logdata, const unsigned long size);
	bool load_from_file(std::vector<unsigned char>& logdata);
	bool load_from_file(std::vector<std::string>& logdata);
	bool clear_file();
	unsigned long get_count();

private:
	rmsession * _rmsession;
	HANDLE _h;
	ULONG _count;
	CRITICAL_SECTION _lock;

};

#endif