
#ifndef _NUDF_ZIP_HPP__
#define _NUDF_ZIP_HPP__

#include <string>
#include <vector>

namespace NX {
namespace ZIP {


bool zip(_In_ const std::wstring& source, _In_ const std::wstring& zipfile, _In_opt_ LPSECURITY_ATTRIBUTES sa);
bool unzip(_In_ const std::wstring& zipfile, _In_ const std::wstring& targetdir);

std::vector<unsigned char> compress_buffer(const void* data, unsigned long length);
std::vector<unsigned char> decompress_buffer(const void* compressed_data, unsigned long length, unsigned long estimated_decompress_size);

bool compress_to_file(const void* data, unsigned long length, const std::wstring& file, bool replace_existing);
bool decompress_to_file(const void* compressed_data, unsigned long length, unsigned long estimated_decompress_size, const std::wstring& file, bool replace_existing);

    
}   // namespace NX::zip
}   // namespace NX

#endif  // _NUDF_ZIP_HPP__