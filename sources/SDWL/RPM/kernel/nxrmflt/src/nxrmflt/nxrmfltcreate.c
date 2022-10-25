#include "nxrmfltdef.h"
#include "nxrmfltcreate.h"
#include "nxrmfltnxlcachemgr.h"
#include "nxrmfltutils.h"
#include "nxrmflt.h"

extern DECLSPEC_CACHEALIGN ULONG				gTraceFlags;
extern DECLSPEC_CACHEALIGN NXRMFLT_GLOBAL_DATA	Global;

extern LPSTR PsGetProcessImageFileName(PEPROCESS  Process);
extern ULONG PsGetCurrentProcessSessionId(void);

BOOLEAN IsNXLFile(PUNICODE_STRING FinalName);
static BOOLEAN build_reparse_name(NXRMFLT_STREAM_CONTEXT *Ctx);
static void ascii2lower(UNICODE_STRING *str);
static BOOLEAN IsReparseEcpPresent(PFLT_CALLBACK_DATA Data);
static BOOLEAN IsBlockingEcpPresent(PFLT_CALLBACK_DATA Data);
static BOOLEAN CompareMupDeviceNames(PUNICODE_STRING FileName, PUNICODE_STRING OpeningMupName);
NTSTATUS get_file_id_and_attribute_ex(PFLT_INSTANCE Instance, FILE_OBJECT *FileObject, LARGE_INTEGER *Id, ULONG *FileAttributes);

BOOLEAN is_explorer(void);
BOOLEAN is_app_in_real_name_access_list(PEPROCESS  Process);

extern NTSTATUS build_nxlcache_file_name(NXL_CACHE_NODE *pNode, UNICODE_STRING *FileName);
extern NTSTATUS build_nxlcache_reparse_file_name(NXL_CACHE_NODE *pNode, UNICODE_STRING *FileName);

FLT_PREOP_CALLBACK_STATUS nxrmfltPreCreate(
	_Inout_ PFLT_CALLBACK_DATA				Data,
	_In_ PCFLT_RELATED_OBJECTS				FltObjects,
	_Flt_CompletionContext_Outptr_ PVOID	*CompletionContext)
{
	PFLT_FILE_NAME_INFORMATION	NameInfo = NULL;
	NTSTATUS					Status = STATUS_SUCCESS;
	FLT_PREOP_CALLBACK_STATUS	CallbackStatus = FLT_PREOP_SUCCESS_NO_CALLBACK;
	BOOLEAN						ReleaseNameInfo = TRUE;

	NXRMFLT_INSTANCE_CONTEXT	*InstCtx = NULL;

	NXL_CACHE_NODE	*pCacheNode = NULL;

	BOOLEAN	CtxAttached = FALSE;

	ULONG Flags = 0;
	
	BOOLEAN	IgnoreCase = TRUE;

	UNICODE_STRING FileNameAppendNXLExtension = { 0 };
	WCHAR *FileNameAppendNXLExtensionBuffer = NULL;

	ULONG ParentDirectoryHash = 0;

	ULONGLONG RightsMask = 0;

	BOOLEAN SkipProcessingAfterCheckingRights = FALSE;
	
	LIST_ENTRY *ite = NULL;

	NXL_SAVEAS_NODE *pSaveAsNode = NULL;

	NXL_CACHE_NODE	*pSaveAsSourceCacheNode = NULL;

	ULONG CreateDisposition = 0;
	ULONG CreateOptions = 0;

	BOOLEAN IsExplorerDeleteFile = FALSE;
	BOOLEAN IsExplorer = FALSE;
	BOOLEAN IsExplorerRequiringOpLock = FALSE;
	const BOOLEAN IsNxrmserv = (getProcessIDFromData(Data) == Global.PortProcessId);

    wchar_t szBuffer[1024] = {0};

	BOOLEAN IsInSafeDir = FALSE;
#ifdef  NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR
	BOOLEAN IsInSanctuaryDir = FALSE;
#endif  // NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR

	DECLARE_CONST_UNICODE_STRING(LegacyThemesFolder, NXRMFLT_LEGACY_THEMES_FOLDER);

	do 
	{

		if (FlagOn(Data->Iopb->OperationFlags, SL_OPEN_PAGING_FILE))
		{
			break;
		}

		if (FlagOn(Data->Iopb->OperationFlags, SL_OPEN_TARGET_DIRECTORY))
		{
			break;
		}

		if (FlagOn(Data->Iopb->TargetFileObject->Flags, FO_VOLUME_OPEN))
		{
			break;
		}

		if (FlagOn(Data->Iopb->Parameters.Create.Options, FILE_OPEN_BY_FILE_ID))
		{
			break;
		}

		if (FlagOn(Data->Iopb->Parameters.Create.Options, FILE_DIRECTORY_FILE))
		{
			break;
		}

		if (Global.DriverUnloading)
		{
			break;
		}

		IgnoreCase = !(BooleanFlagOn(Data->Iopb->OperationFlags, SL_CASE_SENSITIVE));

		CreateDisposition = (Data->Iopb->Parameters.Create.Options & 0xff000000) >> 24;
		CreateOptions = (Data->Iopb->Parameters.Create.Options & 0x00ffffff);

		if (IsReparseEcpPresent(Data))
		{
			*CompletionContext = NULL;

			CallbackStatus = FLT_PREOP_SUCCESS_WITH_CALLBACK;

			break;
		}

		if (IsBlockingEcpPresent(Data))
		{
			*CompletionContext = NULL;

			if (FlagOn(CreateOptions, FILE_DELETE_ON_CLOSE))
				CallbackStatus = FLT_PREOP_SUCCESS_WITH_CALLBACK;
			else
				CallbackStatus = FLT_PREOP_SUCCESS_NO_CALLBACK;

			break;
		}

		//
		//  Get the name information.
		//

		if (FlagOn(Data->Iopb->OperationFlags, SL_OPEN_TARGET_DIRECTORY)) 
		{

			//
			//  The SL_OPEN_TARGET_DIRECTORY flag indicates the caller is attempting
			//  to open the target of a rename or hard link creation operation. We
			//  must clear this flag when asking fltmgr for the name or the result
			//  will not include the final component. We need the full path in order
			//  to compare the name to our mapping.
			//

			ClearFlag(Data->Iopb->OperationFlags, SL_OPEN_TARGET_DIRECTORY);

			PT_DBG_PRINT(PTDBG_TRACE_CRITICAL,
						 ("nxrmflt!nxrmfltPreCreate: -> Clearing SL_OPEN_TARGET_DIRECTORY for %wZ (Cbd = %p, FileObject = %p)\n",
						 &NameInfo->Name,
						 Data,
						 FltObjects->FileObject));

			//
			//  Get the filename as it appears below this filter. Note that we use 
			//  FLT_FILE_NAME_QUERY_FILESYSTEM_ONLY when querying the filename
			//  so that the filename as it appears below this filter does not end up
			//  in filter manager's name cache.
			//

			Status = FltGetFileNameInformation(Data,
											   FLT_FILE_NAME_NORMALIZED | FLT_FILE_NAME_QUERY_FILESYSTEM_ONLY,
											   &NameInfo);

			//
			//  Restore the SL_OPEN_TARGET_DIRECTORY flag so the create will proceed 
			//  for the target. The file systems depend on this flag being set in 
			//  the target create in order for the subsequent SET_INFORMATION 
			//  operation to proceed correctly.
			//

			SetFlag(Data->Iopb->OperationFlags, SL_OPEN_TARGET_DIRECTORY);


		}
		else
		{

			//
			//  Note that we use FLT_FILE_NAME_QUERY_DEFAULT when querying the 
			//  filename. In the precreate the filename should not be in filter
			//  manager's name cache so there is no point looking there.
			//

			Status = FltGetFileNameInformation(Data,
											   FLT_FILE_NAME_NORMALIZED |
											   FLT_FILE_NAME_QUERY_DEFAULT,
											   &NameInfo);
		}

		if (!NT_SUCCESS(Status)) 
		{

			//PT_DBG_PRINT(PTDBG_TRACE_CRITICAL,
			//			 ("nxrmflt!nxrmfltPreCreate: -> Failed to get name information (Cbd = %p, FileObject = %p)\n",
			//			 Data,
			//			 FltObjects->FileObject));

			break;
		}


		PT_DBG_PRINT(PTDBG_TRACE_OPERATION_STATUS,
					 ("nxrmflt!nxrmfltPreCreate -> Processing create for file %wZ (Cbd = %p, FileObject = %p)\n",
					 &NameInfo->Name,
					 Data,
					 FltObjects->FileObject));

		//
		//  Parse the filename information
		//

		Status = FltParseFileNameInformation(NameInfo);
		
		if (!NT_SUCCESS(Status)) 
		{

			PT_DBG_PRINT(PTDBG_TRACE_CRITICAL,
						 ("nxrmflt!nxrmfltPreCreate -> Failed to parse name information for file %wZ (Cbd = %p, FileObject = %p)\n",
						 &NameInfo->Name,
						 Data,
						 FltObjects->FileObject));

			break;
		}

        if (NameInfo->Name.Length > NameInfo->Volume.Length)
        {
            UNICODE_STRING PathPart = { NameInfo->Name.Length - NameInfo->Volume.Length,
                NameInfo->Name.Length - NameInfo->Volume.Length,
                NameInfo->Name.Buffer + NameInfo->Volume.Length / 2 };

            // We only handle these:
            // - NXL file under SafeDir
            // - Non-NXL file under SanctuaryDir
            BOOLEAN overwrite = FALSE;
            IsInSafeDir = nxrmfltInsideSafeDir(FltObjects->Instance, &PathPart, TRUE, &overwrite, NULL);
#ifdef  NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR
            IsInSanctuaryDir = nxrmfltInsideSanctuaryDir(FltObjects->Instance, &PathPart, TRUE);
            FLT_ASSERTMSG("Overlapping SafeDir and SanctuaryDir is not supported!", !(IsInSafeDir && IsInSanctuaryDir));
#endif  // NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR
            if (!IsInSafeDir
#ifdef  NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR
                && !IsInSanctuaryDir
#endif  // NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR
                )
            {
                break;
            }

            {
                memset(szBuffer, 0, sizeof(szBuffer));
                swprintf_s(szBuffer, sizeof(szBuffer) / sizeof(wchar_t), L"nxrmflt!nxrmfltPreCreate -> Processing create for file %wZ (Cbd = %p, FileObject = %p)", &NameInfo->Name,
                    Data,
                    FltObjects->FileObject);
                nxWriteToLogFile(szBuffer);
            }

#ifdef  NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR
            if (IsInSanctuaryDir && IsNXLFile(&NameInfo->Extension))
            {
                break;
            }
#endif  // NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR

			// For RPM dir, when maybe create a new file:
			// if file name contains .nxl except end .nxl, deny(1.nxl.txt.nxl deny 1.txt.nxl allow)
			// if there will be conflict of file name, deny(file name is 1.txt.nxl, 1.txt exists, deny, file name is 1.txt, 1.txt.nxl exists, deny)
			if (IsInSafeDir &&
				((CreateDisposition == FILE_SUPERSEDE) ||
				 (CreateDisposition == FILE_OPEN_IF) ||
				 (CreateDisposition == FILE_OVERWRITE_IF) ||
				 (CreateDisposition == FILE_CREATE)))
			{
				int iCount = 0;
				BOOLEAN bContainColon = FALSE;
				nxrmfltGetDotNXLCount(&PathPart, &iCount, &bContainColon);
				BOOLEAN bEndWithDotNXL = nxrmfltEndWithDotNXL(&PathPart);

				if (!bContainColon)
				{
					if (iCount > 1)
					{
						Data->IoStatus.Status = STATUS_ACCESS_DENIED;
						Data->IoStatus.Information = 0;
						CallbackStatus = FLT_PREOP_COMPLETE;
						break;
					}
					else if (iCount == 1)
					{
						if (!bEndWithDotNXL)
						{// for ectr case, need allow
                            //
                            // bug 71138, we need to allow IE to download .NXL
                            // In IE, when downloading a.nxl, it will be downloaded as a.nxl.tmp first.
                            // But our RPM driver block to create file with ".nxl" in file name, not extension.
                            // So we will bypass IE then.
                            //
                            if (is_IE_like_process() == FALSE)
                            {
                                Data->IoStatus.Status = STATUS_ACCESS_DENIED;
                                Data->IoStatus.Information = 0;
                                CallbackStatus = FLT_PREOP_COMPLETE;
                                break;
                            }
						}
						else
						{
							//If the file is nxl file, and there is a decrypted temp file, if it's FILE_SUPERSEDE or FILE_OVERWRITE_IF
							//delete the temp file, clear flag, finish the supersede or overwrite action.

							UNICODE_STRING FileNameWithoutNXLExtension = { 0 };
							FileNameWithoutNXLExtension.Buffer = NameInfo->Name.Buffer;
							FileNameWithoutNXLExtension.MaximumLength = NameInfo->Name.MaximumLength - sizeof(NXRMFLT_NXL_DOTEXT) + sizeof(WCHAR);
							FileNameWithoutNXLExtension.Length = NameInfo->Name.Length - sizeof(NXRMFLT_NXL_DOTEXT) + sizeof(WCHAR);

							if (nxrmfltDoesFileExist(FltObjects->Instance, &FileNameWithoutNXLExtension, TRUE))
							{
								if (nxrmfltDoesFileExist(FltObjects->Instance, &NameInfo->Name, TRUE))
								{
									if (CreateDisposition == FILE_SUPERSEDE || CreateDisposition == FILE_OVERWRITE_IF)
									{
										if (nxrmfltIsFileOpened(FltObjects->Instance, &FileNameWithoutNXLExtension, TRUE, NULL) && is_msoffice_process())
										{
											FltAcquirePushLockExclusive(&Global.NxlFileCacheLock);

											NXL_CACHE_NODE *pTmpNode = FindNXLNodeInCache(&Global.NxlFileCache, &FileNameWithoutNXLExtension);
											if (pTmpNode)
											{
												ClearFlag(pTmpNode->Flags, NXRMFLT_FLAG_CTX_ATTACHED);
											}

											FltReleasePushLock(&Global.NxlFileCacheLock);
										}
										else 
										{
											if (NT_SUCCESS(nxrmfltDeleteFileByNameSync(FltObjects->Instance, &FileNameWithoutNXLExtension)))
											{
												FltAcquirePushLockExclusive(&Global.NxlFileCacheLock);

												NXL_CACHE_NODE *pTmpNode = FindNXLNodeInCache(&Global.NxlFileCache, &FileNameWithoutNXLExtension);
												if (pTmpNode)
												{
													ClearFlag(pTmpNode->Flags, NXRMFLT_FLAG_CTX_ATTACHED);
												}

												FltReleasePushLock(&Global.NxlFileCacheLock);
											}
											else
											{
												// failed to delete name_without_nxl, it might be locked by application
												if (is_msoffice_process())
												{
													FltAcquirePushLockExclusive(&Global.NxlFileCacheLock);

													NXL_CACHE_NODE *pTmpNode = FindNXLNodeInCache(&Global.NxlFileCache, &FileNameWithoutNXLExtension);
													if (pTmpNode)
													{
														ClearFlag(pTmpNode->Flags, NXRMFLT_FLAG_CTX_ATTACHED);
													}

													FltReleasePushLock(&Global.NxlFileCacheLock);
												}
												else {
													Data->IoStatus.Status = STATUS_SHARING_VIOLATION;
													Data->IoStatus.Information = 0;
													CallbackStatus = FLT_PREOP_COMPLETE;
													break;
												}
											}
										}
									}
								}
								else
								{
									if (!overwrite)
									{
										Data->IoStatus.Status = STATUS_ACCESS_DENIED;
										Data->IoStatus.Information = 0;
										CallbackStatus = FLT_PREOP_COMPLETE;
										break;
									}
								}
							}
						}
					}
					else
					{
						FileNameAppendNXLExtensionBuffer = ExAllocateFromPagedLookasideList(&Global.FullPathLookaside);
						if (FileNameAppendNXLExtensionBuffer)
						{
							FileNameAppendNXLExtension.Buffer = FileNameAppendNXLExtensionBuffer;
							FileNameAppendNXLExtension.MaximumLength = NXRMFLT_FULLPATH_BUFFER_SIZE;
							FileNameAppendNXLExtension.Length = 0;
							RtlUnicodeStringCat(&FileNameAppendNXLExtension, &NameInfo->Name);
							RtlUnicodeStringCatString(&FileNameAppendNXLExtension, NXRMFLT_NXL_DOTEXT);

							BOOLEAN bExist;
							BOOLEAN bOpened = nxrmfltIsFileOpened(FltObjects->Instance, &FileNameAppendNXLExtension, TRUE, &bExist);

							if (bExist)
							{
								if (CreateDisposition == FILE_OPEN_IF || CreateDisposition == FILE_SUPERSEDE || CreateDisposition == FILE_OVERWRITE_IF)
								{
									// The process is trying to create foo.ext for overwrite, and foo.ext.nxl exists.
									// If NXLRMFLT_RPMFOLDER_OVERWRITE is set in the RPM dir, and neither foo.ext nor foo.ext.nxl
									// are currently open, delete both foo.ext and foo.ext.nxl.
                                    
                                    //
                                    // bug: 70960, 70962
                                    //      only when process is ProjectWise Explorer, we allow normal file to overwrite nxl file in RPM folder
                                    //      of course, TC RAC client might have same requirement
                                    //      
                                    //      if not to hard code process name in is_pwexplorer_like_process(), we shall have an API to let application RMX
                                    //      to tell driver which shall be allowed to overwrite NXL with normal file
                                    //
                                    BOOLEAN IsPwExplorer = is_pwexplorer_like_process();
									if (overwrite && !bOpened && IsPwExplorer)
									{
										BOOLEAN bExist2;
										BOOLEAN bOpened2 = nxrmfltIsFileOpened(FltObjects->Instance, &NameInfo->Name, TRUE, &bExist2);

										if (!bOpened2)
										{
											if (!bExist2 || NT_SUCCESS(nxrmfltDeleteFileByNameSync(FltObjects->Instance, &NameInfo->Name)))
											{
												FltAcquirePushLockExclusive(&Global.NxlFileCacheLock);

												pCacheNode = FindNXLNodeInCache(&Global.NxlFileCache, &NameInfo->Name);
												if (pCacheNode)
												{
													ClearFlag(pCacheNode->Flags, NXRMFLT_FLAG_CTX_ATTACHED);
												}

												if (NT_SUCCESS(nxrmfltDeleteFileByNameSync(FltObjects->Instance, &FileNameAppendNXLExtension)))
												{
													if (pCacheNode)
													{
														DeleteNXLNodeInCache(&Global.NxlFileCache, pCacheNode);
														FreeNXLCacheNode(pCacheNode);
													}
												}

												pCacheNode = NULL;

												FltReleasePushLock(&Global.NxlFileCacheLock);
											}
											break;
										}
									}

									if (!nxrmfltDoesFileExist(FltObjects->Instance, &NameInfo->Name, TRUE))
									{
										// Here is a hack to work around Jira issue SAP-2342, which is a problem in SAP PLM add-in
										// use case.  The use case is this:
										// 1. SAP PLM add-in in a trusted Word (or Excel or PowerPoint) process calls
										//    ISDRmcInstance::RPMEditCopyFile() to start editing foo.docx.nxl in an RPM dir.
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
										// #A is handled in nxrmfltPreCleanup().  #B is handled here below.
										if (!is_msoffice_process())
										{
											Data->IoStatus.Status = STATUS_ACCESS_DENIED;
											Data->IoStatus.Information = 0;
											CallbackStatus = FLT_PREOP_COMPLETE;
											break;
										}
										else
										{
											//
											// continue the hack fix for SAP ECTR
											// we shall set the flag as ATTACHED as we allow Office to overwrite
											PT_DBG_PRINT(PTDBG_TRACE_CRITICAL, ("nxrmflt!nxrmfltPreCreate: =================================================================================================\n"));
											PT_DBG_PRINT(PTDBG_TRACE_CRITICAL, ("nxrmflt!nxrmfltPreCreate: IsReattached = TRUE, %wZ%ws\n", &NameInfo->Name, NXRMFLT_NXL_DOTEXT));
											PT_DBG_PRINT(PTDBG_TRACE_CRITICAL, ("nxrmflt!nxrmfltPreCreate: =================================================================================================\n"));

											pCacheNode = FindNXLNodeInCache(&Global.NxlFileCache, &NameInfo->Name);
											if (pCacheNode)
											{ 
												ClearFlag(pCacheNode->Flags, NXRMFLT_FLAG_ATTACHING_CTX);
												SetFlag(pCacheNode->Flags, NXRMFLT_FLAG_CTX_ATTACHED);

												pCacheNode = NULL;
											}
											else
											{
												//
												// cache node is not found, but the both NXL and decrypted native files are in disk
												// rebuild the cache node
												//
												do
												{
													PFLT_INSTANCE	FltInstance = NULL;
													PFILE_OBJECT	FileObject = NULL;
													NXRMFLT_REPARSE_ECP_CONTEXT *EcpCtx = NULL;
													NXL_CACHE_NODE *pNewCacheNode = NULL;
													LARGE_INTEGER FileId = { 0 };
													ULONG FileAttributes = 0;

													ULONG DirHash = 0;

													pNewCacheNode = ExAllocateFromPagedLookasideList(&Global.NXLCacheLookaside);
													if (!pNewCacheNode)
													{
														//
														// very unlikely, out of memory?
														//
														break;
													}

													memset(pNewCacheNode, 0, sizeof(NXL_CACHE_NODE));

													Status = build_nxlcache_file_name(pNewCacheNode, &(NameInfo->Name));
													if (!NT_SUCCESS(Status))
													{
														//
														// ERROR case. No memory
														//
														PT_DBG_PRINT(PTDBG_TRACE_CRITICAL, ("nxrmflt!Can't allocate memory to build cache for %wZ\n", &NameInfo->Name));

														ExFreeToPagedLookasideList(&Global.NXLCacheLookaside, pNewCacheNode);
														pNewCacheNode = NULL;
														break;
													}

													FltInstance = FltObjects->Instance;
													FileObject = FltObjects->FileObject;
													get_file_id_and_attribute_ex(FltInstance, FileObject, &FileId, &FileAttributes);

													pNewCacheNode->FileAttributes = FileAttributes;
													pNewCacheNode->FileID.QuadPart = FileId.QuadPart;
													pNewCacheNode->Flags |= FlagOn(FileAttributes, FILE_ATTRIBUTE_READONLY) ? NXRMFLT_FLAG_READ_ONLY : 0;
													pNewCacheNode->Flags |= NXRMFLT_FLAG_CTX_ATTACHED;
													pNewCacheNode->Instance = FltObjects->Instance;
													pNewCacheNode->ParentDirectoryHash = DirHash;
													pNewCacheNode->OnRemoveOrRemovableMedia = FALSE;
													pNewCacheNode->LastWriteProcessId = NULL;

													ExInitializeRundownProtection(&pNewCacheNode->NodeRundownRef);
													Status = build_nxlcache_reparse_file_name(pNewCacheNode, &NameInfo->Name);
													if (!NT_SUCCESS(Status))
													{
														//
														// ERROR case. No memory
														//
														PT_DBG_PRINT(PTDBG_TRACE_CRITICAL, ("nxrmflt!Can't allocate memory to build cache for %wZ\n", &NameInfo->Name));

														if (pNewCacheNode->ReleaseFileName)
														{
															ExFreePoolWithTag(pNewCacheNode->FileName.Buffer, NXRMFLT_NXLCACHE_TAG);
															RtlInitUnicodeString(&pNewCacheNode->FileName, NULL);
														}

														ExFreeToPagedLookasideList(&Global.NXLCacheLookaside, pNewCacheNode);
														pNewCacheNode = NULL;
														break;
													}

													AddNXLNodeToCache(&Global.NxlFileCache, pNewCacheNode);

													pNewCacheNode = NULL;
												} while (FALSE);
											} 
									
										}
									}
								}
								else
								{
									Data->IoStatus.Status = STATUS_ACCESS_DENIED;
									Data->IoStatus.Information = 0;
									CallbackStatus = FLT_PREOP_COMPLETE;
									break;
								}
							}
						}
					}
				}
			}

			// For RPM dir, when try to delete a nxl file in rpm folder, if the plain file is opened, deny
			if (IsInSafeDir && FlagOn(CreateOptions, FILE_DELETE_ON_CLOSE))
			{
				if (nxrmfltEndWithDotNXL(&PathPart))
				{
					UNICODE_STRING FileNameWithoutNXLExtension = { 0 };
					FileNameWithoutNXLExtension.Buffer = NameInfo->Name.Buffer;
					FileNameWithoutNXLExtension.MaximumLength = NameInfo->Name.MaximumLength - sizeof(NXRMFLT_NXL_DOTEXT) + sizeof(WCHAR);
					FileNameWithoutNXLExtension.Length = NameInfo->Name.Length - sizeof(NXRMFLT_NXL_DOTEXT) + sizeof(WCHAR);

					if (nxrmfltIsFileOpened(FltObjects->Instance, &FileNameWithoutNXLExtension, TRUE, NULL))
					{
						Data->IoStatus.Status = STATUS_ACCESS_DENIED;
						Data->IoStatus.Information = 0;
						CallbackStatus = FLT_PREOP_COMPLETE;
						break;
					}
				}
			}
        }

		if ((RtlCompareUnicodeString(&LegacyThemesFolder, &NameInfo->ParentDir, TRUE) == 0) && is_explorer())
		{
			Data->IoStatus.Status		= STATUS_ACCESS_DENIED;
			Data->IoStatus.Information	= 0;
			CallbackStatus				= FLT_PREOP_COMPLETE;
			
			break;
		}

		FltAcquirePushLockShared(&Global.NxlFileCacheLock);

		pCacheNode = FindNXLNodeInCache(&Global.NxlFileCache, &NameInfo->Name);

		if (pCacheNode)
		{
			if (ExAcquireRundownProtection(&pCacheNode->NodeRundownRef))
			{
				PT_DBG_PRINT(PTDBG_TRACE_CACHE_NODE, ("nxrmflt!nxrmfltPreCreate found file %wZ in cache\n", &NameInfo->Name));

				Flags = pCacheNode->Flags;

				CtxAttached = BooleanFlagOn(Flags, NXRMFLT_FLAG_CTX_ATTACHED);
			}
			else
			{
pCacheNode = NULL;
			}
		}

		FltReleasePushLock(&Global.NxlFileCacheLock);

		//
		// It's OK to use pCacheNode because we acquired rundown protection
		//
		if ((pCacheNode && CtxAttached == FALSE)
#ifdef  NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR
			|| IsInSanctuaryDir
#endif  // NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR
			)
		{
			do
			{
				IsExplorer = is_explorer();

				if (IsInSafeDir)
				{
					// It is NXL file in RPM Dir
					if (Global.ClientPort)
					{
						Status = nxrmfltCheckRights(getProcessIDFromData(Data),
													Data->Thread?PsGetThreadId(Data->Thread) : PsGetCurrentThreadId(),
													pCacheNode,
													FALSE,
													&RightsMask,
													NULL,
													NULL);
					}
					else
					{
						Status = STATUS_SUCCESS;
						RightsMask = 0;
					}


					if ((Status == STATUS_SUCCESS && (!(RightsMask & BUILTIN_RIGHT_VIEW))) || IsExplorer)	// always gives explorer NXL file instead of decrypted file even explorer has rights
					{
						//when try to delete a nxl file in rpm folder, if the plain file is opened, deny
						if (FlagOn(CreateOptions, FILE_DELETE_ON_CLOSE))
						{
							if (nxrmfltIsFileOpened(FltObjects->Instance, &NameInfo->Name, TRUE, NULL))
							{
								Data->IoStatus.Status = STATUS_ACCESS_DENIED;
								Data->IoStatus.Information = 0;
								CallbackStatus = FLT_PREOP_COMPLETE;
								SkipProcessingAfterCheckingRights = TRUE;
								break;
							}
						}

						Status = nxrmfltBlockPreCreate(Data,
													   &pCacheNode->OriginalFileName,
													   NameInfo);

						if (!NT_SUCCESS(Status))
						{
							PT_DBG_PRINT(PTDBG_TRACE_CRITICAL, ("nxrmflt!nxrmfltPreCreate: Failed to give encrypted content %wZ to PID %d! Status is %x\n", &pCacheNode->OriginalFileName, getProcessIDFromData(Data), Status));

							Data->IoStatus.Status		= STATUS_ACCESS_DENIED;
							Data->IoStatus.Information	= 0;
						}

						CallbackStatus = FLT_PREOP_COMPLETE;
						SkipProcessingAfterCheckingRights = TRUE;
						break;
					}
				}
				else
				{
#ifdef  NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR
					// It is non-NXL file in Sanctuary Dir
					BOOLEAN Trusted = FALSE;
					if (Global.ClientPort)
					{
						Status = nxrmfltCheckTrust(getProcessIDFromData(Data), &Trusted);
					}
					else
					{
						Status = STATUS_SUCCESS;
						Trusted = FALSE;
					}

					if ((Status == STATUS_SUCCESS && !Trusted) && !IsExplorer)
					{
						// Process is not Explorer and is untrusted.  Deny access.
						Data->IoStatus.Status		= STATUS_ACCESS_DENIED;
						Data->IoStatus.Information	= 0;
						CallbackStatus = FLT_PREOP_COMPLETE;
					}
					else
					{
						// Process is Explorer or is trusted.

						// If process is not nxrmserv.exe, check the Windows session of this process and see if the use is
						// logged in in that Windows session.  (nxrmserv.exe does not run in a user session.)
						if (!IsNxrmserv)
						{
							ULONG SessionId = 0;

							Status = FltGetRequestorSessionId(Data, &SessionId);
							if (!NT_SUCCESS(Status) || !nxrmfltIsUserLoggedOn(SessionId))
							{
								// User in this Windows session is not logged in.  Deny access.
								Data->IoStatus.Status		= STATUS_ACCESS_DENIED;
								Data->IoStatus.Information	= 0;
								CallbackStatus = FLT_PREOP_COMPLETE;
							}
						}

						if (IsExplorer && nxrmfltIsZipExt(&NameInfo->Extension) &&
							FlagOn(Data->Iopb->Parameters.Create.SecurityContext->DesiredAccess, FILE_READ_DATA))
						{
							// Process is Explorer, file is .zip file, and process wants to read file data.	 Deny access.  (Reading
							// attributes, ACL, etc. are still allowed.)
							Data->IoStatus.Status		= STATUS_ACCESS_DENIED;
							Data->IoStatus.Information	= 0;
							CallbackStatus = FLT_PREOP_COMPLETE;
						}
					}

					SkipProcessingAfterCheckingRights = TRUE;
#endif  // NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR
				}

			} while (FALSE);

			if (SkipProcessingAfterCheckingRights)
			{
				break;
			}

			do
			{
				//
				// file exists but no ctx attached to it
				// must be new open
				//
				SetFlag(pCacheNode->Flags, NXRMFLT_FLAG_ATTACHING_CTX);

				///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
				//
				// Here is a hack fix for SAP ECTR "open for edit" issue
				// Data flow is:
				//		1. User open a word file from ECTR via "open and edit" menu
				//		2. User change office and click "save and close", which will do file checkin
				//		3. Internally, Office will save file to disk, abc.docx
				//		4. ECTR Plugin (still in Office process) will copy abc.docx to temp file, abc_temp.docx
				//		5. ECTR Plugin also set file attribute as read-only
				//		6. However, in above 2 steps, our RPM driver clear the flag of abc.docx.nxl (not sure why, need further research)
				//		7. ECTR Plugin (Office process) now check abc.docx exists or not
				//		8. RPM driver decrypt the abc.docx from abc.docx.nxl again as it think there is no abc.docx for abc.docx.nxl.
				//		9. Now the abc.docx in disk is overwriten by the contents of abc.docx.nxl, and lost the changed content.
				//
				// The correct fix is
				//		a. find out why step4/5 clear the flag of abc.docx.nxl
				//		b. find out why step 8 didn't check the native file before decrypt
				//
				// For this MR, the workaround is to check whehter there is native file or not before decrypt, and only for office process.
				//
				if (is_msoffice_process())
				{
					BOOLEAN bReparseFileOpened = nxrmfltIsFileOpened(FltObjects->Instance, &pCacheNode->ReparseFileName, TRUE, NULL);
					if (nxrmfltDoesFileExist(FltObjects->Instance, &pCacheNode->ReparseFileName, TRUE) || bReparseFileOpened)
					{
						LONGLONG fileSize = 0;
						NTSTATUS ret = STATUS_SUCCESS;
						ret = nxrmfltGetFileSize(FltObjects->Instance, &pCacheNode->ReparseFileName, &fileSize);
						if (!NT_SUCCESS(ret))
						{
							PT_DBG_PRINT(PTDBG_TRACE_CRITICAL, ("nxrmflt!nxrmfltPreCreate: when query file size, failed to open %wZ!\n", &pCacheNode->ReparseFileName));
							if (bReparseFileOpened)
							{
								PT_DBG_PRINT(PTDBG_TRACE_CRITICAL, ("nxrmflt!nxrmfltPreCreate: ReparseFileName already  there %wZ!\n", &pCacheNode->ReparseFileName));
								ClearFlag(pCacheNode->Flags, NXRMFLT_FLAG_ATTACHING_CTX);
								SetFlag(pCacheNode->Flags, NXRMFLT_FLAG_CTX_ATTACHED);
								break;
							}
						}
						else if (fileSize == 0)
						{
							PT_DBG_PRINT(PTDBG_TRACE_CRITICAL, ("nxrmflt!nxrmfltPreCreate: file size is 0, %wZ!\n", &pCacheNode->ReparseFileName));
						}
						else
						{
							PT_DBG_PRINT(PTDBG_TRACE_CRITICAL, ("nxrmflt!nxrmfltPreCreate: ReparseFileName already  there %wZ!\n", &pCacheNode->ReparseFileName));
							ClearFlag(pCacheNode->Flags, NXRMFLT_FLAG_ATTACHING_CTX);
							SetFlag(pCacheNode->Flags, NXRMFLT_FLAG_CTX_ATTACHED);
							break;
						}
					}
					else
					{
						PT_DBG_PRINT(PTDBG_TRACE_CRITICAL, ("nxrmflt!nxrmfltPreCreate: ReparseFileName NOT there %wZ! for %wZ\n", &pCacheNode->ReparseFileName, &pCacheNode->OriginalFileName));
					}
				}
				///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

				Status = nxrmfltDecryptFile(Data,
											FltObjects->Instance,
											FltObjects->Instance,
											&pCacheNode->OriginalFileName,
											&pCacheNode->ReparseFileName);

				if (!NT_SUCCESS(Status))
				{
					PT_DBG_PRINT(PTDBG_TRACE_CRITICAL,("nxrmflt!nxrmfltPreCreate: Failed to decrypt file %wZ! Status is %x\n", &pCacheNode->OriginalFileName, Status));
					break;
				}

				Status = nxrmfltAddReparseECP(Data, NameInfo, FltObjects->Instance, NULL);

				if (!NT_SUCCESS(Status))
				{
					PT_DBG_PRINT(PTDBG_TRACE_CRITICAL,("nxrmflt!nxrmfltPreCreate: Failed to add ECP to %wZ! Status is %x\n", &NameInfo->Name, Status));
					break;
				}

				//
				// Only return STATUS_REPARSE when the opening file is NOT the same as re-parsed file
				//
				if (RtlEqualUnicodeString(&NameInfo->Name, &pCacheNode->ReparseFileName, TRUE))
				{
					CallbackStatus = FLT_PREOP_SUCCESS_WITH_CALLBACK;
				}
				else
				{
					Status = IoReplaceFileObjectName(Data->Iopb->TargetFileObject,
													 pCacheNode->ReparseFileName.Buffer,
													 pCacheNode->ReparseFileName.Length);
					if (!NT_SUCCESS(Status))
					{
						//
						// remove ECP in this case
						//
						nxrmfltDeleteAddedReparseECP(Data);

						PT_DBG_PRINT(PTDBG_TRACE_CRITICAL, ("nxrmflt!nxrmfltPreCreate: Failed to reparse %wZ! Status is %x\n", &NameInfo->Name, Status));
						break;
					}

					Data->IoStatus.Status = STATUS_REPARSE;
					Data->IoStatus.Information = IO_REPARSE;
					CallbackStatus = FLT_PREOP_COMPLETE;
				}

			} while (FALSE);

			if (!NT_SUCCESS(Status))
			{
				//when try to delete a nxl file in rpm folder, if the plain file is opened, deny
				if (FlagOn(CreateOptions, FILE_DELETE_ON_CLOSE))
				{
					if (nxrmfltIsFileOpened(FltObjects->Instance, &NameInfo->Name, TRUE, NULL))
					{
						Data->IoStatus.Status = STATUS_ACCESS_DENIED;
						Data->IoStatus.Information = 0;
						CallbackStatus = FLT_PREOP_COMPLETE;
						break;
					}
				}

				//
				// clear flag in the case of error
				//
				ClearFlag(pCacheNode->Flags, NXRMFLT_FLAG_ATTACHING_CTX);

				Status = nxrmfltBlockPreCreate(Data,
											   &pCacheNode->OriginalFileName,
											   NameInfo);

				if (!NT_SUCCESS(Status))
				{
					PT_DBG_PRINT(PTDBG_TRACE_CRITICAL, ("nxrmflt!nxrmfltPreCreate: Failed to give encrypted content %wZ to PID %d! Status is %x\n", &pCacheNode->OriginalFileName, getProcessIDFromData(Data), Status));
					
					Data->IoStatus.Status		= STATUS_ACCESS_DENIED;
					Data->IoStatus.Information	= 0;
				}

				CallbackStatus	= FLT_PREOP_COMPLETE;
				
				break;
			}

			ReleaseNameInfo = FALSE;

			break;
		}
		else if (pCacheNode)
		{
			do 
			{
				IsExplorer = is_explorer();

				IsExplorerDeleteFile = (IsExplorer && (CreateDisposition == FILE_OPEN && FlagOn(Data->Iopb->Parameters.Create.SecurityContext->DesiredAccess, DELETE)));

				IsExplorerRequiringOpLock = (IsExplorer && (CreateDisposition == FILE_OPEN && FlagOn(CreateOptions, FILE_OPEN_REQUIRING_OPLOCK)));

				if (Global.ClientPort)
				{
					Status = nxrmfltCheckRights(getProcessIDFromData(Data),
												Data->Thread?PsGetThreadId(Data->Thread) : PsGetCurrentThreadId(),
												pCacheNode,
												FALSE,
												&RightsMask,
												NULL,
												NULL);
				}
				else
				{
					Status = STATUS_SUCCESS;
					RightsMask = 0;
				}

				if (IsExplorer && IsExplorerRequiringOpLock && Data->Iopb->Parameters.Create.ShareAccess != FILE_SHARE_VALID_FLAGS)
				{
					Data->IoStatus.Status		= STATUS_SHARING_VIOLATION;
					Data->IoStatus.Information	= 0;
					CallbackStatus				= FLT_PREOP_COMPLETE;

					SkipProcessingAfterCheckingRights = TRUE;
					break;
				}

				if ((Status == STATUS_SUCCESS && (!(RightsMask & BUILTIN_RIGHT_VIEW))) || ((IsExplorer) && (!IsExplorerDeleteFile)))
				{
					//when try to delete a nxl file in rpm folder, if the plain file is opened, deny
                    if (FlagOn(CreateOptions, FILE_DELETE_ON_CLOSE) || FlagOn(Data->Iopb->Parameters.Create.SecurityContext->DesiredAccess, DELETE))
                    {
						if (nxrmfltIsFileOpened(FltObjects->Instance, &NameInfo->Name, TRUE, NULL))
						{
							Data->IoStatus.Status = STATUS_ACCESS_DENIED;
							Data->IoStatus.Information = 0;
							CallbackStatus = FLT_PREOP_COMPLETE;
							SkipProcessingAfterCheckingRights = TRUE;
							break;
						}
					}

					Status = nxrmfltBlockPreCreate(Data,
												   &pCacheNode->OriginalFileName,
												   NameInfo);
					if (!NT_SUCCESS(Status))
					{
						PT_DBG_PRINT(PTDBG_TRACE_CRITICAL, ("nxrmflt!nxrmfltPreCreate: Failed to give encrypted content %wZ to PID %d! Status is %x\n", &pCacheNode->OriginalFileName, getProcessIDFromData(Data), Status));

						Data->IoStatus.Status		= STATUS_ACCESS_DENIED;
						Data->IoStatus.Information	= 0;
					}

					CallbackStatus = FLT_PREOP_COMPLETE;
					SkipProcessingAfterCheckingRights = TRUE;
					break;
				}
				else if (IsExplorer)
				{
					//
					// explorer delete NXL file or explorer requiring OpLock
					//
					Status = nxrmfltForceAccessCheck(FltObjects->Instance,
													 &NameInfo->Name,
													 Data->Iopb->Parameters.Create.SecurityContext->DesiredAccess,
													 Data->Iopb->Parameters.Create.FileAttributes,
													 Data->Iopb->Parameters.Create.ShareAccess,
													 CreateOptions);

					if (!NT_SUCCESS(Status) && (Status != STATUS_OBJECT_NAME_NOT_FOUND))
					{
						Data->IoStatus.Status		= Status;
						Data->IoStatus.Information	= 0;
						CallbackStatus				= FLT_PREOP_COMPLETE;

						SkipProcessingAfterCheckingRights = TRUE;
						break;
					}
					else
					{
						//when try to delete a nxl file in rpm folder, if the plain file is opened, deny
						if (FlagOn(CreateOptions, FILE_DELETE_ON_CLOSE))
						{
							if (nxrmfltIsFileOpened(FltObjects->Instance, &NameInfo->Name, TRUE, NULL))
							{
								Data->IoStatus.Status = STATUS_ACCESS_DENIED;
								Data->IoStatus.Information = 0;
								CallbackStatus = FLT_PREOP_COMPLETE;
								SkipProcessingAfterCheckingRights = TRUE;
								break;
							}
						}

						Status = nxrmfltBlockPreCreate(Data,
													   &pCacheNode->OriginalFileName,
													   NameInfo);
						if (!NT_SUCCESS(Status))
						{
							PT_DBG_PRINT(PTDBG_TRACE_CRITICAL, ("nxrmflt!nxrmfltPreCreate: Failed to give encrypted content %wZ to PID %d! Status is %x\n", &pCacheNode->OriginalFileName, getProcessIDFromData(Data), Status));

							Data->IoStatus.Status = STATUS_ACCESS_DENIED;
							Data->IoStatus.Information = 0;
						}

						CallbackStatus = FLT_PREOP_COMPLETE;
						SkipProcessingAfterCheckingRights = TRUE;
						break;
					}
				}

			} while (FALSE);

			if (SkipProcessingAfterCheckingRights)
			{
				break;
			}
			//
			// do whatever we need to do
			//
			
			//////////////////////////////////////////////////////////////////////////
			//
			// at this point, we need to see PostCreate if CreateDisposition is to
			// "Overwrite" the file. We only do it here where Ctx has been attached
			// to this file.
			// 
			//////////////////////////////////////////////////////////////////////////

			if ((CreateDisposition == FILE_SUPERSEDE) ||
				(CreateDisposition == FILE_OVERWRITE) ||
				(CreateDisposition == FILE_OVERWRITE_IF))
			{
				CallbackStatus = FLT_PREOP_SUCCESS_WITH_CALLBACK;
			}

		}
		else if (IsNXLFile(&NameInfo->Extension))
		{
			UNICODE_STRING FileNameWithoutNXLExtension = { 0 };

			BOOLEAN FoundInCache = FALSE;

			NXLFILE_CREATE_CONTEXT *CreateCtx = NULL;

			//
			// ignore opening NXL file second stream if NXL file is in cache 
			//
			if (NameInfo->Stream.Length)
			{
				break;
			}

			FileNameWithoutNXLExtension.Buffer = NameInfo->Name.Buffer;
			FileNameWithoutNXLExtension.Length = NameInfo->Name.Length - 4 * sizeof(WCHAR);
			FileNameWithoutNXLExtension.MaximumLength = NameInfo->Name.MaximumLength;

			FltAcquirePushLockShared(&Global.NxlFileCacheLock);

			do 
			{
				if (FindNXLNodeInCache(&Global.NxlFileCache, &FileNameWithoutNXLExtension))
				{
					FoundInCache = TRUE;
				}
			} while (FALSE);

			FltReleasePushLock(&Global.NxlFileCacheLock);

			//
			// ignore opening NXL file in cache if it's not truncate content
			//
			if (FoundInCache)
			{
				if ((CreateDisposition != FILE_SUPERSEDE) &&
					(CreateDisposition != FILE_OVERWRITE) &&
					(CreateDisposition != FILE_OVERWRITE_IF))
				{
					//
					// ignore explorer open NXL directly
					// most likely is our icon overlay
					//

					//Comment these, don't bypass explorer
					//IsExplorer = is_explorer();
					//
					//if (IsExplorer)
					//{
					//	break;
					//}

					Status = IoReplaceFileObjectName(Data->Iopb->TargetFileObject,
													 FileNameWithoutNXLExtension.Buffer,
													 FileNameWithoutNXLExtension.Length);
					if (!NT_SUCCESS(Status))
					{
						PT_DBG_PRINT(PTDBG_TRACE_CRITICAL, ("nxrmflt!nxrmfltPreCreate: Failed to force application with PID %d to open %wZ! Status is %x\n", getProcessIDFromData(Data), &FileNameWithoutNXLExtension, Status));

						Data->IoStatus.Status = STATUS_ACCESS_DENIED;
						Data->IoStatus.Information = 0;
						CallbackStatus = FLT_PREOP_COMPLETE;

						break;
					}

					Data->IoStatus.Status = STATUS_REPARSE;
					Data->IoStatus.Information = IO_REPARSE;
					CallbackStatus = FLT_PREOP_COMPLETE;

					break;
				}

				if (is_msoffice_process())
				{ 
					// for Office process, when operating on NXL file, shall not check the permission on non-NXL file
				}
				else
				{
					Status = nxrmfltForceAccessCheck(FltObjects->Instance,
						&FileNameWithoutNXLExtension,
						Data->Iopb->Parameters.Create.SecurityContext->DesiredAccess,
						Data->Iopb->Parameters.Create.FileAttributes,
						Data->Iopb->Parameters.Create.ShareAccess,
						CreateOptions);

					if (!NT_SUCCESS(Status) && (Status != STATUS_OBJECT_NAME_NOT_FOUND))
					{
						Data->IoStatus.Status = Status;
						Data->IoStatus.Information = 0;
						CallbackStatus = FLT_PREOP_COMPLETE;

						break;
					}

					PT_DBG_PRINT(PTDBG_TRACE_CRITICAL, ("nxrmflt!nxrmfltPreCreate: Failed to force check with PID %d Status is %x\n", getProcessIDFromData(Data), Status));
				}
			}

			
			// 
			// this means application is opening a NXL file that not in our cache
			// we need to exam this file in PostCreate
			//

			//
			// We check instance context to make sure this NXL file is not on remote/removable media
			//

			Status = FltGetInstanceContext(FltObjects->Instance, &InstCtx);

			if (!NT_SUCCESS(Status))
			{
				break;
			}

			if (InstCtx->DisableFiltering)
			{
				break;
			}

			CreateCtx = ExAllocateFromPagedLookasideList(&Global.NXLFileCreateCtxLookaside);

			if (!CreateCtx)
			{
				break;
			}

			RtlSecureZeroMemory(CreateCtx, sizeof(*CreateCtx));

			CreateCtx->Flags |= FoundInCache ? NXRMFLT_NXLFILE_CREATE_CTX_FLAG_FILE_IS_IN_CACHE : 0;
			CreateCtx->NameInfo = NameInfo;
			CreateCtx->FileNameWithoutNXLExtension = FileNameWithoutNXLExtension;

			PT_DBG_PRINT(PTDBG_TRACE_CRITICAL, ("nxrmflt!nxrmfltPreCreate: check PostCreate file %wZ! \n", &FileNameWithoutNXLExtension));

			//
			// prevent releasing NameInfo
			//
			ReleaseNameInfo = FALSE;

			*CompletionContext = CreateCtx;
			
			CallbackStatus = FLT_PREOP_SUCCESS_WITH_CALLBACK;

			break;
		}
		else
		{
			//
			// in this case, we need to check SaveAs forecast to see if this "Create" in SaveAs forecast
			//
			FltAcquirePushLockShared(&Global.ExpireTableLock);

			FOR_EACH_LIST(ite, &Global.ExpireTable)
			{
				pSaveAsNode = CONTAINING_RECORD(ite, NXL_SAVEAS_NODE, Link);

				if (0 == RtlCompareUnicodeString(&pSaveAsNode->SaveAsFileName, &NameInfo->Name, TRUE) && 
					pSaveAsNode->ProcessId == getProcessIDFromData(Data))
				{
					if (!ExAcquireRundownProtection(&pSaveAsNode->NodeRundownRef))
					{
						pSaveAsNode = NULL;
					}

					break;
				}
				else if (NameInfo->Share.Length &&
						 pSaveAsNode->ProcessId == getProcessIDFromData(Data))
				{
					UNICODE_STRING MupDevice = {0};

					RtlInitUnicodeString(&MupDevice, NXRMFLT_MUP_PREFIX);

					if (!RtlPrefixUnicodeString(&MupDevice, &NameInfo->Name, TRUE))
					{
						pSaveAsNode = NULL;
						continue;
					}

					if (!CompareMupDeviceNames(&pSaveAsNode->SaveAsFileName, &NameInfo->Name))
					{
						pSaveAsNode = NULL;
						continue;
					}
					
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

			if (!pSaveAsNode)
			{
				break;
			}

			//
			// Check source file
			// Output of following block of code is "pSaveAsSourceCacheNode" which represents source file
			//
			do 
			{
				if (pSaveAsNode->SourceFileName.Length)
				{
					FltAcquirePushLockShared(&Global.NxlFileCacheLock);

					pSaveAsSourceCacheNode = FindNXLNodeInCache(&Global.NxlFileCache, &pSaveAsNode->SourceFileName);

					if (pSaveAsSourceCacheNode)
					{
						if (!ExAcquireRundownProtection(&pSaveAsSourceCacheNode->NodeRundownRef))
						{
							pSaveAsSourceCacheNode = NULL;
						}
					}

					FltReleasePushLock(&Global.NxlFileCacheLock);

					break;
				}
				else
				{
					//
					// comment out following temporarily to avoid encrypting file if we don't know the soruce file
					//
					//if (STATUS_NOT_FOUND == nxrmfltGuessSourceFileFromProcessCache(getProcessIDFromData(Data), &pSaveAsNode->SourceFileName))
					//{
					//	pSaveAsSourceCacheNode = NULL;
					//	break;
					//}

					//FltAcquirePushLockShared(&Global.NxlFileCacheLock);

					//pSaveAsSourceCacheNode = FindNXLNodeInCache(&Global.NxlFileCache, &pSaveAsNode->SourceFileName);

					//if (pSaveAsSourceCacheNode)
					//{
					//	if (!ExAcquireRundownProtection(&pSaveAsSourceCacheNode->NodeRundownRef))
					//	{
					//		pSaveAsSourceCacheNode = NULL;
					//	}
					//}
					//
					//FltReleasePushLock(&Global.NxlFileCacheLock);
				}

			} while (FALSE);

			if (!pSaveAsSourceCacheNode)
			{
				//
				// Source file is NOT a NXL file or this process is not dirty
				// Let it go
				//
				break;
			}

			//
			// we need to check rights for this SaveAs operation
			//
			if (!Global.ClientPort)
			{
				//
				// service stopped?
				//
				break;
			}

			Status = nxrmfltCheckRights(getProcessIDFromData(Data),
										Data->Thread?PsGetThreadId(Data->Thread) : PsGetCurrentThreadId(),
										pSaveAsSourceCacheNode,
										FALSE,
										&RightsMask,
										NULL,
										NULL);

			if (Status == STATUS_SUCCESS && RightsMask == 0)
			{
				Data->IoStatus.Status		= STATUS_ACCESS_DENIED;
				Data->IoStatus.Information	= 0;
				CallbackStatus				= FLT_PREOP_COMPLETE;

				break;
			}

			if (Status == STATUS_SUCCESS && (!(RightsMask & BUILTIN_RIGHT_SAVEAS)) && (RightsMask != 0))
			{
				if (FlagOn(Data->Iopb->Parameters.Create.SecurityContext->DesiredAccess, FILE_WRITE_DATA) ||
					(CreateDisposition == FILE_SUPERSEDE) ||
					(CreateDisposition == FILE_OVERWRITE) ||
					(CreateDisposition == FILE_OVERWRITE_IF))
				{
					Data->IoStatus.Status		= STATUS_ACCESS_DENIED;
					Data->IoStatus.Information	= 0;
					CallbackStatus				= FLT_PREOP_COMPLETE;

					nxrmfltSendBlockNotificationMsg(Data, &NameInfo->Name, nxrmfltDeniedSaveAsOpen);

					break;
				}
			}

			Status = FltGetInstanceContext(FltObjects->Instance, &InstCtx);

			if (!NT_SUCCESS(Status))
			{
				//
				// Source file is NXL file or Process is dirty but we can't get instance ctx
				// deny access
				//
				Data->IoStatus.Status		= STATUS_ACCESS_DENIED;
				Data->IoStatus.Information	= 0;
				CallbackStatus				= FLT_PREOP_COMPLETE;

				nxrmfltSendBlockNotificationMsg(Data, &NameInfo->Name, nxrmfltSaveAsToUnprotectedVolume);

				break;
			}

			if (InstCtx->DisableFiltering)
			{
				//
				// Source file is NXL file or Process is dirty but we don't filtering this volume
				// deny access
				//
				Data->IoStatus.Status		= STATUS_ACCESS_DENIED;
				Data->IoStatus.Information	= 0;
				CallbackStatus				= FLT_PREOP_COMPLETE;

				nxrmfltSendBlockNotificationMsg(Data, &NameInfo->Name, nxrmfltSaveAsToUnprotectedVolume);

				break;
			}

			//
			// Adding ECP
			//
			Status = nxrmfltAddReparseECP(Data, NameInfo, FltObjects->Instance, &pSaveAsSourceCacheNode->FileName);

			if (!NT_SUCCESS(Status))
			{
				PT_DBG_PRINT(PTDBG_TRACE_CRITICAL,("nxrmflt!nxrmfltPreCreate: Failed to add ECP to %wZ! Status is %x\n", &NameInfo->Name, Status));

				Data->IoStatus.Status		= STATUS_ACCESS_DENIED;
				Data->IoStatus.Information	= 0;
				CallbackStatus				= FLT_PREOP_COMPLETE;

				break;
			}

			ReleaseNameInfo = FALSE;

			*CompletionContext = NULL;

			CallbackStatus = FLT_PREOP_SUCCESS_WITH_CALLBACK;

		}

	} while (FALSE);

	if (ReleaseNameInfo && NameInfo)
	{
		FltReleaseFileNameInformation(NameInfo);
	}

	if (InstCtx)
	{
		FltReleaseContext(InstCtx);
	}

	if (pCacheNode)
	{
		ExReleaseRundownProtection(&pCacheNode->NodeRundownRef);
	}
	
	if (pSaveAsSourceCacheNode)
	{
		ExReleaseRundownProtection(&pSaveAsSourceCacheNode->NodeRundownRef);
	}

	if (FileNameAppendNXLExtensionBuffer)
	{
		ExFreeToPagedLookasideList(&Global.FullPathLookaside, FileNameAppendNXLExtensionBuffer);
	}

	if (pSaveAsNode)
	{
		ExReleaseRundownProtection(&pSaveAsNode->NodeRundownRef);
	}

	return CallbackStatus;
}

FLT_POSTOP_CALLBACK_STATUS nxrmfltPostCreate(
	_Inout_ PFLT_CALLBACK_DATA		Data,
	_In_ PCFLT_RELATED_OBJECTS		FltObjects,
	_In_opt_ PVOID					CompletionContext,
	_In_ FLT_POST_OPERATION_FLAGS	Flags)
{
	NTSTATUS Status = STATUS_SUCCESS;
	FLT_POSTOP_CALLBACK_STATUS	CallbackStatus = FLT_POSTOP_FINISHED_PROCESSING;

	NXRMFLT_STREAM_CONTEXT *Ctx = NULL;
	NXRMFLT_STREAMHANDLE_CONTEXT *Ccb = NULL;

	PFLT_INSTANCE	FltInstance = NULL;
	PFILE_OBJECT	FileObject = NULL;
	NXRMFLT_REPARSE_ECP_CONTEXT *EcpCtx = NULL;

	NXL_CACHE_NODE *pCacheNode = NULL;

	ULONG DirHash = 0;

	LARGE_INTEGER FileId = {0};
	ULONG FileAttributes = 0;

	LIST_ENTRY *ite = NULL;
	LIST_ENTRY *tmp = NULL;

	LIST_ENTRY	ExpiredNodeList = {0};

	ULONG CreateDisposition = 0;
	ULONG CreateOptions = 0;

	NXLFILE_CREATE_CONTEXT *CreateCtx = NULL;

	ULONG SessionId = 0;

	do 
	{

		FltInstance = FltObjects->Instance;
		FileObject = FltObjects->FileObject;

		CreateDisposition = (Data->Iopb->Parameters.Create.Options & 0xff000000) >> 24;
		CreateOptions = (Data->Iopb->Parameters.Create.Options & 0x00ffffff);

		if (CompletionContext)
		{
			CreateCtx = (NXLFILE_CREATE_CONTEXT *)CompletionContext;

			do 
			{
				if (FlagOn(CreateCtx->Flags, NXRMFLT_NXLFILE_CREATE_CTX_FLAG_FILE_IS_IN_CACHE))
				{
					if (((CreateDisposition == FILE_SUPERSEDE) ||
						(CreateDisposition == FILE_OVERWRITE) ||
						(CreateDisposition == FILE_OVERWRITE_IF)) && Data->IoStatus.Status == STATUS_SUCCESS)
					{
						ULONG FileNameHash = 0;

						//
						// NXL file has been truncated
						//
						//
						// purge rights cache
						//

						RtlHashUnicodeString(&CreateCtx->FileNameWithoutNXLExtension, TRUE, HASH_STRING_ALGORITHM_X65599, &FileNameHash);

						nxrmfltPurgeRightsCache(FltInstance, FileNameHash);

						//
						// remove ctx attached flags
						//
						FltAcquirePushLockExclusive(&Global.NxlFileCacheLock);

						do
						{
							NXL_CACHE_NODE *pTmpNode = NULL;

							pTmpNode = FindNXLNodeInCache(&Global.NxlFileCache, &CreateCtx->FileNameWithoutNXLExtension);

							if(pTmpNode)
							{
								ClearFlag(pTmpNode->Flags, NXRMFLT_FLAG_CTX_ATTACHED);
							}

						} while (FALSE);

						FltReleasePushLock(&Global.NxlFileCacheLock);

						//
						// delete the decrypted copy
						//
						if (nxrmfltDoesFileExist(FltObjects->Instance, &CreateCtx->FileNameWithoutNXLExtension, TRUE))
						{
							if (nxrmfltIsFileOpened(FltObjects->Instance, &CreateCtx->FileNameWithoutNXLExtension, TRUE, NULL) && is_msoffice_process())
							{
								FltAcquirePushLockExclusive(&Global.NxlFileCacheLock);

								NXL_CACHE_NODE *pTmpNode = FindNXLNodeInCache(&Global.NxlFileCache, &CreateCtx->FileNameWithoutNXLExtension);
								if (pTmpNode)
								{
									ClearFlag(pTmpNode->Flags, NXRMFLT_FLAG_CTX_ATTACHED);
									FltReleasePushLock(&Global.NxlFileCacheLock);

									break;
								}

								FltReleasePushLock(&Global.NxlFileCacheLock);
							}
						}
									
						nxrmfltDeleteFileByName(FltInstance, &CreateCtx->FileNameWithoutNXLExtension);
					}

					break;
				}

				if (Data->IoStatus.Status != STATUS_SUCCESS || 
					Data->IoStatus.Information != FILE_OPENED)
				{
					//
					// if it's not successfully open an existing one ...
					//
					break;
				}

				RtlHashUnicodeString(&(CreateCtx->NameInfo->ParentDir), TRUE, HASH_STRING_ALGORITHM_X65599, &DirHash);

				//
				// Calling this function before acquiring the lock because this function generates I/O.
				//
				get_file_id_and_attribute_ex(FltInstance, FileObject, &FileId, &FileAttributes);

				FltAcquirePushLockExclusive(&Global.NxlFileCacheLock);

				do
				{
					pCacheNode = FindNXLNodeInCache(&Global.NxlFileCache, &(CreateCtx->FileNameWithoutNXLExtension));

					if (pCacheNode)
					{
						//
						// This is NOT expected
						//
						pCacheNode = NULL;
						break;
					}

					//
					// This is expected
					//
					pCacheNode = ExAllocateFromPagedLookasideList(&Global.NXLCacheLookaside);

					if (!pCacheNode)
					{
						//
						// very unlikely, out of memory?
						//
						break;
					}

					memset(pCacheNode, 0, sizeof(NXL_CACHE_NODE));

					Status = build_nxlcache_file_name(pCacheNode, &(CreateCtx->FileNameWithoutNXLExtension));

					if (!NT_SUCCESS(Status))
					{
						//
						// ERROR case. No memory
						//
						PT_DBG_PRINT(PTDBG_TRACE_CRITICAL, ("nxrmflt!Can't allocate memory to build cache for %wZ\n", &(CreateCtx->FileNameWithoutNXLExtension)));

						ExFreeToPagedLookasideList(&Global.NXLCacheLookaside, pCacheNode);
						pCacheNode = NULL;
						break;
					}

					pCacheNode->FileAttributes = FileAttributes;
					pCacheNode->FileID.QuadPart = FileId.QuadPart;
					pCacheNode->Flags |= FlagOn(FileAttributes, FILE_ATTRIBUTE_READONLY) ? NXRMFLT_FLAG_READ_ONLY : 0;
					pCacheNode->Flags |= NXRMFLT_FLAG_ATTACHING_CTX;
					pCacheNode->Instance = FltObjects->Instance;
					pCacheNode->ParentDirectoryHash = DirHash;
					pCacheNode->OnRemoveOrRemovableMedia = FALSE;
					pCacheNode->LastWriteProcessId = NULL;

					ExInitializeRundownProtection(&pCacheNode->NodeRundownRef);

					Status = build_nxlcache_reparse_file_name(pCacheNode, &(CreateCtx->FileNameWithoutNXLExtension));

					if (!NT_SUCCESS(Status))
					{
						//
						// ERROR case. No memory
						//
						PT_DBG_PRINT(PTDBG_TRACE_CRITICAL, ("nxrmflt!Can't allocate memory to build cache for %wZ\n", &(CreateCtx->FileNameWithoutNXLExtension)));

						if (pCacheNode->ReleaseFileName)
						{
							ExFreePoolWithTag(pCacheNode->FileName.Buffer, NXRMFLT_NXLCACHE_TAG);
							RtlInitUnicodeString(&pCacheNode->FileName, NULL);
						}

						ExFreeToPagedLookasideList(&Global.NXLCacheLookaside, pCacheNode);
						pCacheNode = NULL;
						break;
					}

					PT_DBG_PRINT(PTDBG_TRACE_CACHE_NODE, ("nxrmflt!AddNXLNodeToCache add file %wZ into cache\n", &(CreateCtx->FileNameWithoutNXLExtension)));

					AddNXLNodeToCache(&Global.NxlFileCache, pCacheNode);

					pCacheNode = NULL;

				} while (FALSE);

				FltReleasePushLock(&Global.NxlFileCacheLock);

			} while (FALSE);

			//
			// let's break for now. it does not make sense to continue at this time
			//
			break;
		}

		Status = FltGetStreamContext(FltInstance, FileObject, &Ctx);

		if (Ctx)
		{
			if (((CreateDisposition == FILE_SUPERSEDE) ||
				 (CreateDisposition == FILE_OVERWRITE) ||
				 (CreateDisposition == FILE_OVERWRITE_IF)) && Data->IoStatus.Status == STATUS_SUCCESS)
			{
				Ctx->ContentDirty = TRUE;
			}

			EcpCtx = (NXRMFLT_REPARSE_ECP_CONTEXT *)nxrmfltGetReparseECP(Data);

			if (EcpCtx)
			{
				FltAcknowledgeEcp(Global.Filter, EcpCtx);
			}

			break;
		}

		if (FlagOn(CreateOptions, FILE_DELETE_ON_CLOSE))
		{
			//
			// add stream handle context to track delete on close
			//
			Status = FltAllocateContext(Global.Filter,
										FLT_STREAMHANDLE_CONTEXT,
										sizeof(NXRMFLT_STREAMHANDLE_CONTEXT),
										NonPagedPool,
										(PFLT_CONTEXT*)&Ccb);

			if (!NT_SUCCESS(Status))
			{
				PT_DBG_PRINT(PTDBG_TRACE_CRITICAL, ("nxrmflt!nxrmfltPostCreate: Failed to allocate stream handle ctx!\n"));

				break;
			}

			Ccb->DestinationFileNameInfo = NULL;
			Ccb->SourceFileNameInfo = NULL;
			Ccb->SourceFileIsNxlFile = FALSE;
			Ccb->EncryptDestinationFile = FALSE;
			Ccb->DeleteOnClose = TRUE;
			Ccb->Reserved = 0;
			
			Status = FltSetStreamHandleContext(FltInstance,
											   FileObject,
											   FLT_SET_CONTEXT_KEEP_IF_EXISTS,
											   Ccb,
											   NULL);

			if (!NT_SUCCESS(Status))
			{
				//
				// ignore the error and continue
				//
				PT_DBG_PRINT(PTDBG_TRACE_CRITICAL, ("nxrmflt!nxrmfltPostCreate: FltSetStreamHandleContext return %x\n", Status));
			}

		}

		//
		// If Data->IoStatus.Status is STATUS_REPARSE, it means that our
		// PostCreate callback are invoked not because the Create request is
		// being completed, but because a lower driver wants to do a reparse
		// (e.g. due to junctions or symbolic links within the file path).
		// Then our PreCreate callback will be invoked with the same request
		// again.  Therefore, we should not consume our Reparse ECP at this
		// point.  Instead, we will wait until no lower drivers want to do any
		// more reparses.
		//
		if (Data->IoStatus.Status == STATUS_REPARSE)
		{
			break;
		}

		EcpCtx = (NXRMFLT_REPARSE_ECP_CONTEXT *)nxrmfltGetReparseECP(Data);

		if (!EcpCtx)
		{
			break;
		}

		FltAcknowledgeEcp(Global.Filter, EcpCtx);

		if ((Data->IoStatus.Status != STATUS_SUCCESS && Data->IoStatus.Status != STATUS_OBJECT_NAME_COLLISION))
		{
			break;
		}

		//
		// Create NXL cache node if it's a SaveAs Open
		//

		if (EcpCtx->SourceFileName.Length)
		{
			RtlHashUnicodeString(&(EcpCtx->NameInfo->ParentDir), TRUE, HASH_STRING_ALGORITHM_X65599, &DirHash);

			//
			// Calling this function before acquiring the lock because this function generates I/O.
			//
			get_file_id_and_attribute_ex(FltInstance, FileObject, &FileId, &FileAttributes);

			FltAcquirePushLockExclusive(&Global.NxlFileCacheLock);

			do
			{
				pCacheNode = FindNXLNodeInCache(&Global.NxlFileCache, &(EcpCtx->NameInfo->Name));

				if (pCacheNode)
				{
					//
					// This is NOT expected
					//
					pCacheNode = NULL;
					break;
				}

				//
				// This is expected
				//
				pCacheNode = ExAllocateFromPagedLookasideList(&Global.NXLCacheLookaside);

				if (!pCacheNode)
				{
					//
					// very unlikely, out of memory?
					//
					break;
				}

				memset(pCacheNode, 0, sizeof(NXL_CACHE_NODE));

				Status = build_nxlcache_file_name(pCacheNode, &(EcpCtx->NameInfo->Name));

				if (!NT_SUCCESS(Status))
				{
					//
					// ERROR case. No memory
					//
					PT_DBG_PRINT(PTDBG_TRACE_CRITICAL, ("nxrmflt!Can't allocate memory to build cache for %wZ\n", &(EcpCtx->NameInfo->Name)));

					ExFreeToPagedLookasideList(&Global.NXLCacheLookaside, pCacheNode);
					pCacheNode = NULL;
					break;
				}

				pCacheNode->FileAttributes = FileAttributes;
				pCacheNode->FileID.QuadPart = FileId.QuadPart;
				pCacheNode->Flags |= FlagOn(FileAttributes, FILE_ATTRIBUTE_READONLY) ? NXRMFLT_FLAG_READ_ONLY : 0;
				pCacheNode->Flags |= NXRMFLT_FLAG_ATTACHING_CTX;
				pCacheNode->Instance = FltObjects->Instance;
				pCacheNode->ParentDirectoryHash = DirHash;
				pCacheNode->OnRemoveOrRemovableMedia = FALSE;
				pCacheNode->LastWriteProcessId = NULL;

				ExInitializeRundownProtection(&pCacheNode->NodeRundownRef);

				Status = build_nxlcache_reparse_file_name(pCacheNode, &(EcpCtx->NameInfo->Name));

				if (!NT_SUCCESS(Status))
				{
					//
					// ERROR case. No memory
					//
					PT_DBG_PRINT(PTDBG_TRACE_CRITICAL, ("nxrmflt!Can't allocate memory to build cache for %wZ\n", &(EcpCtx->NameInfo->Name)));

					if (pCacheNode->ReleaseFileName)
					{
						ExFreePoolWithTag(pCacheNode->FileName.Buffer, NXRMFLT_NXLCACHE_TAG);
						RtlInitUnicodeString(&pCacheNode->FileName, NULL);
					}

					ExFreeToPagedLookasideList(&Global.NXLCacheLookaside, pCacheNode);
					pCacheNode = NULL;
					break;
				}

				PT_DBG_PRINT(PTDBG_TRACE_CACHE_NODE, ("nxrmflt!AddNXLNodeToCache add file %wZ into cache\n", &(EcpCtx->NameInfo->Name)));

				AddNXLNodeToCache(&Global.NxlFileCache, pCacheNode);

				if (!ExAcquireRundownProtection(&pCacheNode->NodeRundownRef))
				{
					pCacheNode = NULL;
				}

			} while (FALSE);

			FltReleasePushLock(&Global.NxlFileCacheLock);

			if (pCacheNode)
			{
				NXL_CACHE_NODE *pSrcCacheNode = NULL;

				FltAcquirePushLockShared(&Global.NxlFileCacheLock);

				pSrcCacheNode = FindNXLNodeInCache(&Global.NxlFileCache, &EcpCtx->SourceFileName);

				if (pSrcCacheNode)
				{
					if (!ExAcquireRundownProtection(&pSrcCacheNode->NodeRundownRef))
					{
						pSrcCacheNode = NULL;
					}
				}

				FltReleasePushLock(&Global.NxlFileCacheLock);

				if (pSrcCacheNode)
				{
					Status = FltGetRequestorSessionId(Data, &SessionId);

					if (!NT_SUCCESS(Status))
					{
						break;
					}

					Status = nxrmfltCreateEmptyNXLFileFromExistingNXLFile(FltInstance,
																		  getProcessIDFromData(Data),
																		  SessionId,
																		  &pCacheNode->OriginalFileName,
																		  pSrcCacheNode);
					if (!NT_SUCCESS(Status))
					{
						PT_DBG_PRINT(PTDBG_TRACE_CRITICAL, ("nxrmflt!Can't create NXL file %wZ\n", &pCacheNode->OriginalFileName));
					}

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

					ExReleaseRundownProtection(&pSrcCacheNode->NodeRundownRef);

					pSrcCacheNode = NULL;
				}

				ExReleaseRundownProtection(&pCacheNode->NodeRundownRef);

				pCacheNode = NULL;
			}
		}

		if (is_adobe_like_process())
		{
			FltAcquirePushLockShared(&Global.NxlFileCacheLock);

			pCacheNode = FindNXLNodeInCache(&Global.NxlFileCache, &EcpCtx->NameInfo->Name);

			if (pCacheNode)
			{
				SetFlag(pCacheNode->Flags, NXRMFLT_FLAG_CTX_ATTACHED);
				ClearFlag(pCacheNode->Flags, NXRMFLT_FLAG_ATTACHING_CTX);
			}

			FltReleasePushLock(&Global.NxlFileCacheLock);

			break;
		}

		Status = FltAllocateContext(Global.Filter, 
									FLT_STREAM_CONTEXT, 
									sizeof(NXRMFLT_STREAM_CONTEXT), 
									NonPagedPool, 
									(PFLT_CONTEXT*)&Ctx);

		if (!NT_SUCCESS(Status))
		{
			break;
		}

		//
		// Initialize Ctx
		//
		memset(Ctx, 0, sizeof(NXRMFLT_STREAM_CONTEXT));

		//
		// make sure CtxCleanup won't free NULL point in case there is error when building this Ctx
		//
		Ctx->ReleaseFileName = FALSE;

		InterlockedIncrement(&Global.TotalContext);

		Status = nxrmfltBuildNamesInStreamContext(Ctx, &EcpCtx->NameInfo->Name);

		if(!NT_SUCCESS(Status))
		{
			break;
		}

		Ctx->OriginalInstance			= EcpCtx->OriginalInstance;

		Status = FltGetRequestorSessionId(Data, &Ctx->RequestorSessionId);

		if (!NT_SUCCESS(Status))
		{
			Ctx->RequestorSessionId = NXRMFLT_INVALID_SESSION_ID;
		}

		FltInitializePushLock(&Ctx->CtxLock);

		if (((CreateDisposition == FILE_SUPERSEDE) ||
			 (CreateDisposition == FILE_OVERWRITE) ||
			 (CreateDisposition == FILE_OVERWRITE_IF)) && Data->IoStatus.Status == STATUS_SUCCESS)
		{
			Ctx->ContentDirty = TRUE;
		}

		//
		// force sync when SaveAs
		//
		if (EcpCtx->SourceFileName.Length)
		{
			Ctx->ContentDirty = TRUE;
		}

		Status = FltSetStreamContext(FltInstance,
									 FileObject,
									 FLT_SET_CONTEXT_KEEP_IF_EXISTS,
									 Ctx,
									 NULL);

		if (!NT_SUCCESS(Status))
		{
			break;
		}

		FltAcquirePushLockShared(&Global.NxlFileCacheLock);

		pCacheNode = FindNXLNodeInCache(&Global.NxlFileCache, &Ctx->FileName);

		if(pCacheNode)
		{
			SetFlag(pCacheNode->Flags, NXRMFLT_FLAG_CTX_ATTACHED);
			ClearFlag(pCacheNode->Flags, NXRMFLT_FLAG_ATTACHING_CTX);
		}
		
		FltReleasePushLock(&Global.NxlFileCacheLock);

		//
		// Remove SaveAsNode from expire table
		//

		//InitializeListHead(&ExpiredNodeList);

		//if (EcpCtx->SourceFileName.Length)
		//{
		//	FltAcquirePushLockExclusive(&Global.ExpireTableLock);

		//	FOR_EACH_LIST_SAFE(ite, tmp, &Global.ExpireTable)
		//	{
		//		NXL_SAVEAS_NODE *pSaveAsNode = CONTAINING_RECORD(ite, NXL_SAVEAS_NODE, Link);

		//		if (0 == RtlCompareUnicodeString(&pSaveAsNode->SaveAsFileName, &(EcpCtx->NameInfo->Name), TRUE) &&
		//			pSaveAsNode->ProcessId == getProcessIDFromData(Data))
		//		{
		//			RemoveEntryList(ite);

		//			InsertHeadList(&ExpiredNodeList, &pSaveAsNode->Link);
		//		}
		//	}

		//	FltReleasePushLock(&Global.ExpireTableLock);

		//	FOR_EACH_LIST_SAFE(ite, tmp, &ExpiredNodeList)
		//	{
		//		NXL_SAVEAS_NODE *pSaveAsNode = CONTAINING_RECORD(ite, NXL_SAVEAS_NODE, Link);

		//		RemoveEntryList(ite);

		//		//
		//		// Wait for all other threads rundown
		//		//
		//		ExWaitForRundownProtectionRelease(&pSaveAsNode->NodeRundownRef);

		//		ExRundownCompleted(&pSaveAsNode->NodeRundownRef);

		//		memset(pSaveAsNode, 0, sizeof(NXL_SAVEAS_NODE));

		//		ExFreeToPagedLookasideList(&Global.SaveAsExpireLookaside, pSaveAsNode);
		//	}
		//}

	} while (FALSE);

	//
	// It's possible that we need to release ECP nameinfo and release Ctx
	// This happens when two creates happen at the same time
	//
	if(EcpCtx && EcpCtx->NameInfo)
	{
		FltReleaseFileNameInformation(EcpCtx->NameInfo);
	}

	if (Ctx)
	{
		FltReleaseContext(Ctx);
	}

	if (Ccb)
	{
		FltReleaseContext(Ccb);
	}

	if (CreateCtx)
	{
		FltReleaseFileNameInformation(CreateCtx->NameInfo);

		ExFreeToPagedLookasideList(&Global.NXLFileCreateCtxLookaside, CreateCtx);
	}

	return CallbackStatus;
}

BOOLEAN IsNXLFile(PUNICODE_STRING Extension)
{
	BOOLEAN bRet = FALSE;

	do 
	{
        if(Extension && Extension->Length != 0)
		    bRet = (RtlCompareUnicodeString(&Global.NXLFileExtsion, Extension, TRUE) == 0);

	} while (FALSE);

	return bRet;
}

static void ascii2lower(UNICODE_STRING *str)
{
	WCHAR *p = str->Buffer;

	while (*p && (ULONG_PTR)p < (ULONG_PTR)((UCHAR*)str->Buffer + str->Length))
	{
		if (*p >= L'A' && *p <= L'Z')
		{
			*p += L'a' - L'A';
		}

		p++;
	}
}

static BOOLEAN IsReparseEcpPresent(PFLT_CALLBACK_DATA Data)
{
	NTSTATUS status = STATUS_SUCCESS;
	PECP_LIST ecpList = NULL;

	status = FltGetEcpListFromCallbackData(Global.Filter, Data, &ecpList);

	if (NT_SUCCESS(status) && (ecpList != NULL))
	{

		status = FltFindExtraCreateParameter(Global.Filter,
											 ecpList,
											 &GUID_ECP_NXRMFLT_REPARSE,
											 NULL,
											 NULL);

		if (NT_SUCCESS(status))
		{
			return TRUE;
		}
	}

	return FALSE;
}

static BOOLEAN IsBlockingEcpPresent(PFLT_CALLBACK_DATA Data)
{
	NTSTATUS status = STATUS_SUCCESS;
	PECP_LIST ecpList = NULL;

	status = FltGetEcpListFromCallbackData(Global.Filter, Data, &ecpList);

	if (NT_SUCCESS(status) && (ecpList != NULL))
	{

		status = FltFindExtraCreateParameter(Global.Filter,
											 ecpList,
											 &GUID_ECP_NXRMFLT_BLOCKING,
											 NULL,
											 NULL);

		if (NT_SUCCESS(status))
		{
			return TRUE;
		}
	}

	return FALSE;
}

BOOLEAN is_explorer(void)
{
	BOOLEAN bRet = FALSE;

	CHAR *p = NULL;

	do 
	{
		p = PsGetProcessImageFileName(PsGetCurrentProcess());

		if(p)
		{
			if(_stricmp(p,"explorer.exe") == 0)
			{
				bRet = TRUE;
			}
		}

	} while (FALSE);

	return bRet;
}

static BOOLEAN CompareMupDeviceNames(PUNICODE_STRING FileName, PUNICODE_STRING OpeningMupName)
{
	BOOLEAN bRet = FALSE;
	
	UNICODE_STRING FileNameWithoutDevice = {0};

	UNICODE_STRING DevicePrefix = {0};

	UNICODE_STRING OpeningMupNameWithoutDeviceMup = { 0 };

	do 
	{
		DevicePrefix.Buffer			= NXRMFLT_DEVICE_PREFIX;
		DevicePrefix.Length			= sizeof(NXRMFLT_DEVICE_PREFIX) - sizeof(WCHAR);
		DevicePrefix.MaximumLength	= sizeof(NXRMFLT_DEVICE_PREFIX) - sizeof(WCHAR);

		if (RtlPrefixUnicodeString(&DevicePrefix, FileName, TRUE))
		{
			FileNameWithoutDevice.Buffer		= FileName->Buffer + (sizeof(NXRMFLT_DEVICE_PREFIX) - sizeof(WCHAR))/sizeof(WCHAR);
			FileNameWithoutDevice.Length		= FileName->Length - (sizeof(NXRMFLT_DEVICE_PREFIX) - sizeof(WCHAR));
			FileNameWithoutDevice.MaximumLength = FileName->MaximumLength - (sizeof(NXRMFLT_DEVICE_PREFIX) - sizeof(WCHAR));

			OpeningMupNameWithoutDeviceMup.Buffer			= OpeningMupName->Buffer + (sizeof(NXRMFLT_MUP_PREFIX) - sizeof(WCHAR)) / sizeof(WCHAR);
			OpeningMupNameWithoutDeviceMup.Length			= OpeningMupName->Length - (sizeof(NXRMFLT_MUP_PREFIX) - sizeof(WCHAR));
			OpeningMupNameWithoutDeviceMup.MaximumLength	= OpeningMupName->MaximumLength - (sizeof(NXRMFLT_MUP_PREFIX) - sizeof(WCHAR));

			bRet = NkEndsWithUnicodeString(OpeningMupName, &FileNameWithoutDevice, TRUE);

			if (!bRet)
			{
				bRet = NkEndsWithUnicodeString(FileName, &OpeningMupNameWithoutDeviceMup, TRUE);
			}
		}
		else
		{
			bRet = NkEndsWithUnicodeString(OpeningMupName, FileName, TRUE);

		}

	} while (FALSE);

	return bRet;
}

BOOLEAN is_app_in_real_name_access_list(PEPROCESS  Process)
{
	BOOLEAN bRet = FALSE;

	CHAR *p = NULL;

	do 
	{
		p = PsGetProcessImageFileName(Process);

		if (p)
		{
			if (_stricmp(p,"iexplore.exe") == 0)
			{
				bRet = TRUE;
				break;
			}

			if (_stricmp(p, "chrome.exe") == 0)
			{
				bRet = TRUE;
				break;
			}

			if (_stricmp(p, "firefox.exe") == 0)
			{
				bRet = TRUE;
				break;
			}

			if (_stricmp(p, "System") == 0)
			{
				bRet = TRUE;
				break;
			}

			if (_stricmp(p, "outlook.exe") == 0)
			{
				bRet = TRUE;
				break;
			}

			if (_stricmp(p, "saplogon.exe") == 0)
			{
				bRet = TRUE;
				break;
			}

			if (_stricmp(p, "sapguiserver.e") == 0)
			{
				bRet = TRUE;
				break;
			}

			if (_stricmp(p, "MicrosoftEdgeC") == 0)
			{
				bRet = TRUE;
				break;
			}

			if (_stricmp(p, "PickerHost.exe") == 0)
			{
				bRet = TRUE;
				break;
			}

			if (_stricmp(p, "OneDrive.exe") == 0) {
				bRet = TRUE;
				break;
			}

			if (_stricmp(p, "GoodSync.exe") == 0){
				bRet = TRUE;
				break;
			}

			if (_stricmp(p, "googledrivesyn") == 0) {
				bRet = TRUE;
				break;
			}

			if (_stricmp(p, "javaw.exe") == 0)
			{
				bRet = TRUE;
				break;
			}
		}

		if (is_process_a_service(Process))
		{
			bRet = TRUE;
			break;
		}

	} while (FALSE);

	return bRet;
}

BOOLEAN is_app_in_graphic_integration_list(PEPROCESS  Process)
{
	BOOLEAN bRet = FALSE;

	CHAR *p = NULL;

	do
	{
		p = PsGetProcessImageFileName(Process);

		if (p)
		{
			if (_stricmp(p, "winword.exe") == 0)
			{
				bRet = TRUE;
				break;
			}

			if (_stricmp(p, "excel.exe") == 0)
			{
				bRet = TRUE;
				break;
			}

			if (_stricmp(p, "powerpnt.exe") == 0)
			{
				bRet = TRUE;
				break;
			}

			if (_stricmp(p, "acrord32.exe") == 0)
			{
				bRet = TRUE;
				break;
			}

			//if (_stricmp(p, "veviewer.exe") == 0)
			//{
			//	bRet = TRUE;
			//	break;
			//}

			//if (_stricmp(p, "visview.exe") == 0)
			//{
			//	bRet = TRUE;
			//	break;
			//}
		}

	} while (FALSE);

	return bRet;
}



NTSTATUS get_file_id_and_attribute_ex(PFLT_INSTANCE	Instance, FILE_OBJECT *FileObject, LARGE_INTEGER *Id, ULONG *FileAttributes)
{
	NTSTATUS Status = STATUS_SUCCESS;

	IO_STATUS_BLOCK IoStatus = { 0 };

	FILE_BASIC_INFORMATION BasicInfo = { 0 };
	FILE_INTERNAL_INFORMATION IdInfo = { 0 };

	do
	{
		Status = FltQueryInformationFile(Instance,
										 FileObject,
										 &BasicInfo,
										 sizeof(BasicInfo),
										 FileBasicInformation,
										 NULL);

		if (!NT_SUCCESS(Status))
		{
			break;
		}

		Status = FltQueryInformationFile(Instance,
										 FileObject,
										 &IdInfo,
										 sizeof(IdInfo),
										 FileInternalInformation,
										 NULL);

		if (!NT_SUCCESS(Status))
		{
			break;
		}

		Id->QuadPart = IdInfo.IndexNumber.QuadPart;

		*FileAttributes = BasicInfo.FileAttributes;


	} while (FALSE);

	return Status;
}
