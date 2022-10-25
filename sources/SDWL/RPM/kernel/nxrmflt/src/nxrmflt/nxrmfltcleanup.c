#include "nxrmfltdef.h"
#include "nxrmfltnxlcachemgr.h"
#include "nxrmfltutils.h"
#include "nxrmfltcleanup.h"

extern DECLSPEC_CACHEALIGN ULONG				gTraceFlags;
extern DECLSPEC_CACHEALIGN NXRMFLT_GLOBAL_DATA	Global;

extern LPSTR PsGetProcessImageFileName(PEPROCESS  Process);

extern BOOLEAN IsNXLFile(PUNICODE_STRING Extension);

extern BOOLEAN is_explorer(void);
extern NTSTATUS build_nxlcache_file_name(NXL_CACHE_NODE *pNode, UNICODE_STRING *FileName);
extern NTSTATUS build_nxlcache_reparse_file_name(NXL_CACHE_NODE *pNode, UNICODE_STRING *FileName);
extern NTSTATUS get_file_id_and_attribute_ex(PFLT_INSTANCE Instance, FILE_OBJECT *FileObject, LARGE_INTEGER *Id, ULONG *FileAttributes);

FLT_PREOP_CALLBACK_STATUS
nxrmfltPreCleanup(
_Inout_ PFLT_CALLBACK_DATA				Data,
_In_ PCFLT_RELATED_OBJECTS				FltObjects,
_Flt_CompletionContext_Outptr_ PVOID	*CompletionContext
)
{
	NTSTATUS Status = STATUS_SUCCESS;
	BOOLEAN DeleteFile = FALSE;
	BOOLEAN ContentDirty = FALSE;
	BOOLEAN	KeepRecordAndOnDiskNXLFile = FALSE;
	PFLT_INSTANCE			FltInstance = NULL;
	PFILE_OBJECT			FileObject = NULL;
	PNXRMFLT_STREAM_CONTEXT	Ctx = NULL;

	NXL_CACHE_NODE	*pCacheNode = NULL;

	PNXRMFLT_STREAMHANDLE_CONTEXT Ccb = NULL;
	PNXL_RENAME_NODE pRenameNode = NULL;

	NXRMFLT_INSTANCE_CONTEXT *InstCtx = NULL;

	LIST_ENTRY *ite = NULL;

	HANDLE RequestorProcessId = NULL;
	ULONG SessionId = 0;

	FltInstance = FltObjects->Instance;
	FileObject = FltObjects->FileObject;

	//
	// The following block is for handleing automatic appending of .nxl
	// filename extension to non-.nxl files containing NXL header in RPM dir.
	//
	{
		WCHAR *bufDosName = NULL;
		PFLT_FILE_NAME_INFORMATION NameInfo = NULL;
		WCHAR *bufNtDosName = NULL;
		WCHAR *bufNewName = NULL;

		do {
			UNICODE_STRING FileName = {0, 0, NULL};
			HANDLE processHandle = getProcessIDFromData(Data);
			ULONG processId = (ULONG)(ULONG_PTR)processHandle;
			ULONG threadId = (ULONG)(ULONG_PTR)(Data->Thread ? PsGetThreadId(Data->Thread) : PsGetCurrentThreadId());

			PDEVICE_OBJECT devObj = NULL;
			Status = FltGetDiskDeviceObject(FltObjects->Volume, &devObj);
			if (!NT_SUCCESS(Status))
			{
				break;
			}

			bufDosName = ExAllocateFromPagedLookasideList(&Global.FullPathLookaside);
			if (!bufDosName)
			{
				break;
			}

			UNICODE_STRING dosName;
			RtlInitEmptyUnicodeString(&dosName, bufDosName, NXRMFLT_FULLPATH_BUFFER_SIZE);

			UNICODE_STRING volumeName;
			Status = IoVolumeDeviceToDosName(devObj, &volumeName);
			ObDereferenceObject(devObj);
			devObj = NULL;
			if (!NT_SUCCESS(Status))
			{
				break;
			}

			//PT_DBG_PRINT(PTDBG_TRACE_CRITICAL, ("nxrmflt!Test PreEnter nxrmfltPreCleanup processId : %u dosName : %wZ Length : %u, MaxLength : %u \n", processId, &dosName, dosName.Length, dosName.MaximumLength));

			Status = RtlAppendUnicodeStringToString(&dosName, &volumeName);
			ExFreePool(volumeName.Buffer);

			Status = FltGetFileNameInformation(Data,
											   FLT_FILE_NAME_NORMALIZED | FLT_FILE_NAME_QUERY_DEFAULT,
											   &NameInfo);
			if (!NT_SUCCESS(Status))
			{
				break;
			}

			Status = FltParseFileNameInformation(NameInfo);
			if (!NT_SUCCESS(Status))
			{
				break;
			}

			FileName.Length = NameInfo->Name.Length - NameInfo->Volume.Length;
			FileName.MaximumLength = NameInfo->Name.MaximumLength - NameInfo->Volume.Length;
			FileName.Buffer = NameInfo->Name.Buffer + NameInfo->Volume.Length / sizeof(WCHAR);

			Status = RtlAppendUnicodeStringToString(&dosName, &FileName);

			if (!NT_SUCCESS(Status))
			{
				PT_DBG_PRINT(PTDBG_TRACE_CRITICAL, ("nxrmflt!Test failed nxrmfltPreCleanup processId : %u fileLength : %u, maxLen : %u error : %u \n",
					processId, FileName.Length, FileName.MaximumLength, Status));
			}

			{
				FILE_STANDARD_INFORMATION StandInfo;

				Status = FltQueryInformationFile(FltInstance, FileObject, &StandInfo, sizeof(StandInfo), FileStandardInformation, NULL);

				if (NT_SUCCESS(Status) && !StandInfo.Directory)
				{
					BOOLEAN autoAppendNxlExt;
					if (nxrmfltInsideSafeDir(FltInstance, &FileName, FALSE, NULL, &autoAppendNxlExt)
						&& autoAppendNxlExt
						&& !nxrmfltIsTmpExt(&NameInfo->Extension))
					{
						bufNtDosName = ExAllocateFromPagedLookasideList(&Global.FullPathLookaside);
						if (!bufNtDosName)
						{
							break;
						}

						UNICODE_STRING ntDosName;
						RtlInitEmptyUnicodeString(&ntDosName, bufNtDosName, NXRMFLT_FULLPATH_BUFFER_SIZE);

						RtlAppendUnicodeToString(&ntDosName, L"\\??\\");
						RtlAppendUnicodeStringToString(&ntDosName, &dosName);

						//check file extension

						if (!nxrmfltEndWithDotNXL(&ntDosName))
						{
							Status = NXLCheck(FltInstance, FileObject); //check NXL header

							if (!NT_SUCCESS(Status))
							{
								break;
							}

							// Check if the file on disk is at least as big as the end of content
							BOOLEAN present;
							Status = nxrmfltIsAllFileContentPresent(FltInstance, FileObject, &present);
							if (!NT_SUCCESS(Status))
							{
								break;
							}

							if (present)
							{
								bufNewName = ExAllocateFromPagedLookasideList(&Global.FullPathLookaside);
								if (!bufNewName)
								{
									break;
								}

								UNICODE_STRING new_name;
								RtlInitEmptyUnicodeString(&new_name, bufNewName, NXRMFLT_FULLPATH_BUFFER_SIZE);
								RtlAppendUnicodeStringToString(&new_name, &ntDosName);
								RtlAppendUnicodeToString(&new_name, L".nxl");

								if (!nxrmfltDoesFileExist(FltInstance, &new_name, TRUE))
								{
									Status = nxrmfltRenameFile(FltInstance, &ntDosName, &new_name);
									if (!NT_SUCCESS(Status))
									{
										PT_DBG_PRINT(PTDBG_TRACE_CRITICAL, ("nxrmflt!Can't rename %wZ to %wZ, error : %u\n", &ntDosName, &new_name, Status));
										break;
									}

									do {
										ULONG DirHash = 0;
										LARGE_INTEGER FileId = { 0 };
										ULONG FileAttributes = 0;

										// create the cache node
										NXL_CACHE_NODE *pCacheNode = NULL;
										pCacheNode = ExAllocateFromPagedLookasideList(&Global.NXLCacheLookaside);

										if (!pCacheNode)
										{
											//
											// very unlikely, out of memory?
											//
											break;
										}

										memset(pCacheNode, 0, sizeof(NXL_CACHE_NODE));

										Status = build_nxlcache_file_name(pCacheNode, &NameInfo->Name);

										if (!NT_SUCCESS(Status))
										{
											//
											// ERROR case. No memory
											//
											PT_DBG_PRINT(PTDBG_TRACE_CRITICAL, ("nxrmflt!Can't allocate memory to build cache for %wZ\n", &ntDosName));

											ExFreeToPagedLookasideList(&Global.NXLCacheLookaside, pCacheNode);
											pCacheNode = NULL;
											break;
										}

										const UNICODE_STRING ParentFullPath = {
											NameInfo->Name.Length - NameInfo->FinalComponent.Length,
											NameInfo->Name.MaximumLength,
											NameInfo->Name.Buffer
										};

										RtlHashUnicodeString(&ParentFullPath, TRUE, HASH_STRING_ALGORITHM_X65599, &DirHash);

										get_file_id_and_attribute_ex(FltInstance, FileObject, &FileId, &FileAttributes);
										pCacheNode->FileAttributes = FileAttributes;
										pCacheNode->FileID.QuadPart = FileId.QuadPart;
										pCacheNode->Flags |= FlagOn(FileAttributes, FILE_ATTRIBUTE_READONLY) ? NXRMFLT_FLAG_READ_ONLY : 0;
										pCacheNode->Instance = FltObjects->Instance;
										pCacheNode->ParentDirectoryHash = DirHash;
										pCacheNode->OnRemoveOrRemovableMedia = FALSE;
										pCacheNode->LastWriteProcessId = NULL;

										ExInitializeRundownProtection(&pCacheNode->NodeRundownRef);

										Status = build_nxlcache_reparse_file_name(pCacheNode, &NameInfo->Name);

										if (!NT_SUCCESS(Status))
										{
											//
											// ERROR case. No memory
											//
											PT_DBG_PRINT(PTDBG_TRACE_CRITICAL, ("nxrmflt!Can't allocate memory to build cache for %wZ\n", &ntDosName));

											if (pCacheNode->ReleaseFileName)
											{
												ExFreePoolWithTag(pCacheNode->FileName.Buffer, NXRMFLT_NXLCACHE_TAG);
												RtlInitUnicodeString(&pCacheNode->FileName, NULL);
											}

											ExFreeToPagedLookasideList(&Global.NXLCacheLookaside, pCacheNode);
											pCacheNode = NULL;
											break;
										}

										PT_DBG_PRINT(PTDBG_TRACE_CACHE_NODE, ("nxrmflt!AddNXLNodeToCache add file %wZ into cache\n", &ntDosName));

										AddNXLNodeToCache(&Global.NxlFileCache, pCacheNode);

										pCacheNode = NULL;

									} while (FALSE);
								}
							}
						}
					}
				}
			}
		} while (FALSE);

		if (bufNewName)
		{
			ExFreeToPagedLookasideList(&Global.FullPathLookaside, bufNewName);
			bufNewName = NULL;
		}

		if (bufNtDosName)
		{
			ExFreeToPagedLookasideList(&Global.FullPathLookaside, bufNtDosName);
			bufNtDosName = NULL;
		}

		if (NameInfo)
		{
			FltReleaseFileNameInformation(NameInfo);
			NameInfo = NULL;
		}

		if (bufDosName)
		{
			ExFreeToPagedLookasideList(&Global.FullPathLookaside, bufDosName);
			bufDosName = NULL;
		}
	}

	do 
	{
		Status = FltGetInstanceContext(FltInstance, &InstCtx);

		if (!NT_SUCCESS(Status))
		{
			break;
		}

		if (InstCtx->DisableFiltering)
		{
			break;
		}

		Status = FltGetStreamHandleContext(FltInstance, FileObject, &Ccb);

		//
		// In the case of failing to get DestinationFileNameInfo in the rename request, DestinationFileNameInfo is NULL
		//
		if (Ccb && Ccb->DestinationFileNameInfo)
		{
			FltAcquirePushLockExclusive(&Global.RenameListLock);

			pRenameNode = nxrmfltFindRenameNodeFromCcb(Ccb);

			if(pRenameNode)
			{
				RemoveEntryList(&(pRenameNode->Link));
			}
			else
			{
				PT_DBG_PRINT(PTDBG_TRACE_CRITICAL,("nxrmflt!Failed to find rename item for Ccb %p which has src: %wZ and dst: %wZ\n", Ccb, &Ccb->SourceFileNameInfo->Name, &Ccb->DestinationFileNameInfo->Name));
			}

			FltReleasePushLock(&Global.RenameListLock);

			if(pRenameNode)
			{
				nxrmfltFreeRenameNode(pRenameNode);

				ExFreeToPagedLookasideList(&Global.RenameCacheLookaside, pRenameNode);

				pRenameNode = NULL;
			}

			if (Ccb->EncryptDestinationFile)
			{
				FltAcquirePushLockShared(&Global.NxlFileCacheLock);

				pCacheNode = FindNXLNodeInCache(&Global.NxlFileCache, &Ccb->DestinationFileNameInfo->Name);

				if (pCacheNode)
				{
					if (!ExAcquireRundownProtection(&pCacheNode->NodeRundownRef))
					{
						pCacheNode = NULL;
					}
				}

				FltReleasePushLock(&Global.NxlFileCacheLock);

				if (pCacheNode)
				{
					ExReleaseRundownProtection(&pCacheNode->NodeRundownRef);
					ExWaitForRundownProtectionRelease(&pCacheNode->NodeRundownRef);
					Status = nxrmfltEncryptFile(Data,
												FltInstance,
												FltInstance,			// I can use this because it's a rename which means it's in the same volume
												&pCacheNode->ReparseFileName,
												&pCacheNode->OriginalFileName);

					ExReInitializeRundownProtection(&pCacheNode->NodeRundownRef);
					if (!NT_SUCCESS(Status))
					{
						PT_DBG_PRINT(PTDBG_TRACE_CRITICAL, ("nxrmflt!nxrmfltPreCleanup: Failed to encrypt file %wZ! Status is %x\n", &pCacheNode->OriginalFileName, Status));
					}
					else
					{
						Ccb->EncryptDestinationFile = FALSE;

						RequestorProcessId = getProcessIDFromData(Data);

						if (pCacheNode->LastWriteProcessId != RequestorProcessId)
						{
							Status = FltGetRequestorSessionId(Data, &SessionId);

							if (Status == STATUS_SUCCESS)
							{
								Status = nxrmfltSendEditActivityLog(SessionId,
																	(HANDLE)RequestorProcessId,
																	InstCtx->Instance,
																	&pCacheNode->FileName);
								if (NT_SUCCESS(Status))
								{
									pCacheNode->LastWriteProcessId = RequestorProcessId;
								}
							}
						}
					}

					pCacheNode = NULL;
				}
			}
		}

		if (Ccb && Ccb->DeleteOnClose)
		{
			DeleteFile = TRUE;
		}

		Status = FltGetStreamContext(FltInstance, FileObject, &Ctx);

		if (!Ctx)
		{
			PFLT_FILE_NAME_INFORMATION	NameInfo = NULL;
		
			do 
			{
				DeleteFile = (DeleteFile || (FileObject->DeletePending != 0));

				if (!DeleteFile)
				{
					break;
				}

				Status = FltGetFileNameInformation(Data,
												   FLT_FILE_NAME_NORMALIZED | FLT_FILE_NAME_QUERY_DEFAULT,
												   &NameInfo);

				if (!NT_SUCCESS(Status))
				{
					break;
				}

				Status = FltParseFileNameInformation(NameInfo);

				if (!NT_SUCCESS(Status))
				{
					break;
				}

				//
				// in the case of explorer or nxrmserv delete nxl file
				//
			
				
				{
					UNICODE_STRING SrcFileNameWithoutNXLExtension = {0};

					if (IsNXLFile(&(NameInfo->Extension)))
					{
						SrcFileNameWithoutNXLExtension.Buffer = NameInfo->Name.Buffer;
						SrcFileNameWithoutNXLExtension.Length = NameInfo->Name.Length - 4 * sizeof(WCHAR);
						SrcFileNameWithoutNXLExtension.MaximumLength = NameInfo->Name.MaximumLength;
					}
					else
					{
						SrcFileNameWithoutNXLExtension.Buffer = NameInfo->Name.Buffer;
						SrcFileNameWithoutNXLExtension.Length = NameInfo->Name.Length;
						SrcFileNameWithoutNXLExtension.MaximumLength = NameInfo->Name.MaximumLength;
					}

					//
					// build new cache node, rename on disk nxl file and complete this I/O
					//

					do 
					{
						NXL_CACHE_NODE	*pExistingCacheNode = NULL;
						FltAcquirePushLockExclusive(&Global.NxlFileCacheLock);

						pExistingCacheNode = FindNXLNodeInCache(&Global.NxlFileCache, &SrcFileNameWithoutNXLExtension);

						if (pExistingCacheNode)
						{
							if (!ExAcquireRundownProtection(&pExistingCacheNode->NodeRundownRef))
							{
								pExistingCacheNode = NULL;
							}
							else {
								//
								// Delete old cache node from cache. NOTE: pCacheNode still valid after delete
								// We only delete the node from cache. We don't free resource here
								//
								DeleteNXLNodeInCache(&Global.NxlFileCache, pExistingCacheNode);

								//
								// ok to call it inside lock because "nxrmfltDeleteFileByName" only queue workitem
								//
								nxrmfltDeleteFileByName(FltInstance,
									&SrcFileNameWithoutNXLExtension);

							}
						}

						FltReleasePushLock(&Global.NxlFileCacheLock);

						if (!pExistingCacheNode)
						{
							break;
						}

						//
						// purge rights cache
						//
						nxrmfltPurgeRightsCache(pExistingCacheNode->Instance, pExistingCacheNode->FileNameHash);

						//
						// release old cache node rundown protection
						//
						ExReleaseRundownProtection(&pExistingCacheNode->NodeRundownRef);
						//
						// free old cache node
						//
						FreeNXLCacheNode(pExistingCacheNode);
						pExistingCacheNode = NULL;

					} while (FALSE);

				}

			} while (FALSE);

			if (NameInfo)
			{
				FltReleaseFileNameInformation(NameInfo);
				NameInfo = NULL;
			}

			break;
		}

		FltAcquirePushLockShared(&Global.ExpireTableLock);

		FOR_EACH_LIST(ite, &Global.ExpireTable)
		{
			NXL_SAVEAS_NODE *pSaveAsNode = CONTAINING_RECORD(ite, NXL_SAVEAS_NODE, Link);

			if (RtlEqualUnicodeString(&Ctx->FileName, &pSaveAsNode->SourceFileName, TRUE) &&
				RtlEqualUnicodeString(&pSaveAsNode->SourceFileName, &pSaveAsNode->SaveAsFileName, TRUE))
			{
				KeepRecordAndOnDiskNXLFile = TRUE;
				break;
			}
		}

		FltReleasePushLock(&Global.ExpireTableLock);

		//
		// Here is a hack to work around Jira issue SAP-2342, which is a problem in SAP PLM add-in
		// use case.  The use case is this:
		// 1. SAP PLM add-in in a trusted Word (or Excel or PowerPoint) process calls
		//    ISDRmcInstance::RPMEditCopyFile() to start editing foo.docx.nxl an RPM dir.
		// 2. Add-in tries to delete foo.docx.
		// 3. Add-in copies some plain content from another file to foo.docx.
		// 4. Sometime later, add-in calls ISDRmcInstance::RPMEditSaveFile() to finish editing.
		// In a normal edit workflow, Steps #2 and #3 should not be performed.  However, the SAP
		// PLM add-in does that.
		//
		// Normally, Step #2 would cause nxrmflt driver to delete both the invisible foo.docx and
		// the visible foo.docx.nxl.  Then Step #3 would cause the resulting foo.docx in the RPM
		// dir to be unprotected.  This causes data leak.
		//
		// The hack to work around the problem is that:
		// A: For Step #2, if the process is a trusted Office process, don't delete foo.docx.nxl
		//    when deleting foo.docx.
		// B: For Step #3, if the process is a trusted Office process, don't deny creating
		//    foo.docx for overwrite even if foo.docx.nxl exists.
		// #A is handled here below.  #B is handled in nxrmfltPreCreate().
		//
		if (FileObject->DeletePending != 0 && is_msoffice_process())
		{
			KeepRecordAndOnDiskNXLFile = TRUE;
		}

		ContentDirty = Ctx->ContentDirty?TRUE:FALSE;

		DeleteFile = (FileObject->DeletePending != 0 || DeleteFile) && (!KeepRecordAndOnDiskNXLFile);

		if (DeleteFile)
		{
			FltAcquirePushLockExclusive(&Global.NxlFileCacheLock);
		}
		else
		{
			FltAcquirePushLockShared(&Global.NxlFileCacheLock);
		}

		pCacheNode = FindNXLNodeInCache(&Global.NxlFileCache, &Ctx->FileName);

		if (pCacheNode)
		{
			if (DeleteFile)
			{
				//
				//
				// Remove Cache node from the Cache. We don't free resource here because:
				//		A. There could be other threads are using this record
				//		B. We don't want to wait other threads rundown while holding a push lock
				//
				DeleteNXLNodeInCache(&Global.NxlFileCache, pCacheNode);
				//
				// Delete original file
				//
				Status = nxrmfltDeleteFileByName(Ctx->OriginalInstance, &pCacheNode->OriginalFileName);

				if (!NT_SUCCESS(Status))
				{
					PT_DBG_PRINT(PTDBG_TRACE_CRITICAL, ("nxrmflt!nxrmfltPreCleanup: Failed to delete file %wZ! Status is %x\n", &pCacheNode->OriginalFileName, Status));
				}
			}
			else
			{
				if (!ExAcquireRundownProtection(&pCacheNode->NodeRundownRef))
				{
					pCacheNode = NULL;
				}
			}
		}

		FltReleasePushLock(&Global.NxlFileCacheLock);

		if (!pCacheNode)
		{
			//
			// in the case of protection has been moved
			//
			break;
		}

		if (DeleteFile)
		{
			//
			// purge rights cache
			//
			nxrmfltPurgeRightsCache(pCacheNode->Instance, pCacheNode->FileNameHash);

			FreeNXLCacheNode(pCacheNode);

			pCacheNode = NULL;
		}
		else
		{
			if (ContentDirty)
			{
				NXL_SAVEAS_NODE *pSaveAsNode = NULL;
				NXL_CACHE_NODE *pSrcCacheNode = NULL;

				InterlockedExchange(&Ctx->ContentDirty, 0);

				//
				// as we didn't support save back to NXL in current RMD/nxrmflt design, we shall not go into Encrypt/Save logic anymore
				// also, although the nxrmfltEncryptFile will fail for some reason, following code still change the cache node status (line 774)
				// so for bug 66522, we will break the save logic
				//
				break;

				ExReleaseRundownProtection(&pCacheNode->NodeRundownRef);
				ExWaitForRundownProtectionRelease(&pCacheNode->NodeRundownRef);
				Status = nxrmfltEncryptFile(Data,
											FltInstance,
											Ctx->OriginalInstance,
											&pCacheNode->ReparseFileName,
											&pCacheNode->OriginalFileName);
				ExReInitializeRundownProtection(&pCacheNode->NodeRundownRef);
				ExAcquireRundownProtection(&pCacheNode->NodeRundownRef);
				if (!NT_SUCCESS(Status))
				{
					//
					// restore Flag if encryption failed
					//
					InterlockedExchange(&Ctx->ContentDirty, 1);

					PT_DBG_PRINT(PTDBG_TRACE_CRITICAL, ("nxrmflt!nxrmfltPreCleanup: Failed to encrypt file %wZ! Status is %x\n", &pCacheNode->OriginalFileName, Status));
				}
				else
				{
					RequestorProcessId = getProcessIDFromData(Data);

					if (pCacheNode->LastWriteProcessId != RequestorProcessId)
					{
						Status = FltGetRequestorSessionId(Data, &SessionId);

						if (Status == STATUS_SUCCESS)
						{
							Status = nxrmfltSendEditActivityLog(SessionId,
																(HANDLE)RequestorProcessId,
																InstCtx->Instance,
																&pCacheNode->FileName);
							if (NT_SUCCESS(Status))
							{
								pCacheNode->LastWriteProcessId = RequestorProcessId;
							}
						}
					}
				}

				FltAcquirePushLockShared(&Global.ExpireTableLock);

				FOR_EACH_LIST(ite, &Global.ExpireTable)
				{
					pSaveAsNode = CONTAINING_RECORD(ite, NXL_SAVEAS_NODE, Link);

					if (RtlEqualUnicodeString(&Ctx->FileName, &pSaveAsNode->SaveAsFileName, TRUE))
					{
						if (!ExAcquireRundownProtection(&pSaveAsNode->NodeRundownRef))
						{
							pSaveAsNode = NULL;
						}

						break;
					}
					else
					{
						pSaveAsNode = NULL;
					}
				}

				FltReleasePushLock(&Global.ExpireTableLock);

				if (pSaveAsNode)
				{
					FltAcquirePushLockShared(&Global.NxlFileCacheLock);

					pSrcCacheNode = FindNXLNodeInCache(&Global.NxlFileCache, &pSaveAsNode->SourceFileName);

					if (pSrcCacheNode)
					{
						if (!ExAcquireRundownProtection(&pSrcCacheNode->NodeRundownRef))
						{
							pSrcCacheNode = NULL;
						}
					}

					FltReleasePushLock(&Global.NxlFileCacheLock);

					ExReleaseRundownProtection(&pSaveAsNode->NodeRundownRef);

					pSaveAsNode = NULL;

					if (pSrcCacheNode)
					{
						//
						// Update Tags here
						//
						Status = FltGetRequestorSessionId(Data, &SessionId);
						
						if (STATUS_SUCCESS == Status)
						{
							Status = nxrmfltCopyTags(getProcessIDFromData(Data),
													 SessionId,
													 pSrcCacheNode->Instance,
													 &pSrcCacheNode->OriginalFileName,
													 pCacheNode->Instance,
													 &pCacheNode->OriginalFileName);

							if (!NT_SUCCESS(Status))
							{
								PT_DBG_PRINT(PTDBG_TRACE_CRITICAL, ("nxrmflt!Can't copy tags from file %wZ to %wZ\n", &pSrcCacheNode->OriginalFileName, &pCacheNode->OriginalFileName));
							}
							else
							{
								nxrmfltPurgeRightsCache(pCacheNode->Instance, pCacheNode->FileNameHash);
							}
						}

						ExReleaseRundownProtection(&pSrcCacheNode->NodeRundownRef);

						pSrcCacheNode = NULL;
					}
				}

				//
				// copy saved NXL file is the file is on remote or removable media
				//
				if (pCacheNode->OnRemoveOrRemovableMedia && pCacheNode->SourceFileName.Length)
				{
					//
					// still need to make sure the final component in pCacheNode->FileName match SourceFileName because we ONLY deal with "Save"
					//
					if (0 == nxrmfltCompareFinalComponent(&pCacheNode->OriginalFileName,
														  &pCacheNode->SourceFileName,
														  TRUE))
					{
						Status = nxrmfltCopyOnDiskNxlFile(pCacheNode->Instance,
														  &pCacheNode->OriginalFileName,
														  nxrmfltFindInstanceByFileName(&pCacheNode->SourceFileName),
														  &pCacheNode->SourceFileName);

						if (!NT_SUCCESS(Status))
						{
							PT_DBG_PRINT(PTDBG_TRACE_CRITICAL, ("nxrmflt!Can't save back file %wZ to %wZ\n", &pSrcCacheNode->OriginalFileName, &pCacheNode->SourceFileName));
						}
					}
				}
			}

			if (KeepRecordAndOnDiskNXLFile && (FileObject->DeletePending != 0))
			{
				SetFlag(pCacheNode->Flags, NXRMFLT_FLAG_ATTACHING_CTX);
				ClearFlag(pCacheNode->Flags, NXRMFLT_FLAG_CTX_ATTACHED);
			}
		}

	} while (FALSE);

	if (pCacheNode)
	{
		ExReleaseRundownProtection(&pCacheNode->NodeRundownRef);
	}

	if (Ctx)
	{
		FltReleaseContext(Ctx);
	}

	if (Ccb)
	{
		FltReleaseContext(Ccb);
	}

	if (InstCtx)
	{
		FltReleaseContext(InstCtx);
	}

	return FLT_PREOP_SUCCESS_NO_CALLBACK;
}