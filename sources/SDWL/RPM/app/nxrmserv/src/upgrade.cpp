

#include <Windows.h>
#include <assert.h>


#include <nudf/eh.hpp>
#include <nudf/debug.hpp>
#include <nudf/conversion.hpp>
#include <nudf/string.hpp>
#include <nudf/crypto.hpp>
#include <nudf/zip.hpp>
#include <nudf/filesys.hpp>

#include "nxrmserv.hpp"
#include "serv.hpp"
#include "global.hpp"
#include "upgrade.hpp"

#include <filesystem>

static std::wstring get_file_name_from_url(const std::wstring& url);
static bool verify_cab_file(const std::wstring& file, const std::wstring& expected_checksum);
static bool verify_msi_file(const std::wstring& file);
static BOOL system_reboot(LPTSTR lpMsg);

bool download_new_version(const std::wstring& downloadURL, const std::wstring& sha1Checksum, std::wstring& newInstaller)
{
    bool result = false;

    const std::wstring& tempfolder = GLOBAL.get_temp_folder_name(NX::win::get_windows_temp_directory());
    if (tempfolder.empty()) {
        return false;
    }

    do {

        const std::wstring download_file(tempfolder + L"\\" + get_file_name_from_url(downloadURL));
        const std::wstring msi_file(tempfolder + L"\\NextLabs Rights Management.msi");
        const std::wstring exe_file(tempfolder + L"\\Setup.exe");
        std::wstring installer_file;

        HRESULT hr = URLDownloadToFileW(NULL, downloadURL.c_str(), download_file.c_str(), 0, NULL);
        if (FAILED(hr) || INVALID_FILE_ATTRIBUTES == GetFileAttributesW(download_file.c_str())) {
            LOGERROR(NX::string_formater(L"AutoUpdate: Download failed (0x%08X)", hr));
            break;
        }

        if (!verify_cab_file(download_file, sha1Checksum)) {
            break;
        }

        if (!NX::ZIP::unzip(download_file, tempfolder)) {
            LOGERROR(NX::string_formater(L"AutoUpdate: Unzip file failed (%d)", GetLastError()));
            break;
        }

        // find the unzipped file, it can be "RMC.msi", or "Setup.exe"
        if (std::experimental::filesystem::exists(msi_file))
            installer_file = msi_file;
        else if (std::experimental::filesystem::exists(exe_file))
            installer_file = exe_file;
        else
            break;

        if (!verify_msi_file(installer_file)) {
            break;
        }

        newInstaller = installer_file;
        result = true;

    } while (false);

    if (!result) {
        NX::fs::remove_directory(tempfolder, true);
    }

    return result;
}

bool install_new_version(const std::wstring& newInstaller)
{
    std::wstring _installer = newInstaller;
    std::transform(_installer.begin(), _installer.end(), _installer.begin(), ::tolower);
    std::wstring msiexec_cmd;
    if (_installer.find(L"setup.exe") != _installer.npos)
    {
        msiexec_cmd = newInstaller;
        msiexec_cmd += L" /S /v\"/qn /norestart /l*v ";
        // The update.log cannot be put into RMC install folder because it will block installation process.
        msiexec_cmd += NX::win::get_windows_temp_directory();
        msiexec_cmd += L"\\NextLabs-RMC-Update.log\"";
    }
    else
    {
        // Good, start installer now
        msiexec_cmd = NX::win::get_system_directory();
        msiexec_cmd += L"\\msiexec.exe /i \"";
        msiexec_cmd += newInstaller;
        msiexec_cmd += L"\" /quiet /norestart /L*V \"";
        // The update.log cannot be put into RMC install folder because it will block installation process.
        msiexec_cmd += NX::win::get_windows_temp_directory();
        msiexec_cmd += L"\\NextLabs-RMC-Update.log\"";
    }

    NX::fs::dos_filepath full_path(newInstaller);
    // Create Process
    STARTUPINFOW si;
    PROCESS_INFORMATION pi;
    memset(&si, 0, sizeof(si));
    memset(&pi, 0, sizeof(pi));
    si.cb = sizeof(si);
    LOGINFO(L"AutoUpdate: Installing ...");
    if (!::CreateProcessW(NULL, (LPWSTR)msiexec_cmd.c_str(), NULL, NULL, FALSE, CREATE_SUSPENDED, NULL, full_path.file_dir().c_str(), &si, &pi)) {
        LOGERROR(NX::string_formater(L"AutoUpdate: Fail to initiate install process (%d)", GetLastError()));
        return false;
    }
    ResumeThread(pi.hThread);
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);

    // inform end user to restart machine
    //system_reboot(L"Please close and save your application data. System will be rebooted in 5 minutes as the key software is updated.");

    return true;
}

std::wstring get_file_name_from_url(const std::wstring& url)
{
    std::wstring::size_type pos = url.find_last_of(L"/\\");
    if (pos == std::wstring::npos) {
        return url;
    }

    return url.substr(pos + 1);
}

bool verify_cab_file(const std::wstring& file, const std::wstring& expected_checksum)
{
    bool result = false;
    HANDLE h = INVALID_HANDLE_VALUE;

    do {

        h = ::CreateFileW(file.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        if (INVALID_HANDLE_VALUE == h) {
            LOGERROR(NX::string_formater(L"AutoUpdate: Fail to open downloaded file (%d, %s)", GetLastError(), file.c_str()));
            break;
        }

        DWORD dwSize = ::GetFileSize(h, NULL);
        if (INVALID_FILE_SIZE == dwSize) {
            LOGERROR(NX::string_formater(L"AutoUpdate: Fail to get downloaded file size (%d, %s)", GetLastError(), file.c_str()));
            break;
        }

        if (0 == dwSize) {
            LOGERROR(NX::string_formater(L"AutoUpdate: Empty downloaded file (%s)", file.c_str()));
            break;
        }

        std::vector<unsigned char> buf;
        buf.resize(dwSize, 0);

        DWORD dwRead = 0;
        if (!::ReadFile(h, buf.data(), dwSize, &dwRead, NULL)) {
            LOGERROR(NX::string_formater(L"AutoUpdate: Fail to read downloaded file (%d, %s)", GetLastError(), file.c_str()));
            break;
        }
        if (dwSize != dwRead) {
            LOGERROR(NX::string_formater(L"AutoUpdate: Fail to read entire downloaded file (%d, %s)", GetLastError(), file.c_str()));
            break;
        }

        std::vector<unsigned char> checksum;
        checksum.resize(20, 0);
        if (!NX::crypto::sha1(buf.data(), (unsigned long)buf.size(), checksum.data())) {
            LOGERROR(NX::string_formater(L"AutoUpdate: Fail to calculate downloaded file's sha1 checksum (%d, %s)", GetLastError(), file.c_str()));
            break;
        }

        std::wstring str_checksum = NX::conversion::to_wstring(checksum);
        if (0 != _wcsicmp(str_checksum.c_str(), expected_checksum.c_str())) {
            LOGERROR(NX::string_formater(L"AutoUpdate: mismatch checksum of downloaded file (%s)", file.c_str()));
            break;
        }

        result = true;

    } while (false);

    if (INVALID_HANDLE_VALUE != h) {
        CloseHandle(h);
        h = INVALID_HANDLE_VALUE;
    }

    return result;
}

bool verify_msi_file(const std::wstring& file)
{
    NX::win::pe_file pef(file);

    if (0 != _wcsicmp(pef.get_image_publisher().c_str(), L"NextLabs Inc.")) {
        LOGERROR(NX::string_formater(L"AutoUpdate: fail to verify MSI signature (%s)", file.c_str()));
        return false;
    }

    return true;
}

BOOL system_reboot(LPTSTR lpMsg)
{
    HANDLE hToken;              // handle to process token 
    TOKEN_PRIVILEGES tkp;       // pointer to token structure 

    BOOL fResult;               // system shutdown flag 

                                // Get the current process token handle so we can get shutdown 
                                // privilege. 

    if (!OpenProcessToken(GetCurrentProcess(),
        TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken))
        return FALSE;

    // Get the LUID for shutdown privilege. 

    LookupPrivilegeValue(NULL, SE_SHUTDOWN_NAME,
        &tkp.Privileges[0].Luid);

    tkp.PrivilegeCount = 1;  // one privilege to set    
    tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

    // Get shutdown privilege for this process. 

    AdjustTokenPrivileges(hToken, FALSE, &tkp, 0,
        (PTOKEN_PRIVILEGES)NULL, 0);

    // Cannot test the return value of AdjustTokenPrivileges. 

    if (GetLastError() != ERROR_SUCCESS)
        return FALSE;

    // Display the shutdown dialog box and start the countdown. 

    fResult = InitiateSystemShutdown(
        NULL,    // shut down local computer 
        lpMsg,   // message for user
        300,      // time-out period, in seconds 
        TRUE,   // ask user to close apps 
        TRUE);   // reboot after shutdown 

    if (!fResult)
        return FALSE;

    // Disable shutdown privilege. 

    tkp.Privileges[0].Attributes = 0;
    AdjustTokenPrivileges(hToken, FALSE, &tkp, 0,
        (PTOKEN_PRIVILEGES)NULL, 0);

    return TRUE;
}
