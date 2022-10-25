

#ifndef __NXRM_DIAG_HPP__
#define __NXRM_DIAG_HPP__

#include <string>
#include <vector>

#include <nudf/json.hpp>

class find_data : public WIN32_FIND_DATAW
{
public:
    find_data()
    {
    }
    find_data(const find_data& other)
    {
        if (this != &other) {
            memcpy(this, &other, sizeof(WIN32_FIND_DATAW));
        }
    }
    ~find_data()
    {
    }

    void clear()
    {
        memset(this, 0, sizeof(WIN32_FIND_DATAW));
    }

    find_data& operator = (const find_data& other)
    {
        if (this != &other) {
            memcpy(this, &other, sizeof(WIN32_FIND_DATAW));
        }
        return *this;
    }

    inline bool empty() const { return (0 == cFileName[0]); }
    inline __int64 get_filesize() const {
        __int64 n = nFileSizeHigh;
        n <<= 32;
        n += nFileSizeLow;
        return n;
    }
    inline bool is_dir() const {
        return (!empty() && (FILE_ATTRIBUTE_DIRECTORY == (dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)));
    }
};

class diagtool
{
public:
    diagtool();
    ~diagtool();

    bool generate(const std::wstring& user_sid, const std::wstring& tenant_id, const std::wstring& user_id, const std::wstring& target_dir, std::wstring& dbgfile);
    
protected:
    void dump_system_summary(); 
    void dump_services_summary(); 
    void dump_software_summary();
    void dump_rmc_summary();
    void dump_rmc_debuglog();
    void dump_rmc_user_profiles();
	//collects windows system, security, setup and Application logs
	void collect_windows_event_logs();
protected:
    void create_files_report(NX::json_value& parent, const std::wstring& file);
    NX::json_value create_files_report(const std::vector<std::wstring>& file);
    NX::json_value directory_report(const std::wstring& dir);
    NX::json_value file_report(const std::wstring& file, const find_data& wfd);

    void report_os_info(NX::json_value& v);
    void report_processor_info(NX::json_value& v);
    void report_memory_info(NX::json_value& v);
    void report_network_adapters_info(NX::json_value& v);
    void report_drives_info(NX::json_value& v);
    void report_processes_info(NX::json_value& v);
    void report_services_info(NX::json_value& v);
    void report_software_info(NX::json_value& v);
    void report_software_info2(NX::json_value& v, bool is_wow64);

    std::wstring build_file_attributes(DWORD dwFileAttributes);
    std::wstring build_file_time(const FILETIME* ft);

private:
    std::wstring    _tempdir;
    std::wstring    _user_sid;
    std::wstring    _tenant_id;
    std::wstring    _user_id;
};



#endif