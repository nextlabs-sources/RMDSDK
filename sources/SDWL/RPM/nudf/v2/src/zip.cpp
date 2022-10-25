

#include <Windows.h>
#include <Shldisp.h>
#include <assert.h>
#include <AccCtrl.h>
#include <Aclapi.h>

#include <atlbase.h>
#include <atlcomcli.h>

#include <nudf\eh.hpp>
#include <nudf\zip.hpp>
#include <nudf\filesys.hpp>
#include <nudf\handyutil.hpp>

using namespace NX;

static bool create_empty_zip(_In_ const std::wstring& zipfile, _In_opt_ LPSECURITY_ATTRIBUTES sa);
static bool read_file(const std::wstring& file, std::vector<unsigned char>& buf);
bool write_file(const std::wstring& file, const std::vector<unsigned char>& buf, bool replace);

bool ZIP::zip(_In_ const std::wstring& source, _In_ const std::wstring& zipfile, _In_opt_ LPSECURITY_ATTRIBUTES sa)
{
    bool result = false;

    CoInitialize(NULL);

    do {

        HRESULT hr;
        CComPtr<IShellDispatch> spISD;
        CComPtr<Folder>         spToFolder;
        CComVariant vSource;
        CComVariant vZipFile;
        CComVariant vOpt;
        
        if(!create_empty_zip(zipfile, sa)) {
            return false;
        }

        vZipFile.vt = VT_BSTR;
        auto bstr1 = ::SysAllocStringLen(NULL, (UINT)zipfile.length() + 3);
        vZipFile.bstrVal = bstr1;
        RtlSecureZeroMemory(vZipFile.bstrVal, sizeof(WCHAR)*(zipfile.length() + 3));
        memcpy(vZipFile.bstrVal, zipfile.c_str(), sizeof(WCHAR)*zipfile.length());

        vSource.vt = VT_BSTR;
        auto bstr2 = ::SysAllocStringLen(NULL, (UINT)source.length() + 3);
        vSource.bstrVal = bstr2;
        RtlSecureZeroMemory(vSource.bstrVal, sizeof(WCHAR)*(source.length() + 3));
        memcpy(vSource.bstrVal, source.c_str(), sizeof(WCHAR)*source.length());

        vOpt.vt = VT_I4;
        vOpt.lVal = 0x0614; //FOF_NO_UI;  //Do not display a progress dialog box, not useful in compression
        
        hr = ::CoCreateInstance(CLSID_Shell, NULL, CLSCTX_INPROC_SERVER, IID_IShellDispatch, (void **)&spISD);
        if (SUCCEEDED(hr) && NULL!=spISD.p) {

            // Destination is our zip file
            hr= spISD->NameSpace(vZipFile, &spToFolder);
            if (SUCCEEDED(hr) && NULL != spToFolder.p) {
                
                // Copying and compressing the source files to our zip
                hr = spToFolder->CopyHere(vSource, vOpt);
                result = SUCCEEDED(hr) ? true : false;

                // CopyHere() creates a separate thread to copy files and 
                // it may happen that the main thread exits before the 
                // copy thread is initialized. So we put the main thread to sleep 
                // for a second to give time for the copy thread to start.
                if(result) {
                    Sleep(1000);
                    // Done
                    spToFolder.Release();
                    int count = 0;
                    while (!NX::fs::IsFileClosed(zipfile) && count < 20)
                    {
                        Sleep(1000);
                        spToFolder.Release();
                        count++;
                    }
                    (void)NX::fs::open_file_exclusively(zipfile, INFINITE);
                    vOpt.Clear();
                }
            }

            spISD.Release();
            SysFreeString(bstr1);
            vZipFile.Clear();
            SysFreeString(bstr2);
            vSource.Clear();
            result = true;
        }
    } while (FALSE);

    CoUninitialize();
    if (!result) {
        ::DeleteFileW(zipfile.c_str());
    }

    return result;
}

bool ZIP::unzip(_In_ const std::wstring& zipfile, _In_ const std::wstring& targetdir)
{
    bool result = false;

    CoInitialize(NULL);

    do {

        HRESULT hr;
        CComPtr<IShellDispatch> spISD;
        CComPtr<Folder>         spZipFolder;
        CComPtr<Folder>         spToFolder;
        CComVariant vTarget;
        CComVariant vZipFile;
        CComVariant vOpt;
        
        vZipFile.vt = VT_BSTR;
        vZipFile.bstrVal = ::SysAllocStringLen(NULL, (UINT)zipfile.length() + 3);
        RtlSecureZeroMemory(vZipFile.bstrVal, sizeof(WCHAR)*(zipfile.length() + 3));
        memcpy(vZipFile.bstrVal, zipfile.c_str(), sizeof(WCHAR)*zipfile.length());

        vTarget.vt = VT_BSTR;
        vTarget.bstrVal = ::SysAllocStringLen(NULL, (UINT)targetdir.length() + 3);
        RtlSecureZeroMemory(vTarget.bstrVal, sizeof(WCHAR)*(targetdir.length() + 3));
        memcpy(vTarget.bstrVal, targetdir.c_str(), sizeof(WCHAR)*targetdir.length());

        vOpt.vt = VT_I4;
        vOpt.lVal = 0x0614; //FOF_NO_UI;  //Do not display a progress dialog box, not useful in compression
        
        hr = ::CoCreateInstance(CLSID_Shell, NULL, CLSCTX_INPROC_SERVER, IID_IShellDispatch, (void **)&spISD);
        if (SUCCEEDED(hr) && NULL!=spISD.p) {

            // Source is our zip file
            hr= spISD->NameSpace(vZipFile, &spZipFolder);
            if (SUCCEEDED(hr) && NULL != spZipFolder.p) {

                CComPtr<FolderItems> spZipItems;

                hr = spZipFolder->Items(&spZipItems);
                if(SUCCEEDED(hr) && NULL != spZipItems) {

                    hr= spISD->NameSpace(vTarget, &spToFolder);
                    if (SUCCEEDED(hr) && NULL != spToFolder.p) {
                        // Creating a new Variant with pointer to FolderItems to be copied
                        VARIANT newV;
                        VariantInit(&newV);
                        newV.vt = VT_DISPATCH;
                        newV.pdispVal = spZipItems.p;
                        hr = spToFolder->CopyHere(newV, vOpt);
                        result = SUCCEEDED(hr) ? true : false;
                        // Done
                        if(result) {
                            Sleep(1000);
                        }

                        spToFolder.Release();
                    }

                    spZipItems.Release();
                }

                spZipFolder.Release();
            }

            spISD.Release();
        }
    } while (FALSE);

    CoUninitialize();

    return result;
}

unsigned long get_compress_workspace_size()
{
    typedef LONG(NTAPI* RTLGETCOMPRESSIONWORKSPACESIZE)(_In_ USHORT, _Out_ PULONG, _Out_ PULONG);
    static unsigned long workspace_size = 0;
    static RTLGETCOMPRESSIONWORKSPACESIZE RtlGetCompressionWorkSpaceSize = NULL;

    if (0 == workspace_size) {

        if (NULL == RtlGetCompressionWorkSpaceSize) {
            RtlGetCompressionWorkSpaceSize = (RTLGETCOMPRESSIONWORKSPACESIZE)GetProcAddress(::GetModuleHandleW(L"ntdll.dll"), "RtlGetCompressionWorkSpaceSize");
            if (NULL == RtlGetCompressionWorkSpaceSize) {
                SetLastError(ERROR_INVALID_FUNCTION);
                return 0;
            }
        }
        assert(NULL != RtlGetCompressionWorkSpaceSize);
        unsigned long fragment_workspace_size = 0;
        LONG status = RtlGetCompressionWorkSpaceSize(COMPRESSION_FORMAT_LZNT1 | COMPRESSION_ENGINE_STANDARD, &workspace_size, &fragment_workspace_size);
        if (0 != status) {
            workspace_size = 0;
        }
    }

    return workspace_size;
}

#define STATUS_BUFFER_TOO_SMALL         (0xC0000023L)
#define STATUS_BAD_COMPRESSION_BUFFER   (0xC0000242L)

std::vector<unsigned char> NX::ZIP::compress_buffer(const void* data, unsigned long length)
{
    typedef LONG(NTAPI* RTLCOMPRESSBUFFER)(
        _In_  USHORT CompressionFormatAndEngine,
        _In_  PUCHAR UncompressedBuffer,
        _In_  ULONG  UncompressedBufferSize,
        _Out_ PUCHAR CompressedBuffer,
        _In_  ULONG  CompressedBufferSize,
        _In_  ULONG  UncompressedChunkSize,
        _Out_ PULONG FinalCompressedSize,
        _In_  PVOID  WorkSpace
        );
    static RTLCOMPRESSBUFFER RtlCompressBuffer = NULL;

    if (NULL == RtlCompressBuffer) {
        RtlCompressBuffer = (RTLCOMPRESSBUFFER)GetProcAddress(::GetModuleHandleW(L"ntdll.dll"), "RtlCompressBuffer");
        if (NULL == RtlCompressBuffer) {
            SetLastError(ERROR_INVALID_FUNCTION);
            return std::vector<unsigned char>();
        }
    }
    assert(NULL != RtlCompressBuffer);

    std::vector<unsigned char> compressed_buf;
    std::vector<unsigned char> workspace_buf;
    unsigned long compress_buffer_size = RoundToSize32(length, 4096);
    const unsigned long workspace_size = get_compress_workspace_size();

    if (0 == workspace_size) {
        return compressed_buf;
    }

    workspace_buf.resize(workspace_size, 0);
    compressed_buf.resize(compress_buffer_size, 0);

    do {

        unsigned long final_compressed_size = 0;

        LONG status = RtlCompressBuffer(COMPRESSION_FORMAT_LZNT1 | COMPRESSION_ENGINE_STANDARD,
            (PUCHAR)data,
            length,
            compressed_buf.data(),
            (ULONG)compressed_buf.size(),
            4096,
            &final_compressed_size,
            workspace_buf.data()
            );

        if (STATUS_BUFFER_TOO_SMALL == status) {

            if (compress_buffer_size >= 0xFFFFEFFF) {
                compressed_buf.clear();
                SetLastError(ERROR_BUFFER_OVERFLOW);
                break;
            }

            compress_buffer_size += 4096;
            compressed_buf.resize(compress_buffer_size, 0);
            continue;
        }
        
        switch (status)
        {
        case 0:
            SetLastError(0);
            break;
        case STATUS_BAD_COMPRESSION_BUFFER:
            SetLastError(ERROR_BAD_COMPRESSION_BUFFER);
            compressed_buf.clear();
            final_compressed_size = 0;
            break;
        default:
            SetLastError(status);
            compressed_buf.clear();
            final_compressed_size = 0;
            break;
        }

        // Error or Succeed, don't continue
        if (final_compressed_size != (ULONG)compressed_buf.size()) {
            assert(final_compressed_size < (ULONG)compressed_buf.size());
            compressed_buf.resize(final_compressed_size);
        }
        break;

    } while (TRUE);
    
    // Finally
    return std::move(compressed_buf);
}

std::vector<unsigned char> NX::ZIP::decompress_buffer(const void* compressed_data, unsigned long length, unsigned long estimated_decompress_size)
{
    typedef LONG(NTAPI* RTLDECOMPRESSBUFFER)(
        _In_  USHORT CompressionFormat,
        _Out_ PUCHAR UncompressedBuffer,
        _In_  ULONG  UncompressedBufferSize,
        _In_  PUCHAR CompressedBuffer,
        _In_  ULONG  CompressedBufferSize,
        _Out_ PULONG FinalUncompressedSize
        );
    static RTLDECOMPRESSBUFFER RtlDecompressBuffer = NULL;

    std::vector<unsigned char> uncompressed_buf;

    if (NULL == RtlDecompressBuffer) {
        RtlDecompressBuffer = (RTLDECOMPRESSBUFFER)GetProcAddress(::GetModuleHandleW(L"ntdll.dll"), "RtlDecompressBuffer");
        if (NULL == RtlDecompressBuffer) {
            SetLastError(ERROR_INVALID_FUNCTION);
            return uncompressed_buf;
        }
    }
    assert(NULL != RtlDecompressBuffer);


    unsigned long uncompress_buffer_size = RoundToSize32(estimated_decompress_size, 4096);
    uncompressed_buf.resize(uncompress_buffer_size, 0);

    do {

        unsigned long final_uncompressed_length = 0;

        LONG status = RtlDecompressBuffer(COMPRESSION_FORMAT_LZNT1, uncompressed_buf.data(), (unsigned long)uncompressed_buf.size(), (PUCHAR)compressed_data, length, &final_uncompressed_length);
        
        if (STATUS_BUFFER_TOO_SMALL == status) {

            if (uncompress_buffer_size >= 0xFFFFEFFF) {
                uncompressed_buf.clear();
                SetLastError(ERROR_BUFFER_OVERFLOW);
                break;
            }

            uncompress_buffer_size += 4096;
            uncompressed_buf.resize(uncompress_buffer_size, 0);
            continue;
        }

        switch (status)
        {
        case 0:
            SetLastError(0);
            break;
        case STATUS_BAD_COMPRESSION_BUFFER:
            SetLastError(ERROR_BAD_COMPRESSION_BUFFER);
            uncompressed_buf.clear();
            final_uncompressed_length = 0;
            break;
        default:
            SetLastError(status);
            uncompressed_buf.clear();
            final_uncompressed_length = 0;
            break;
        }

        if (final_uncompressed_length < (unsigned long)uncompressed_buf.size()) {
            uncompressed_buf.resize(final_uncompressed_length);
        }
        break;

    } while (uncompress_buffer_size <= 0xFFFFFFFF);

    return std::move(uncompressed_buf);
}

bool NX::ZIP::compress_to_file(const void* data, unsigned long length, const std::wstring& file, bool replace_existing)
{
    const std::vector<unsigned char>& buf = NX::ZIP::compress_buffer(data, length);
    if (buf.empty()) {
        return false;
    }

    return write_file(file, buf, replace_existing);
}

bool NX::ZIP::decompress_to_file(const void* compressed_data, unsigned long length, unsigned long estimated_decompress_size, const std::wstring& file, bool replace_existing)
{
    const std::vector<unsigned char>& buf = NX::ZIP::decompress_buffer(compressed_data, length, estimated_decompress_size);
    if (buf.empty()) {
        return false;
    }

    return write_file(file, buf, replace_existing);
}


bool read_file(const std::wstring& file, std::vector<unsigned char>& buf)
{
    HANDLE h = INVALID_HANDLE_VALUE;
    h = ::CreateFileW(file.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (INVALID_HANDLE_VALUE == h) {
        return false;
    }

    DWORD file_size = GetFileSize(h, NULL);
    if (INVALID_FILE_SIZE == file_size) {
        CloseHandle(h);
        return false;
    }

    if (0 == file_size) {
        CloseHandle(h);
        return true;
    }

    buf.resize(file_size, 0);

    DWORD bytes_read = 0;
    if (!::ReadFile(h, buf.data(), file_size, &bytes_read, NULL)) {
        CloseHandle(h);
        buf.clear();
        return false;
    }
    CloseHandle(h); h = INVALID_HANDLE_VALUE;

    if (file_size > bytes_read) {
        buf.clear();
        return false;
    }

    return true;
}

bool write_file(const std::wstring& file, const std::vector<unsigned char>& buf, bool replace)
{
    HANDLE h = INVALID_HANDLE_VALUE;
    h = ::CreateFileW(file.c_str(), GENERIC_READ | GENERIC_WRITE, 0, NULL, replace ? CREATE_ALWAYS : CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
    if (INVALID_HANDLE_VALUE == h) {
        return false;
    }

    DWORD bytes_written = 0;
    if (!::WriteFile(h, buf.data(), (ULONG)buf.size(), &bytes_written, NULL)) {
        CloseHandle(h);
        return false;
    }
    CloseHandle(h); h = INVALID_HANDLE_VALUE;

    return true;
}

bool create_empty_zip(_In_ const std::wstring& zipfile, _In_opt_ LPSECURITY_ATTRIBUTES sa)
{
    static const UCHAR emptyZip[] = {80, 75, 5, 6, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    HANDLE hFile = INVALID_HANDLE_VALUE;
    DWORD dwWritten = 0;

    hFile = ::CreateFileW(zipfile.c_str(), GENERIC_READ|GENERIC_WRITE, 0, sa, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if(INVALID_HANDLE_VALUE == hFile) {
        return false;
    }

    if(!WriteFile(hFile, emptyZip, sizeof(emptyZip), &dwWritten, NULL)) {
        CloseHandle(hFile);
        ::DeleteFileW(zipfile.c_str());
        return false;
    }

    CloseHandle(hFile);
    return true;
}