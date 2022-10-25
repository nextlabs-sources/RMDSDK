

#ifndef _NUDF_RESUTIL_HPP__
#define _NUDF_RESUTIL_HPP__


#include <Windows.h>
#include <assert.h>
#include <string>
#include <vector>

namespace NX {
namespace RES {

#define MAX_MESSAGE_LENGTH  2048

std::wstring LoadMessage(_In_ HMODULE module, _In_ UINT id, _In_ ULONG max_length, _In_ DWORD langid=LANG_NEUTRAL, _In_opt_ LPCWSTR default_msg=NULL);
std::wstring LoadMessageEx(_In_ HMODULE module, _In_ UINT id, _In_ ULONG max_length, _In_ DWORD langid, _In_opt_ LPCWSTR default_msg, ...);

bool extract_resource(_In_opt_ HINSTANCE hInstance, _In_ UINT id, _In_ LPCWSTR type, _Out_ unsigned char** ppdata, _Out_ unsigned long* psize);
bool extract_resource(_In_opt_ HINSTANCE hInstance, _In_ UINT id, _In_ LPCWSTR type, _In_ std::vector<unsigned char>& data);
bool extract_resource(_In_opt_ HINSTANCE hInstance, _In_ UINT id, _In_ LPCWSTR type, _In_ const std::wstring& file, _In_ DWORD file_attributes, _In_ bool replace_existing);


}   // namespace NX::RES
}   // namespace NX

#endif  // _NUDF_RESUTIL_HPP__