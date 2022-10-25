

#include <ntifs.h>

#include "nkdf/basic/defines.h"
#include "nkdf/basic/compress.h"


_Check_return_
_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
NkGetCompressionWorkSpaceSize(
    _Out_ PULONG CompressBufferWorkSpaceSize
    );



//  Assign text sections for each routine.
#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, NkGetCompressionWorkSpaceSize)
#pragma alloc_text(PAGE, NkCompressBuffer)
#pragma alloc_text(PAGE, NkDecompressBuffer)
#endif





_Check_return_
_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
NkCompressBuffer(
    _In_ const VOID* Data,
    _In_ ULONG DataLength,
    _Out_writes_bytes_(CompressedBufferSize) VOID* CompressedBuffer,
    _In_ ULONG CompressedBufferSize,
    _Out_ PULONG CompressedDataLength
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    PVOID WorkSpace = NULL;

    PAGED_CODE();

    try {

        ULONG CompressBufferWorkSpaceSize = 0;

        Status = NkGetCompressionWorkSpaceSize(&CompressBufferWorkSpaceSize);
        if (!NT_SUCCESS(Status)) {
            try_return(Status);
        }

        WorkSpace = ExAllocatePoolWithTag(PagedPool, CompressBufferWorkSpaceSize, TAG_TEMP);
        if (NULL == WorkSpace) {
            try_return(Status = STATUS_INSUFFICIENT_RESOURCES);
        }

        Status = RtlCompressBuffer(COMPRESSION_FORMAT_LZNT1 | COMPRESSION_ENGINE_STANDARD,
            (PUCHAR)Data,
            DataLength,
            (PUCHAR)CompressedBuffer,
            CompressedBufferSize,
            4096,
            CompressedDataLength,
            WorkSpace
            );


    try_exit: NOTHING;
    }
    finally {

        if (NULL != WorkSpace) {
            ExFreePool(WorkSpace);
            WorkSpace = NULL;
        }
    }

    return Status;
}



_Check_return_
_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
NkDecompressBuffer(
    _In_ const VOID* CompressedData,
    _In_ ULONG CompressedDataLength,
    _In_ VOID* UncompressedBuffer,
    _In_ ULONG UncompressedBufferSize,
    _Out_ PULONG UncompressedDataLength
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    PVOID WorkSpace = NULL;

    PAGED_CODE();

    try {

        ULONG CompressBufferWorkSpaceSize = 0;

        Status = NkGetCompressionWorkSpaceSize(&CompressBufferWorkSpaceSize);
        if (!NT_SUCCESS(Status)) {
            try_return(Status);
        }

        Status = RtlDecompressBuffer(COMPRESSION_FORMAT_LZNT1,
            (PUCHAR)UncompressedBuffer,
            UncompressedBufferSize,
            (PUCHAR)CompressedData,
            CompressedDataLength,
            UncompressedDataLength
            );


    try_exit: NOTHING;
    }
    finally {
        NOTHING;
    }

    return Status;
}







//////////////////////////////////////////////////////////////////////////
// Local
_Check_return_
_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
NkGetCompressionWorkSpaceSize(
    _Out_ PULONG CompressBufferWorkSpaceSize
    )
{
    static ULONG GlobalCompressBufferWorkSpaceSize = 0;

    if (0 == GlobalCompressBufferWorkSpaceSize) {

        ULONG CompressFragmentWorkSpaceSize = 0;
        NTSTATUS Status = RtlGetCompressionWorkSpaceSize(COMPRESSION_FORMAT_LZNT1 | COMPRESSION_ENGINE_STANDARD, &GlobalCompressBufferWorkSpaceSize, &CompressFragmentWorkSpaceSize);
        if (!NT_SUCCESS(Status)) {
            GlobalCompressBufferWorkSpaceSize = 0;
            return Status;
        }
    }

    ASSERT(0 != GlobalCompressBufferWorkSpaceSize);

    *CompressBufferWorkSpaceSize = GlobalCompressBufferWorkSpaceSize;
    return STATUS_SUCCESS;
}