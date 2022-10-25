

#ifndef _NXRM_UPGRADE_HPP__
#define _NXRM_UPGRADE_HPP__

#include <string>


bool download_new_version(const std::wstring& downloadURL, const std::wstring& sha1Checksum, std::wstring& newInstaller);
bool install_new_version(const std::wstring& newInstaller);

#endif