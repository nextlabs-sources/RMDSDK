

#ifndef __NKDF_BASIC_COMPRESS_H__
#define __NKDF_BASIC_COMPRESS_H__



_Check_return_
_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
NkCompressBuffer(
    _In_ const VOID* Data,
    _In_ ULONG DataLength,
    _Out_ VOID* CompressedBuffer,
    _In_ ULONG CompressedBufferSize,
    _Out_ PULONG CompressedDataLength
    );



_Check_return_
_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
NkDecompressBuffer(
    _In_ const VOID* CompressedData,
    _In_ ULONG CompressedDataLength,
    _In_ VOID* UncompressedBuffer,
    _In_ ULONG UncompressedBufferSize,
    _Out_ PULONG UncompressedDataLength
    );



#endif