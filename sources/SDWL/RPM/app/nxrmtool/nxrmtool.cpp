// nxrmtool.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <iostream>
#include <string>
#include <boost/algorithm/string.hpp>
#include "nlohmann/json.hpp"
#include "SDLInc.h"
#include "nudf/conversion.hpp"
#include "nudf/winutil.hpp"

typedef enum {
    CMD_ADD_RPM_DIR,
    CMD_REMOVE_RPM_DIR,
    CMD_IS_RPM_DIR,
#ifdef  NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR
    CMD_ADD_SANC_DIR,
    CMD_REMOVE_SANC_DIR,
    CMD_IS_SANC_DIR,
#endif  // NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR
} cmd_t;

const std::string ReadFromFile(std::string filename)
{
    std::string retstr;

    FILE * testfile;
    fopen_s(&testfile, filename.c_str(), "r");
    if (testfile == NULL) {
        return retstr;
    }

    char buffer[257];
    while (!feof(testfile)) {
        memset(buffer, 0, sizeof(buffer));
        size_t s = fread(buffer, sizeof(char), sizeof(buffer) - 1, testfile);
        retstr += buffer;
    }
    fclose(testfile);

    return retstr;
}

void printUsage(const wchar_t *cmd)
{
    wprintf(L"Usage:\n");
    wprintf(L"%s <command> [<args>...] <fullDirPath>\n", cmd);

    wprintf(L"%s addRPMDir [--overwrite] [--ext] [--autoProtect [--attr=<name>:<value>[,<name>:<value>]...]] <fullDirPath>\n", cmd);

    wprintf(L"%s removeRPMDir <fullDirPath>\n", cmd);
    wprintf(L"%s isRPMDir <fullDirPath>\n", cmd);
#ifdef  NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR
    wprintf(L"%s addSancDir [--attr=<name>:<value>[,<name>:<value>]...] <fullDirPath>\n", cmd);
    wprintf(L"%s removeSancDir <fullDirPath>\n", cmd);
    wprintf(L"%s isSancDir <fullDirPath>\n", cmd);
#endif  // NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR
}

std::string attrArgsToJSON(wchar_t *args)
{
    const wchar_t *key, *value;
    wchar_t *p;

    nlohmann::json root = nlohmann::json::object();

    key = wcstok_s(args, L":", &p);

    while (key != NULL) {
        value = wcstok_s(NULL, L",", &p);
        if (value == NULL) {
            return {};
        }
        root[NX::conversion::utf16_to_utf8(key)].push_back(NX::conversion::utf16_to_utf8(value));

        key = wcstok_s(NULL, L":", &p);
    }

    return root.dump();
}

int wmain(int argc, wchar_t *argv[])
{
    ISDRmcInstance * pInstance;
    ISDRmTenant *pTenant;
    ISDRmUser *pUser;
    SDWLResult res;

    if (argc < 2) {
        printUsage(argv[0]);
        return ERROR_SUCCESS;
    }
    const std::wstring cmdStr = argv[1];

    cmd_t cmd;
    bool op_overwrite = false;
    bool op_ext = false;

    bool op_autoProtect = false;
    std::string utf8TagsStr = "{}";
    std::wstring dirPath;

    if (boost::iequals(cmdStr, L"addRPMDir")) {
        cmd = CMD_ADD_RPM_DIR;

        int i;
        for (i = 2; i < argc && wcsncmp(argv[i], L"--", 2) == 0; i++) {
            if (_wcsicmp(argv[i] + 2, L"overwrite") == 0) {
                op_overwrite = true;
            } else if (_wcsicmp(argv[i] + 2, L"ext") == 0) {
                op_ext = true;
            } else if (_wcsicmp(argv[i] + 2, L"autoProtect") == 0) {
                op_autoProtect = true;

                const int j = i + 1;
                if (j < argc && _wcsnicmp(argv[j], L"--attr=", wcslen(L"--attr=")) == 0) {
                    utf8TagsStr = attrArgsToJSON(argv[j] + wcslen(L"--attr="));

                    if (utf8TagsStr.empty() || utf8TagsStr == "{}") {
                        printUsage(argv[0]);
                        return ERROR_INVALID_PARAMETER;
                    }

                    i++;
                }
            } else {
                printUsage(argv[0]);
                return ERROR_INVALID_PARAMETER;
            }
        }

        if (i != argc - 1) {
            printUsage(argv[0]);
            return ERROR_INVALID_PARAMETER;
        }
        dirPath = argv[i];
    } else if (boost::iequals(cmdStr, L"removeRPMDir")) {
        cmd = CMD_REMOVE_RPM_DIR;

        if (argc != 3 || wcsncmp(argv[2], L"--", 2) == 0) {
            printUsage(argv[0]);
            return ERROR_INVALID_PARAMETER;
        }
        dirPath = argv[2];
    } else if (boost::iequals(cmdStr, L"isRPMDir")) {
        cmd = CMD_IS_RPM_DIR;

        if (argc != 3 || wcsncmp(argv[2], L"--", 2) == 0) {
            printUsage(argv[0]);
            return ERROR_INVALID_PARAMETER;
        }
        dirPath = argv[2];
#ifdef  NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR
    } else if (boost::iequals(cmdStr, L"addSancDir")) {
        cmd = CMD_ADD_SANC_DIR;

        int i;
        for (i = 2; i < argc && wcsncmp(argv[i], L"--", 2) == 0; i++) {
            if (_wcsnicmp(argv[i] + 2, L"attr=", wcslen(L"attr=")) == 0) {
                utf8TagsStr = attrArgsToJSON(argv[i] + 2 + wcslen(L"attr="));

                if (utf8TagsStr.empty() || utf8TagsStr == "{}") {
                    printUsage(argv[0]);
                    return ERROR_INVALID_PARAMETER;
                }
            } else {
                printUsage(argv[0]);
                return ERROR_INVALID_PARAMETER;
            }
        }

        if (i != argc - 1) {
            printUsage(argv[0]);
            return ERROR_INVALID_PARAMETER;
        }
        dirPath = argv[i];
    } else if (boost::iequals(cmdStr, L"removeSancDir")) {
        cmd = CMD_REMOVE_SANC_DIR;

        if (argc != 3 || wcsncmp(argv[2], L"--", 2) == 0) {
            printUsage(argv[0]);
            return ERROR_INVALID_PARAMETER;
        }
        dirPath = argv[2];
    } else if (boost::iequals(cmdStr, L"isSancDir")) {
        cmd = CMD_IS_SANC_DIR;

        if (argc != 3 || wcsncmp(argv[2], L"--", 2) == 0) {
            printUsage(argv[0]);
            return ERROR_INVALID_PARAMETER;
        }
        dirPath = argv[2];
#endif  // NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR
    } else {
        printUsage(argv[0]);
        return ERROR_BAD_COMMAND;
    }



    SDWLibInit();

    //std::string security = ReadFromFile("security.txt");
    std::string security = "{6829b159-b9bb-42fc-af19-4a6af3c9fcf6}";

    res = RPMGetCurrentLoggedInUser(security, pInstance, pTenant, pUser);
    if (!res) {
        wprintf(L"Error, reason: %s\n", res.ToString().c_str());
        SDWLibCleanup();
        return res.GetCode();
    }

    const std::wstring appPath = NX::win::get_current_process_path();
    res = pInstance->RPMRegisterApp(appPath, security);
    res = pInstance->RPMNotifyRMXStatus(true, security);

    switch (cmd) {
        case CMD_ADD_RPM_DIR:
        {
            uint32_t option = SDRmRPMFolderOption::RPMFOLDER_NORMAL | SDRmRPMFolderOption::RPMFOLDER_API;

            if (op_autoProtect) {
                option |= SDRmRPMFolderOption::RPMFOLDER_AUTOPROTECT;
            }

            if (op_overwrite) {
                option |= SDRmRPMFolderOption::RPMFOLDER_OVERWRITE;
            }

            if (op_ext) {
                option |= SDRmRPMFolderOption::RPMFOLDER_EXT;
            }

            res = pInstance->AddRPMDir(dirPath, option, utf8TagsStr);
            break;
        }

        case CMD_REMOVE_RPM_DIR:
            res = pInstance->RemoveRPMDir(dirPath);
            break;

        case CMD_IS_RPM_DIR:
        {
            uint32_t dirStatus;

            SDRmRPMFolderOption option;
            std::wstring fileTags;
            res = pInstance->IsRPMFolder(dirPath, &dirStatus, &option, fileTags);

            if (res) {
                wprintf(L"RPM dir:\t\t%s\n",
                        dirStatus & RPM_SAFEDIRRELATION_SAFE_DIR ? L"true" : L"false");
                wprintf(L"Ancestor of RPM dir:\t%s\n",
                        dirStatus & RPM_SAFEDIRRELATION_ANCESTOR_OF_SAFE_DIR ? L"true" : L"false");
                wprintf(L"Descendant of RPM dir:\t%s\n",
                        dirStatus & RPM_SAFEDIRRELATION_DESCENDANT_OF_SAFE_DIR ? L"true" : L"false");


                if (dirStatus & (RPM_SAFEDIRRELATION_SAFE_DIR | RPM_SAFEDIRRELATION_DESCENDANT_OF_SAFE_DIR)) {
                    wprintf(L"AutoProtect:\t\t%s\n", option & SDRmRPMFolderOption::RPMFOLDER_AUTOPROTECT ? L"true" : L"false");
                    wprintf(L"Overwrite:\t\t%s\n", option & SDRmRPMFolderOption::RPMFOLDER_OVERWRITE ? L"true" : L"false");
                    wprintf(L"Ext:\t\t\t%s\n", option & SDRmRPMFolderOption::RPMFOLDER_EXT ? L"true" : L"false");

                    if (option & SDRmRPMFolderOption::RPMFOLDER_AUTOPROTECT) {
                        wprintf(L"fileTags=\"%s\"\n", fileTags.c_str());
                    }
                }
            }

            break;
        }

#ifdef  NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR
        case CMD_ADD_SANC_DIR:
            res = pInstance->AddSanctuaryDir(dirPath, utf8TagsStr);
            break;

        case CMD_REMOVE_SANC_DIR:
            res = pInstance->RemoveSanctuaryDir(dirPath);
            break;

        case CMD_IS_SANC_DIR:
        {
            uint32_t dirStatus;
            std::wstring fileTags;
            res = pInstance->IsSanctuaryFolder(dirPath, &dirStatus, fileTags);
            if (res) {
                wprintf(L"Sanctuary dir:\t\t\t%s\n",
                        dirStatus & RPM_SANCTUARYDIRRELATION_SANCTUARY_DIR ? L"true" : L"false");
                wprintf(L"Ancestor of sanctuary dir:\t%s\n",
                        dirStatus & RPM_SANCTUARYDIRRELATION_ANCESTOR_OF_SANCTUARY_DIR ? L"true" : L"false");
                wprintf(L"Descendant of sanctuary dir:\t%s\n",
                        dirStatus & RPM_SANCTUARYDIRRELATION_DESCENDANT_OF_SANCTUARY_DIR ? L"true" : L"false");

                if (dirStatus & (RPM_SANCTUARYDIRRELATION_SANCTUARY_DIR | RPM_SANCTUARYDIRRELATION_DESCENDANT_OF_SANCTUARY_DIR)) {
                    wprintf(L"fileTags=\"%s\"\n", fileTags.c_str());
                }
            }

            break;
        }
#endif  // NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR
    }

    if (res) {
        wprintf(L"Success\n");
    } else {
        wprintf(L"Error, reason: %s\n", res.ToString().c_str());
    }

    res = pInstance->RPMNotifyRMXStatus(false, security);
    res = pInstance->RPMUnregisterApp(appPath, security);

    SDWLibDeleteRmcInstance(pInstance);

    SDWLibCleanup();

    return ERROR_SUCCESS;
}
