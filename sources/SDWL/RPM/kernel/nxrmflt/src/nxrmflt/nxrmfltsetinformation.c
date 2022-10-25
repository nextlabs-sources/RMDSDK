#include "nxrmfltdef.h"
#include "nxrmfltnxlcachemgr.h"
#include "nxrmfltsetinformation.h"
#include "nxrmfltutils.h"

extern DECLSPEC_CACHEALIGN PFLT_FILTER			gFilterHandle;
extern DECLSPEC_CACHEALIGN ULONG				gTraceFlags;
extern DECLSPEC_CACHEALIGN NXRMFLT_GLOBAL_DATA	Global;

static NTSTATUS build_nxlcache_file_name(NXL_CACHE_NODE *pNode, UNICODE_STRING *ParentDirName, UNICODE_STRING *FinalComponent);
static NTSTATUS build_nxlcache_file_name_from_name_with_nxl_extension(NXL_CACHE_NODE *pNode, UNICODE_STRING *ParentDirName, UNICODE_STRING *FinalComponent);
static NTSTATUS build_nxlcache_file_name_from_name_without_nxl_extension(NXL_CACHE_NODE *pNode, UNICODE_STRING *ParentDirName, UNICODE_STRING *FinalComponent);
static NTSTATUS build_nxlcache_reparse_name(NXL_CACHE_NODE *pNode, UNICODE_STRING *NewReparseName);

static NTSTATUS build_nxlcache_file_name_ex(NXL_CACHE_NODE *pNode, UNICODE_STRING *FullPathFileName);
static NTSTATUS build_nxlcache_file_name_from_name_with_nxl_extension_ex(NXL_CACHE_NODE *pNode, UNICODE_STRING *FullPathFileName);
static NTSTATUS build_nxlcache_file_name_from_name_without_nxl_extension_ex(NXL_CACHE_NODE *pNode, UNICODE_STRING *FullPathFileName);
static NTSTATUS build_nxlcache_source_file_name(NXL_CACHE_NODE *pNode, UNICODE_STRING *SourceFileName);

extern BOOLEAN IsNXLFile(PUNICODE_STRING Extension);
extern BOOLEAN is_explorer(void);
extern NTSTATUS get_file_id_and_attribute(PFLT_INSTANCE	Instance, UNICODE_STRING *FileName, LARGE_INTEGER *Id, ULONG *FileAttributes);
extern LPSTR PsGetProcessImageFileName(PEPROCESS  Process);

FLT_PREOP_CALLBACK_STATUS
nxrmfltPreSetInformation(
_Inout_ PFLT_CALLBACK_DATA				Data,
_In_ PCFLT_RELATED_OBJECTS				FltObjects,
_Flt_CompletionContext_Outptr_ PVOID	*CompletionContext
)
{
	FLT_PREOP_CALLBACK_STATUS	CallbackStatus = FLT_PREOP_SUCCESS_NO_CALLBACK;

	NTSTATUS Status = STATUS_SUCCESS;

	PFLT_INSTANCE			FltInstance = NULL;
	PFILE_OBJECT			FileObject = NULL;

	FILE_INFORMATION_CLASS FileInformationClass = 0;

	PFLT_FILE_NAME_INFORMATION	NameInfo = NULL;
	PFILE_RENAME_INFORMATION	RenameInfo = NULL;

	PFLT_FILE_NAME_INFORMATION	DeleteFileInfo = NULL;

	PNXRMFLT_STREAMHANDLE_CONTEXT Ccb = NULL;
	NXL_CACHE_NODE	*pCacheNode = NULL;
	
	BOOLEAN SourceFileIsNxlFile = FALSE;

	NXRMFLT_STREAM_CONTEXT	*Ctx = NULL;

	NXRMFLT_SETINFORMATION_CONTEXT	*SetFileInfoCtx = NULL;

	NXRMFLT_INSTANCE_CONTEXT	*InstCtx = NULL;

	BOOLEAN bSrcIsInSafeDir = FALSE;
	BOOLEAN bDesIsInSafeDir = FALSE;

	FltInstance = FltObjects->Instance;
	FileObject = FltObjects->FileObject;
	
	FileInformationClass = Data->Iopb->Parameters.SetFileInformation.FileInformationClass;
	
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

		if (FileInformationClass != FileAllocationInformation &&
			FileInformationClass != FileBasicInformation &&
			FileInformationClass != FileEndOfFileInformation &&
			FileInformationClass != FileRenameInformation &&
            FileInformationClass != FileRenameInformationEx &&
			FileInformationClass != FileValidDataLengthInformation)
		{
			//
			// Disallow delete operation on RPM dir
			//
			if (FileInformationClass == FileDispositionInformation)
			{
				if (!((PFILE_DISPOSITION_INFORMATION)Data->Iopb->Parameters.SetFileInformation.InfoBuffer)->DeleteFile)
				{
					break;
				}

				FILE_BASIC_INFORMATION BasicInfo;
				NTSTATUS Status2 = FltQueryInformationFile(FltInstance, FileObject, &BasicInfo, sizeof BasicInfo, FileBasicInformation, NULL);
				if (!NT_SUCCESS(Status2))
				{
					break;
				}

				Status2 = FltGetFileNameInformation(Data, FLT_FILE_NAME_NORMALIZED | FLT_FILE_NAME_QUERY_DEFAULT, &DeleteFileInfo);
				if (!NT_SUCCESS(Status2))
				{
					break;
				}

				Status2 = FltParseFileNameInformation(DeleteFileInfo);
				if (!NT_SUCCESS(Status2))
				{
					break;
				}

				if (DeleteFileInfo->Name.Length > DeleteFileInfo->Volume.Length)
				{
					UNICODE_STRING SrcPathPart = {
						DeleteFileInfo->Name.Length - DeleteFileInfo->Volume.Length,
						DeleteFileInfo->Name.Length - DeleteFileInfo->Volume.Length,
						DeleteFileInfo->Name.Buffer + DeleteFileInfo->Volume.Length / 2
					};
						
					if (!(BasicInfo.FileAttributes & FILE_ATTRIBUTE_DIRECTORY))
					{
						//when try to delete a nxl file in rpm folder, if the plain file is opened, deny
						if (nxrmfltEndWithDotNXL(&SrcPathPart) && nxrmfltInsideSafeDir(FltInstance, &SrcPathPart, FALSE, NULL, NULL))
						{
							UNICODE_STRING FileNameWithoutNXLExtension = { 0 };
							FileNameWithoutNXLExtension.Buffer = DeleteFileInfo->Name.Buffer;
							FileNameWithoutNXLExtension.MaximumLength = DeleteFileInfo->Name.MaximumLength - sizeof(NXRMFLT_NXL_DOTEXT) + sizeof(WCHAR);
							FileNameWithoutNXLExtension.Length = DeleteFileInfo->Name.Length - sizeof(NXRMFLT_NXL_DOTEXT) + sizeof(WCHAR);

							if (nxrmfltIsFileOpened(FltObjects->Instance, &FileNameWithoutNXLExtension, TRUE, NULL))
							{
								Status = STATUS_ACCESS_DENIED;
								Data->IoStatus.Status = Status;
								Data->IoStatus.Information = 0;
								CallbackStatus = FLT_PREOP_COMPLETE;
							}
						}
					}
					else
					{
						if (nxrmfltIsSafeDir(FltInstance, &SrcPathPart)
#ifdef  NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR
							|| nxrmfltIsSanctuaryDir(FltInstance, &SrcPathPart)
#endif  // NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR
							)
						{
							// We return STATUS_ACCESS_DENIED here in order to
							// deny deleting.  A more appropriate NTSTATUS code
							// would seem to be STATUS_NOT_SUPPORTED.  However,
							// that code causes Explorer in Win7 to hang for some
							// reason. So we can't use that code.
							Status = STATUS_ACCESS_DENIED;
							Data->IoStatus.Status = Status;
							Data->IoStatus.Information = 0;
							CallbackStatus = FLT_PREOP_COMPLETE;
						}
					}
				}
			}

			break;
		}
		
		if(FileInformationClass == FileRenameInformation || FileInformationClass == FileRenameInformationEx)
		{
			PT_DBG_PRINT(PTDBG_TRACE_RENAME, ("nxrmflt!nxrmfltPreSetInformation: Received a rename request!\n"));

			if (is_adobe_like_process())
			{
				break;
			}

			Status = FltAllocateContext(Global.Filter,
										FLT_STREAMHANDLE_CONTEXT,
										sizeof(NXRMFLT_STREAMHANDLE_CONTEXT),
										NonPagedPool,
										(PFLT_CONTEXT*)&Ccb);

			if (!NT_SUCCESS(Status))
			{
				PT_DBG_PRINT(PTDBG_TRACE_CRITICAL, ("nxrmflt!nxrmfltPreSetInformation: Failed to allocate stream handle ctx!\n"));
				
				break;
			}

			Ccb->DestinationFileNameInfo	= NULL;
			Ccb->SourceFileNameInfo			= NULL;
			Ccb->SourceFileIsNxlFile		= FALSE;
			Ccb->EncryptDestinationFile		= FALSE;
			Ccb->DeleteOnClose				= FALSE;
			Ccb->Reserved					= 0;

			Status =  FltGetFileNameInformation(Data,
												FLT_FILE_NAME_NORMALIZED | FLT_FILE_NAME_QUERY_DEFAULT,
												&Ccb->SourceFileNameInfo);

			if (!NT_SUCCESS(Status))
			{
				PT_DBG_PRINT(PTDBG_TRACE_CRITICAL, ("nxrmflt!nxrmfltPreSetInformation: Failed to query file name information!\n"));

				break;
			}

			Status = FltParseFileNameInformation(Ccb->SourceFileNameInfo);

			if (!NT_SUCCESS(Status))
			{
				PT_DBG_PRINT(PTDBG_TRACE_CRITICAL, ("nxrmflt!nxrmfltPreSetInformation: Failed to parse name information!\n"));

				break;
			}

			FltAcquirePushLockShared(&Global.NxlFileCacheLock);

			pCacheNode = FindNXLNodeInCache(&Global.NxlFileCache, &Ccb->SourceFileNameInfo->Name);

			if(pCacheNode)
			{
				Ccb->SourceFileIsNxlFile = SourceFileIsNxlFile = TRUE;
			}
			else
			{
				Ccb->SourceFileIsNxlFile = SourceFileIsNxlFile = FALSE;
			}

			FltReleasePushLock(&Global.NxlFileCacheLock);

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
				PT_DBG_PRINT(PTDBG_TRACE_CRITICAL, ("nxrmflt!nxrmfltPreSetInformation: FltSetStreamHandleContext return %x\n", Status));
			}

			RenameInfo = (PFILE_RENAME_INFORMATION)Data->Iopb->Parameters.SetFileInformation.InfoBuffer;
	
			//
			// We can use FltInstance instead of OriginalInstance store in Ctx because Rename has to happen on the same Volume
			// otherwise it's a copy and delete operation
			//
			Status = FltGetDestinationFileNameInformation(FltInstance,
														  FileObject,
														  RenameInfo->RootDirectory,
														  RenameInfo->FileName,
														  RenameInfo->FileNameLength,
														  FLT_FILE_NAME_NORMALIZED | FLT_FILE_NAME_QUERY_DEFAULT,
														  &NameInfo);

			if(!NT_SUCCESS(Status))
			{
				PT_DBG_PRINT(PTDBG_TRACE_CRITICAL, ("nxrmflt!nxrmfltPreSetInformation: FltGetDestinationFileNameInformation return %x\n", Status));

				break;
			}

			NTSTATUS Status2;
			FILE_BASIC_INFORMATION BasicInfo;
			Status2 = FltQueryInformationFile(FltInstance, FileObject, &BasicInfo, sizeof BasicInfo, FileBasicInformation, NULL);

			if (NT_SUCCESS(Status2))
			{
				//
				// Disallow move or rename operation for the following:
				// - If source is safe dir, deny
				// - If source is ancestor of safe dir, deny
				// - If source is descendant of safe dir, if destination is non-RPM, deny, but destination is the recycle bin folder, allow, if destination is also RPM, allow,
				//
				// Apply similar restrictions to Sanctuary dir.
				//
				if (BasicInfo.FileAttributes & FILE_ATTRIBUTE_DIRECTORY)
				{
					if (Ccb->SourceFileNameInfo->Name.Length > Ccb->SourceFileNameInfo->Volume.Length &&
						NameInfo->Name.Length > NameInfo->Volume.Length)
					{
						BOOLEAN bDeny = FALSE;

						UNICODE_STRING SrcPathPart = {
							Ccb->SourceFileNameInfo->Name.Length - Ccb->SourceFileNameInfo->Volume.Length,
							Ccb->SourceFileNameInfo->Name.Length - Ccb->SourceFileNameInfo->Volume.Length,
							Ccb->SourceFileNameInfo->Name.Buffer + Ccb->SourceFileNameInfo->Volume.Length / 2
						};
						ULONG SrcRPMRelation = nxrmfltGetSafeDirRelation(FltInstance, &SrcPathPart);
#ifdef  NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR
						ULONG SrcSanctuaryRelation = nxrmfltGetSanctuaryDirRelation(FltInstance, &SrcPathPart);
#endif  // NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR
						if ((SrcRPMRelation & (NXRMFLT_SAFEDIRRELATION_SAFE_DIR | NXRMFLT_SAFEDIRRELATION_ANCESTOR_OF_SAFE_DIR))
#ifdef  NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR
							|| (SrcSanctuaryRelation & (NXRMFLT_SANCTUARYDIRRELATION_SANCTUARY_DIR | NXRMFLT_SANCTUARYDIRRELATION_ANCESTOR_OF_SANCTUARY_DIR))
#endif  // NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR
							)
						{
							bDeny = TRUE;
						}
						else if ((SrcRPMRelation & NXRMFLT_SAFEDIRRELATION_DESCENDANT_OF_SAFE_DIR)
#ifdef  NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR
								 || (SrcSanctuaryRelation & NXRMFLT_SANCTUARYDIRRELATION_DESCENDANT_OF_SANCTUARY_DIR)
#endif  // NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR
								 )
						{
							UNICODE_STRING DestPathPart = {
								NameInfo->Name.Length - NameInfo->Volume.Length,
								NameInfo->Name.Length - NameInfo->Volume.Length,
								NameInfo->Name.Buffer + NameInfo->Volume.Length / 2
							};

							//
							BOOLEAN bIsInRecycleBin = FALSE;

							UNICODE_STRING RecycleBin = { 0 };
							RtlInitUnicodeString(&RecycleBin, L"\\$Recycle.Bin\\");
							if (DestPathPart.Length > RecycleBin.Length)
							{
								if (L'\\' == DestPathPart.Buffer[RecycleBin.Length / 2 - 1])
								{
									UNICODE_STRING TmpPath = { 0 };
									TmpPath.Buffer = DestPathPart.Buffer;
									TmpPath.Length = RecycleBin.Length;
									TmpPath.MaximumLength = RecycleBin.Length;

									if (0 == RtlCompareUnicodeString(&TmpPath, &RecycleBin, TRUE))
									{
										bIsInRecycleBin = TRUE;
									}
								}
							}

							if (!bIsInRecycleBin &&
								(((SrcRPMRelation & NXRMFLT_SAFEDIRRELATION_DESCENDANT_OF_SAFE_DIR) && !nxrmfltInsideSafeDir(FltInstance, &DestPathPart, FALSE, NULL, NULL))
#ifdef  NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR
								 || ((SrcSanctuaryRelation & NXRMFLT_SANCTUARYDIRRELATION_DESCENDANT_OF_SANCTUARY_DIR) && !nxrmfltInsideSanctuaryDir(FltInstance, &DestPathPart, FALSE))
#endif  // NEXTLABS_FEATURE_SKYDRM_SANCTUARY_DIR
								 ))
							{
								bDeny = TRUE;
							}
							else
							{
								if (SrcRPMRelation & NXRMFLT_SAFEDIRRELATION_DESCENDANT_OF_SAFE_DIR)
								{
									WCHAR* FullPath = ExAllocateFromPagedLookasideList(&Global.FullPathLookaside);
									if (FullPath)
									{
										UNICODE_STRING renamingPath = { 0, NXRMFLT_FULLPATH_BUFFER_SIZE, FullPath };
										memset(FullPath, 0, NXRMFLT_FULLPATH_BUFFER_SIZE);
										RtlUnicodeStringCat(&renamingPath, &Ccb->SourceFileNameInfo->Name);
										RtlUnicodeStringCatString(&renamingPath, L"\\");

										rb_node *ite = NULL;
										rb_node *tmp = NULL;

										FltAcquirePushLockExclusive(&Global.NxlFileCacheLock);

										RB_EACH_NODE_SAFE(ite, tmp, &Global.NxlFileCache)
										{
											NXL_CACHE_NODE *pNode = CONTAINING_RECORD(ite, NXL_CACHE_NODE, Node);

											if (pNode->Instance == FltInstance && pNode->FileName.Length > renamingPath.Length)
											{
												if (L'\\' == pNode->FileName.Buffer[renamingPath.Length / 2 - 1])
												{
													UNICODE_STRING TmpPath = { 0 };
													TmpPath.Buffer = pNode->FileName.Buffer;
													TmpPath.Length = renamingPath.Length;
													TmpPath.MaximumLength = renamingPath.Length;

													if (0 == RtlCompareUnicodeString(&TmpPath, &renamingPath, TRUE))
													{
														nxrmfltDeleteFileByName(pNode->Instance, &pNode->FileName);
														DeleteNXLNodeInCache(&Global.NxlFileCache, pNode);
														nxrmfltPurgeRightsCache(pNode->Instance, pNode->FileNameHash);
														FreeNXLCacheNode(pNode);
													}
												}
											}
										}

										FltReleasePushLock(&Global.NxlFileCacheLock);

										ExFreeToPagedLookasideList(&Global.FullPathLookaside, FullPath);
									}
								}
							}
						}

						if (bDeny)
						{
							// We return STATUS_ACCESS_DENIED here in order to
							// deny renaming.  A more appropriate NTSTATUS code
							// would seem to be STATUS_NOT_SUPPORTED.  However,
							// that code causes Explorer in Win7 to hang for some
							// reason. So we can't use that code.
							Status = STATUS_ACCESS_DENIED;
							Data->IoStatus.Status = Status;
							Data->IoStatus.Information = 0;
							CallbackStatus = FLT_PREOP_COMPLETE;
							break;
						}
					}
				}
				else
				{
					//
					// For move operation of file:
					// - If destination is not safe dir, allow
					// - If destination is safe dir, if file name contains .nxl except end .nxl, deny(1.nxl.txt.nxl deny 1.txt.nxl allow)
					// - If destination is safe dir and file name is ok, if there will be conflict of file name, deny(file name is 1.txt.nxl, 1.txt exists, deny, file name is 1.txt, 1.txt.nxl exists, deny)
					if (Ccb->SourceFileNameInfo->Name.Length > Ccb->SourceFileNameInfo->Volume.Length &&
						NameInfo->Name.Length > NameInfo->Volume.Length)
					{
						UNICODE_STRING DestPathPart = {
							NameInfo->Name.Length - NameInfo->Volume.Length,
							NameInfo->Name.Length - NameInfo->Volume.Length,
							NameInfo->Name.Buffer + NameInfo->Volume.Length / 2
						};

						UNICODE_STRING SrcPathPart = {
							Ccb->SourceFileNameInfo->Name.Length - Ccb->SourceFileNameInfo->Volume.Length,
							Ccb->SourceFileNameInfo->Name.Length - Ccb->SourceFileNameInfo->Volume.Length,
							Ccb->SourceFileNameInfo->Name.Buffer + Ccb->SourceFileNameInfo->Volume.Length / 2
						};
						BOOLEAN bSrcIsInSafeDir = nxrmfltInsideSafeDir(FltInstance, &SrcPathPart, FALSE, NULL, NULL);
						BOOLEAN bSrcEndWithDotNXL = nxrmfltEndWithDotNXL(&SrcPathPart);

						if (!nxrmfltInsideSafeDir(FltInstance, &DestPathPart, FALSE, NULL, NULL))
						{
							bDesIsInSafeDir = FALSE;

							if (!bSrcIsInSafeDir)
							{
								break;
							}
						}
						else
						{
							bDesIsInSafeDir = TRUE;

							int iCount = 0;
							BOOLEAN bContainColon = FALSE;
							nxrmfltGetDotNXLCount(&DestPathPart, &iCount, &bContainColon);
							BOOLEAN bDestEndWithDotNXL = nxrmfltEndWithDotNXL(&DestPathPart);

							if (!bContainColon)
							{
								if (bSrcIsInSafeDir)
								{
									if (bSrcEndWithDotNXL && 0 == nxrmfltCompareParentComponent(&SrcPathPart, &DestPathPart, TRUE))
									{
										if (iCount >= 1)
										{
											Status = STATUS_ACCESS_DENIED;
											Data->IoStatus.Status = Status;
											Data->IoStatus.Information = 0;
											CallbackStatus = FLT_PREOP_COMPLETE;
											break;
										}
										else
										{
											//If destination file with nxl exists, and the destination file exists, if ReplaceIfExists is false, return name collision, else try to delelte it, if fail, return denied;
											//If destination file with nxl doesn't exist, and the destination file exists, return denied;
											//If destination file with nxl exists, and the destination file doesn't exist, if ReplaceIfExists is false, return name collision.
											//		if destination file with nxl is the same as source file with nxl, if ReplaceIfExists is false, ok to continue
											BOOLEAN bDstFileAppendNXLExtension = FALSE;

											UNICODE_STRING DstFileNameAppendNXLExtension = { 0 };
											DstFileNameAppendNXLExtension.Buffer = ExAllocateFromPagedLookasideList(&Global.FullPathLookaside);
											if (DstFileNameAppendNXLExtension.Buffer)
											{
												DstFileNameAppendNXLExtension.MaximumLength = NXRMFLT_FULLPATH_BUFFER_SIZE;
												DstFileNameAppendNXLExtension.Length = 0;
												RtlUnicodeStringCat(&DstFileNameAppendNXLExtension, &NameInfo->Name);
												RtlUnicodeStringCatString(&DstFileNameAppendNXLExtension, NXRMFLT_NXL_DOTEXT);

												bDstFileAppendNXLExtension = nxrmfltDoesFileExist(FltInstance, &DstFileNameAppendNXLExtension, TRUE);

												ExFreeToPagedLookasideList(&Global.FullPathLookaside, DstFileNameAppendNXLExtension.Buffer);
											}

											if (nxrmfltDoesFileExist(FltInstance, &DestPathPart, TRUE))
											{
												if (bDstFileAppendNXLExtension)
												{
													if (!RenameInfo->ReplaceIfExists)
													{
														Status = STATUS_OBJECT_NAME_COLLISION;
														Data->IoStatus.Status = Status;
														Data->IoStatus.Information = 0;
														CallbackStatus = FLT_PREOP_COMPLETE;
														break;
													}
													else
													{
														if (NT_SUCCESS(nxrmfltDeleteFileByNameSync(FltObjects->Instance, &NameInfo->Name)))
														{
															FltAcquirePushLockExclusive(&Global.NxlFileCacheLock);

															NXL_CACHE_NODE *pTmpNode = FindNXLNodeInCache(&Global.NxlFileCache, &NameInfo->Name);
															if (pTmpNode)
															{
																ClearFlag(pTmpNode->Flags, NXRMFLT_FLAG_CTX_ATTACHED);
															}

															FltReleasePushLock(&Global.NxlFileCacheLock);
														}
														else
														{
															Status = STATUS_ACCESS_DENIED;
															Data->IoStatus.Status = Status;
															Data->IoStatus.Information = 0;
															CallbackStatus = FLT_PREOP_COMPLETE;
															break;
														}

													}
												}
												else
												{
													Status = STATUS_ACCESS_DENIED;
													Data->IoStatus.Status = Status;
													Data->IoStatus.Information = 0;
													CallbackStatus = FLT_PREOP_COMPLETE;
													break;
												}
											}
											else
											{
												if (bDstFileAppendNXLExtension)
												{
													if (!RenameInfo->ReplaceIfExists)
													{
														LONG SameFileOfSrcDest = 1;
														UNICODE_STRING DstPathPartWithNXL = { 0 };
														WCHAR *FullPathName = NULL;
														FullPathName = ExAllocateFromPagedLookasideList(&Global.FullPathLookaside);
														if (FullPathName)
														{
															memset(FullPathName, 0, NXRMFLT_FULLPATH_BUFFER_SIZE);

															DstPathPartWithNXL.Buffer = FullPathName;
															DstPathPartWithNXL.Length = 0;
															DstPathPartWithNXL.MaximumLength = NXRMFLT_FULLPATH_BUFFER_SIZE;

															RtlCopyUnicodeString(&DstPathPartWithNXL, &DestPathPart);
															RtlUnicodeStringCatString(&DstPathPartWithNXL, NXRMFLT_NXL_DOTEXT);

															SameFileOfSrcDest = RtlCompareUnicodeString(&SrcPathPart, &DstPathPartWithNXL, TRUE);

															ExFreeToPagedLookasideList(&Global.FullPathLookaside, FullPathName);
															FullPathName = NULL;

														}

														if (SameFileOfSrcDest == 0)
														{
															// source is with .nxl, dest is without .nxl, but same file name, and dest.nxl exists
															// then they are same file, do nothing and return successfully directly
															Status = STATUS_SUCCESS;
															Data->IoStatus.Status = Status;
															Data->IoStatus.Information = 0;
															CallbackStatus = FLT_PREOP_COMPLETE;
															break;
														}
														else
														{
															Status = STATUS_OBJECT_NAME_COLLISION;
															Data->IoStatus.Status = Status;
															Data->IoStatus.Information = 0;
															CallbackStatus = FLT_PREOP_COMPLETE;
															break;
														}
													}
												}
											}
										}
									}
									else
									{
										if (iCount > 1)
										{
											Status = STATUS_ACCESS_DENIED;
											Data->IoStatus.Status = Status;
											Data->IoStatus.Information = 0;
											CallbackStatus = FLT_PREOP_COMPLETE;
											break;
										}
										else if (iCount == 1)
										{
											if (!bDestEndWithDotNXL)
											{
												Status = STATUS_ACCESS_DENIED;
												Data->IoStatus.Status = Status;
												Data->IoStatus.Information = 0;
												CallbackStatus = FLT_PREOP_COMPLETE;
												break;
											}
											else
											{
												UNICODE_STRING DstFileNameWithoutNXLExtension = { 0 };
												DstFileNameWithoutNXLExtension.Buffer = NameInfo->Name.Buffer;
												DstFileNameWithoutNXLExtension.MaximumLength = NameInfo->Name.MaximumLength - sizeof(NXRMFLT_NXL_DOTEXT) + sizeof(WCHAR);
												DstFileNameWithoutNXLExtension.Length = NameInfo->Name.Length - sizeof(NXRMFLT_NXL_DOTEXT) + sizeof(WCHAR);

												if (0 != RtlCompareUnicodeString(&Ccb->SourceFileNameInfo->Name, &DstFileNameWithoutNXLExtension, FALSE)
													&& nxrmfltDoesFileExist(FltInstance, &DstFileNameWithoutNXLExtension, TRUE))
												{
													if (nxrmfltDoesFileExist(FltInstance, &NameInfo->Name, TRUE))
													{
														if (RenameInfo->ReplaceIfExists)
														{
                                                            if (nxrmfltIsFileOpened(FltObjects->Instance, &DstFileNameWithoutNXLExtension, TRUE, NULL) && !is_msoffice_process())
                                                            {
                                                                Status = STATUS_ACCESS_DENIED;
                                                                Data->IoStatus.Status = Status;
                                                                Data->IoStatus.Information = 0;
                                                                CallbackStatus = FLT_PREOP_COMPLETE;

                                                                break;
                                                            }

															if (NT_SUCCESS(nxrmfltDeleteFileByNameSync(FltObjects->Instance, &DstFileNameWithoutNXLExtension)))
															{
																FltAcquirePushLockExclusive(&Global.NxlFileCacheLock);

																NXL_CACHE_NODE *pTmpNode = FindNXLNodeInCache(&Global.NxlFileCache, &DstFileNameWithoutNXLExtension);
																if (pTmpNode)
																{
																	ClearFlag(pTmpNode->Flags, NXRMFLT_FLAG_CTX_ATTACHED);
																}

																FltReleasePushLock(&Global.NxlFileCacheLock);
															}
															else
															{
																Status = STATUS_ACCESS_DENIED;
																Data->IoStatus.Status = Status;
																Data->IoStatus.Information = 0;
																CallbackStatus = FLT_PREOP_COMPLETE;
																break;
															}
														}
													}
													else
													{
														Status = STATUS_ACCESS_DENIED;
														Data->IoStatus.Status = Status;
														Data->IoStatus.Information = 0;
														CallbackStatus = FLT_PREOP_COMPLETE;
														break;
													}
												}
											}
										}
										else
										{
											UNICODE_STRING DstFileNameAppendNXLExtension = { 0 };
											DstFileNameAppendNXLExtension.Buffer = ExAllocateFromPagedLookasideList(&Global.FullPathLookaside);
											if (DstFileNameAppendNXLExtension.Buffer)
											{
												DstFileNameAppendNXLExtension.MaximumLength = NXRMFLT_FULLPATH_BUFFER_SIZE;
												DstFileNameAppendNXLExtension.Length = 0;
												RtlUnicodeStringCat(&DstFileNameAppendNXLExtension, &NameInfo->Name);
												RtlUnicodeStringCatString(&DstFileNameAppendNXLExtension, NXRMFLT_NXL_DOTEXT);

												BOOLEAN bExist = nxrmfltDoesFileExist(FltInstance, &DstFileNameAppendNXLExtension, TRUE);

												ExFreeToPagedLookasideList(&Global.FullPathLookaside, DstFileNameAppendNXLExtension.Buffer);

												if (bExist)
												{
													// 
													// both SrcPathPart and DestPathPart are not with '.nxl'
													// if they are the same file, not block it
													//
													LONG SameFileOfSrcDest = RtlCompareUnicodeString(&SrcPathPart, &DestPathPart, TRUE);
													if (SameFileOfSrcDest != 0)
													{
														Status = STATUS_ACCESS_DENIED;
														Data->IoStatus.Status = Status;
														Data->IoStatus.Information = 0;
														CallbackStatus = FLT_PREOP_COMPLETE;
														break;
													}
												}
											}
										}
									}
								}
								else
								{
									if (iCount > 1)
									{
										Status = STATUS_ACCESS_DENIED;
										Data->IoStatus.Status = Status;
										Data->IoStatus.Information = 0;
										CallbackStatus = FLT_PREOP_COMPLETE;
										break;
									}
									else if (iCount == 1)
									{
										if (!bDestEndWithDotNXL)
										{
											Status = STATUS_ACCESS_DENIED;
											Data->IoStatus.Status = Status;
											Data->IoStatus.Information = 0;
											CallbackStatus = FLT_PREOP_COMPLETE;
											break;
										}
										else
										{
											UNICODE_STRING DstFileNameWithoutNXLExtension = { 0 };
											DstFileNameWithoutNXLExtension.Buffer = NameInfo->Name.Buffer;
											DstFileNameWithoutNXLExtension.MaximumLength = NameInfo->Name.MaximumLength - sizeof(NXRMFLT_NXL_DOTEXT) + sizeof(WCHAR);
											DstFileNameWithoutNXLExtension.Length = NameInfo->Name.Length - sizeof(NXRMFLT_NXL_DOTEXT) + sizeof(WCHAR);

											if (nxrmfltDoesFileExist(FltInstance, &DstFileNameWithoutNXLExtension, TRUE))
											{
												if (nxrmfltDoesFileExist(FltInstance, &NameInfo->Name, TRUE))
												{
													if (RenameInfo->ReplaceIfExists)
													{
														//////////////////////////////////////////////////////////////////////////////////////////////////
														// fix bug 64902 "[MoveFileExA] NXL file still opened, can be replaced, and lost protect after do save"
														//
														// if native file is open, we shall not allow the NXL file to be replaced (here is replaced via MoveFileEx
														// previously, we call nxrmfltDeleteFileByNameSync to delete native file, and if success, we continue the Rename
														// it seems nxrmfltDeleteFileByNameSync has issue even native file is open (because of delay-delete?)
														// we need to fix nxrmfltDeleteFileByNameSync, but for this bug, we do extra check to see native file is open or not
														//
														if (nxrmfltIsFileOpened(FltObjects->Instance, &DstFileNameWithoutNXLExtension, TRUE, NULL))
														{
															Status = STATUS_ACCESS_DENIED;
															Data->IoStatus.Status = Status;
															Data->IoStatus.Information = 0;
															CallbackStatus = FLT_PREOP_COMPLETE;
															break;
														}
														//////////////////////////////////////////////////////////////////////////////////////////////////

														if (NT_SUCCESS(nxrmfltDeleteFileByNameSync(FltObjects->Instance, &DstFileNameWithoutNXLExtension)))
														{
															FltAcquirePushLockExclusive(&Global.NxlFileCacheLock);

															NXL_CACHE_NODE *pTmpNode = FindNXLNodeInCache(&Global.NxlFileCache, &DstFileNameWithoutNXLExtension);
															if (pTmpNode)
															{
																ClearFlag(pTmpNode->Flags, NXRMFLT_FLAG_CTX_ATTACHED);
															}

															FltReleasePushLock(&Global.NxlFileCacheLock);
														}
														else
														{
															Status = STATUS_ACCESS_DENIED;
															Data->IoStatus.Status = Status;
															Data->IoStatus.Information = 0;
															CallbackStatus = FLT_PREOP_COMPLETE;
															break;
														}
													}
												}
												else
												{
												Status = STATUS_ACCESS_DENIED;
												Data->IoStatus.Status = Status;
												Data->IoStatus.Information = 0;
												CallbackStatus = FLT_PREOP_COMPLETE;
												break;
											}
										}
									}
									}
									else
									{
										UNICODE_STRING DstFileNameAppendNXLExtension = { 0 };
										DstFileNameAppendNXLExtension.Buffer = ExAllocateFromPagedLookasideList(&Global.FullPathLookaside);
										if (DstFileNameAppendNXLExtension.Buffer)
										{
											DstFileNameAppendNXLExtension.MaximumLength = NXRMFLT_FULLPATH_BUFFER_SIZE;
											DstFileNameAppendNXLExtension.Length = 0;
											RtlUnicodeStringCat(&DstFileNameAppendNXLExtension, &NameInfo->Name);
											RtlUnicodeStringCatString(&DstFileNameAppendNXLExtension, NXRMFLT_NXL_DOTEXT);

											BOOLEAN bExist = nxrmfltDoesFileExist(FltInstance, &DstFileNameAppendNXLExtension, TRUE);

											ExFreeToPagedLookasideList(&Global.FullPathLookaside, DstFileNameAppendNXLExtension.Buffer);

											if (bExist)
											{
												Status = STATUS_ACCESS_DENIED;
												Data->IoStatus.Status = Status;
												Data->IoStatus.Information = 0;
												CallbackStatus = FLT_PREOP_COMPLETE;
												break;
											}
										}
									}
								}
							}
						}

						//when try to move a nxl file in rpm folder, if the plain file is opened, deny
						if (bSrcEndWithDotNXL && bSrcIsInSafeDir)
						{
							UNICODE_STRING DstFileNameWithoutNXLExtension = { 0 };
							DstFileNameWithoutNXLExtension.Buffer = Ccb->SourceFileNameInfo->Name.Buffer;
							DstFileNameWithoutNXLExtension.MaximumLength = Ccb->SourceFileNameInfo->Name.MaximumLength - sizeof(NXRMFLT_NXL_DOTEXT) + sizeof(WCHAR);
							DstFileNameWithoutNXLExtension.Length = Ccb->SourceFileNameInfo->Name.Length - sizeof(NXRMFLT_NXL_DOTEXT) + sizeof(WCHAR);

							if (nxrmfltIsFileOpened(FltObjects->Instance, &DstFileNameWithoutNXLExtension, TRUE, NULL))
							{
								Status = STATUS_ACCESS_DENIED;
								Data->IoStatus.Status = Status;
								Data->IoStatus.Information = 0;
								CallbackStatus = FLT_PREOP_COMPLETE;
								break;
							}
						}
					}
				}
			}

			//Fix bug RMDC-357 that show "Item Not Found" dialog when rename a NXL file in NON-RPMdir (rename to same name with a nature file)

			//if (RenameInfo->ReplaceIfExists == FALSE && 
			//	IsNXLFile(&Ccb->SourceFileNameInfo->Extension) &&
			//	NameInfo->Name.Length >= sizeof(NXRMFLT_NXL_DOTEXT) &&
			//	memcmp((UCHAR*)NameInfo->Name.Buffer + NameInfo->Name.Length - (sizeof(NXRMFLT_NXL_DOTEXT) - sizeof(WCHAR)), NXRMFLT_NXL_DOTEXT, sizeof(NXRMFLT_NXL_DOTEXT) - sizeof(WCHAR)) == 0)
			//{
			//	UNICODE_STRING DstFileNameWithoutNXLExtension = {0};

			//	DstFileNameWithoutNXLExtension.Buffer = NameInfo->Name.Buffer;
			//	DstFileNameWithoutNXLExtension.MaximumLength = NameInfo->Name.MaximumLength - sizeof(NXRMFLT_NXL_DOTEXT) + sizeof(WCHAR);
			//	DstFileNameWithoutNXLExtension.Length = NameInfo->Name.Length - sizeof(NXRMFLT_NXL_DOTEXT) + sizeof(WCHAR);

			//	//
			//	// return STATUS_OBJECT_NAME_COLLISION if destination file without nxl extension does exist
			//	//
			//	if (nxrmfltDoesFileExist(FltInstance, &DstFileNameWithoutNXLExtension, TRUE))
			//	{
			//		Status = STATUS_OBJECT_NAME_COLLISION;
			//		Data->IoStatus.Status		= Status;
			//		Data->IoStatus.Information	= 0;
			//		CallbackStatus = FLT_PREOP_COMPLETE;
			//		
			//		break;
			//	}
			//}

			PT_DBG_PRINT(PTDBG_TRACE_RENAME, 
						 ("nxrmflt!nxrmfltPreSetInformation: Source name is %wZ and destination file is %wZ. %wZ %s an NXL file!\n", 
						 &Ccb->SourceFileNameInfo->Name, 
						 &Ccb->DestinationFileNameInfo->Name,
						 &Ccb->SourceFileNameInfo->Name,
						 SourceFileIsNxlFile?"is":"is NOT"));
		}

        if (FileInformationClass == FileBasicInformation)
        {
            PFLT_FILE_NAME_INFORMATION		FileNameInfo = NULL;

            do
            {
                FILE_BASIC_INFORMATION BasicInformation;
                NTSTATUS RetStatus = FltQueryInformationFile(FltInstance, FileObject, &BasicInformation, sizeof(FILE_BASIC_INFORMATION), FileBasicInformation, NULL);

                if (!NT_SUCCESS(RetStatus))
                {
                    PT_DBG_PRINT(PTDBG_TRACE_CRITICAL, ("nxrmflt!nxrmfltPreSetInformation: FltQueryInformationFile Ret (%x), LINE (%d) !\n",
                        RetStatus, __LINE__));

                    break;
                }

                if (BasicInformation.FileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                {
                    //PT_DBG_PRINT(PTDBG_TRACE_CRITICAL, ("nxrmflt!nxrmfltPreSetInformation: FILE_ATTRIBUTE_DIRECTORY, LINE (%d) !\n", __LINE__));

                    break;
                }

                RetStatus = FltGetFileNameInformation(Data, FLT_FILE_NAME_NORMALIZED | FLT_FILE_NAME_QUERY_DEFAULT, &FileNameInfo);
                if (!NT_SUCCESS(RetStatus))
                {
                    PT_DBG_PRINT(PTDBG_TRACE_CRITICAL, ("nxrmflt!nxrmfltPreSetInformation: FltGetFileNameInformation Ret (%x), LINE (%d) !\n",
                        RetStatus, __LINE__));

                    break;
                }

                RetStatus = FltParseFileNameInformation(FileNameInfo);
                if (!NT_SUCCESS(RetStatus))
                {
                    PT_DBG_PRINT(PTDBG_TRACE_CRITICAL, ("nxrmflt!nxrmfltPreSetInformation: FltParseFileNameInformation Ret (%x), LINE (%d) !\n",
                        RetStatus, __LINE__));

                    break;
                }

                UNICODE_STRING SrcPathPart = {
                    FileNameInfo->Name.Length - FileNameInfo->Volume.Length,
                    FileNameInfo->Name.Length - FileNameInfo->Volume.Length,
                    FileNameInfo->Name.Buffer + FileNameInfo->Volume.Length / 2
                };

                if (!nxrmfltInsideSafeDir(FltInstance, &SrcPathPart, FALSE, NULL, NULL))
                {
                    break;
                }

                PFILE_BASIC_INFORMATION NewBasicInfo = (PFILE_BASIC_INFORMATION)Data->Iopb->Parameters.SetFileInformation.InfoBuffer;

                RetStatus = nxrmfltSyncFileAttributes(FltInstance, &FileNameInfo->Name, NewBasicInfo, TRUE);

                if (!NT_SUCCESS(RetStatus))
                {
                    PT_DBG_PRINT(PTDBG_TRACE_CRITICAL, ("nxrmflt!nxrmfltPreSetInformation: SrcPathPart(%wZ), Name(%wZ), LINE (%d), Ret (%x)!\n",
                        &SrcPathPart, &FileNameInfo->Name, __LINE__, RetStatus));

                    break;
                }

                UNICODE_STRING NxlNodeFileName = {
                    FileNameInfo->Name.Length,
                    FileNameInfo->Name.Length,
                    FileNameInfo->Name.Buffer
                };

                if (nxrmfltEndWithDotNXL(&NxlNodeFileName))
                {
                    NxlNodeFileName.Length -= 4 * sizeof(WCHAR);
                }

                NXL_CACHE_NODE* pNxlCacheNode = NULL;
                {
                    FltAcquirePushLockShared(&Global.NxlFileCacheLock);

                    pNxlCacheNode = FindNXLNodeInCache(&Global.NxlFileCache, &NxlNodeFileName);

                    if (pNxlCacheNode)
                    {
                        if (!ExAcquireRundownProtection(&pNxlCacheNode->NodeRundownRef))
                        {
                            pNxlCacheNode = NULL;
                        }
                    }

                    FltReleasePushLock(&Global.NxlFileCacheLock);
                }

                if (pNxlCacheNode)
                {
                    pNxlCacheNode->FileAttributes = NewBasicInfo->FileAttributes;

                    ExReleaseRundownProtection(&pNxlCacheNode->NodeRundownRef);

                    pNxlCacheNode = NULL;
                }

            } while (FALSE);

            if (FileNameInfo)
            {
                FltReleaseFileNameInformation(FileNameInfo);
                FileNameInfo = NULL;
            }
        }

		FltGetStreamContext(FltInstance, FileObject, &Ctx);

		if (Ctx == NULL &&
			NameInfo == NULL)
		{
			break;
		}

		SetFileInfoCtx = ExAllocateFromPagedLookasideList(&Global.SetInformationCtxLookaside);

		if (!SetFileInfoCtx)
		{
			break;
		}

		SetFileInfoCtx->Ctx				= Ctx;
		SetFileInfoCtx->NameInfo		= NameInfo;
		SetFileInfoCtx->bSrcIsInSafeDir = bSrcIsInSafeDir;
		SetFileInfoCtx->bDesIsInSafeDir = bDesIsInSafeDir;

		*CompletionContext = SetFileInfoCtx;

		//
		// set to NULL to prevent it from being freed
		//
		SetFileInfoCtx = NULL;

		//
		// set to NULL to prevent it from being freed
		//
		NameInfo = NULL;

		CallbackStatus = FLT_PREOP_SYNCHRONIZE;

	} while (FALSE);

	if(Ccb)
	{
		FltReleaseContext(Ccb);
	}

	if (SetFileInfoCtx)
	{
		ExFreeToPagedLookasideList(&Global.SetInformationCtxLookaside, SetFileInfoCtx);
		SetFileInfoCtx = NULL;
	}

	if (DeleteFileInfo)
	{
		FltReleaseFileNameInformation(DeleteFileInfo);
		DeleteFileInfo = NULL;
	}

	if (NameInfo)
	{
		FltReleaseFileNameInformation(NameInfo);
		NameInfo = NULL;
	}

	if (InstCtx)
	{
		FltReleaseContext(InstCtx);
	}

	return CallbackStatus;
}

FLT_POSTOP_CALLBACK_STATUS
	nxrmfltPostSetInformation(
	_Inout_ PFLT_CALLBACK_DATA		Data,
	_In_ PCFLT_RELATED_OBJECTS		FltObjects,
	_In_opt_ PVOID					CompletionContext,
	_In_ FLT_POST_OPERATION_FLAGS	Flags
	)
{
	FLT_POSTOP_CALLBACK_STATUS	CallbackStatus = FLT_POSTOP_FINISHED_PROCESSING;

	NTSTATUS	Status = STATUS_SUCCESS;
	ULONG_PTR	Information = 0;

	FILE_INFORMATION_CLASS FileInformationClass = 0;

	PFLT_INSTANCE			FltInstance = NULL;
	PFILE_OBJECT			FileObject = NULL;

	NXL_CACHE_NODE	*pCacheNode = NULL;

	PFILE_BASIC_INFORMATION FileBasicInfo = NULL;

	BOOLEAN	MediaEjected = FALSE;

	PNXRMFLT_SETINFORMATION_CONTEXT SetFileInfoCtx = (PNXRMFLT_SETINFORMATION_CONTEXT)CompletionContext;

	PFLT_FILE_NAME_INFORMATION	NameInfo		= SetFileInfoCtx->NameInfo;
	PNXRMFLT_STREAM_CONTEXT		Ctx				= SetFileInfoCtx->Ctx;

	PFLT_FILE_NAME_INFORMATION	TunnelNameInfo = NULL;

	BOOLEAN	ReleaseNameInfo = TRUE;

	NXL_CACHE_NODE	*pNode = NULL;

	UNICODE_STRING	OnDiskNXLFileName = { 0 };

	PNXRMFLT_STREAMHANDLE_CONTEXT Ccb = NULL;

	PNXL_RENAME_NODE pRenameNode = NULL;

	LIST_ENTRY	*ite = NULL;

	BOOLEAN AttachStreamCtxToRenamedFile = FALSE;

	BOOLEAN	SkipUpdateNxlCache = FALSE;

	FltInstance = FltObjects->Instance;
	FileObject = FltObjects->FileObject;

	do
	{
		Status = Data->IoStatus.Status;
		Information = Data->IoStatus.Information;
		FileInformationClass = Data->Iopb->Parameters.SetFileInformation.FileInformationClass;

		if (!NT_SUCCESS(Status))
		{
			if(FileInformationClass == FileRenameInformation || FileInformationClass == FileRenameInformationEx)
			{
				PT_DBG_PRINT(PTDBG_TRACE_RENAME, ("nxrmflt!nxrmfltPostSetInformation: Renaming request failed! Status is %x\n",Status));
			}
			
			break;
		}

		if(NameInfo)
		{
			Status = FltGetTunneledName(Data, NameInfo, &TunnelNameInfo);

			if (!NT_SUCCESS(Status))
			{
				PT_DBG_PRINT(PTDBG_TRACE_CRITICAL, ("nxrmflt!nxrmfltPostSetInformation: Failed to get tunneled file name information! Status is %x\n", Status));
				break;
			}

			if (TunnelNameInfo)
			{
				PT_DBG_PRINT(PTDBG_TRACE_RENAME, ("nxrmflt!nxrmfltPostSetInformation: Successfully get tunneled name information! New name is %wZ\n", &TunnelNameInfo->Name));

				FltReleaseFileNameInformation(NameInfo);

				NameInfo = TunnelNameInfo;
			}

			Status = FltParseFileNameInformation(NameInfo);

			if(!NT_SUCCESS(Status))
			{
				PT_DBG_PRINT(PTDBG_TRACE_CRITICAL, ("nxrmflt!nxrmfltPostSetInformation: Failed to parse file name information! Status is %x\n", Status));
				break;
			}
		}

		Status = FltGetStreamHandleContext(FltInstance, FileObject, &Ccb);

		if(Ccb && NameInfo && (FileInformationClass == FileRenameInformation || FileInformationClass == FileRenameInformationEx))
		{
			UNICODE_STRING SrcOnDiskNxlFileName = { 0 };
			UNICODE_STRING DstOnDiskNxlFileName = { 0 };
			UNICODE_STRING SrcOnDiskFileNameWithoutExtension = { 0 };

			Ccb->DestinationFileNameInfo = NameInfo;

			ReleaseNameInfo = FALSE;

			do 
			{
				pRenameNode = ExAllocateFromPagedLookasideList(&Global.RenameCacheLookaside);

				if(!pRenameNode)
				{
					PT_DBG_PRINT(PTDBG_TRACE_CRITICAL, ("nxrmflt!nxrmfltPostSetInformation: Failed to allocate rename node from lookaside!\n"));

					break;
				}

				memset(pRenameNode, 0, sizeof(NXL_RENAME_NODE));
				
				Status = nxrmfltBuildRenameNodeFromCcb(Ccb, pRenameNode);

				if(!NT_SUCCESS(Status))
				{
					PT_DBG_PRINT(PTDBG_TRACE_CRITICAL, ("nxrmflt!nxrmfltPostSetInformation: Failed to build rename node from Ccb! Status is %x\n",Status));

					break;
				}

				PT_DBG_PRINT(PTDBG_TRACE_RENAME, 
							 ("nxrmflt!nxrmfltPostSetInformation: Insert rename node into rename list. Source name is %wZ and destination name is %wZ\n", 
							 &pRenameNode->SourceFileName, 
							 &pRenameNode->DestinationFileName));

				FltAcquirePushLockExclusive(&Global.RenameListLock);

				InsertHeadList(&Global.RenameList, &pRenameNode->Link);

				FltReleasePushLock(&Global.RenameListLock);

				//
				// make sure pRenameNode won't get freed
				//
				pRenameNode = NULL;

				FltAcquirePushLockShared(&Global.RenameListLock);

				FOR_EACH_LIST(ite, &Global.RenameList)
				{
					NXL_RENAME_NODE *pTmpNode = CONTAINING_RECORD(ite, NXL_RENAME_NODE, Link);

					if(0 == RtlCompareUnicodeString(&Ccb->DestinationFileNameInfo->Name, &pTmpNode->SourceFileName, TRUE) &&
					   pTmpNode->SourceFileIsNxlFile)
					{
						SrcOnDiskNxlFileName.Buffer			= ExAllocateFromPagedLookasideList(&Global.FullPathLookaside);
						
						if(SrcOnDiskNxlFileName.Buffer)
						{
							SrcOnDiskNxlFileName.MaximumLength	= NXRMFLT_FULLPATH_BUFFER_SIZE;
							SrcOnDiskNxlFileName.Length			= 0;

							RtlUnicodeStringCat(&SrcOnDiskNxlFileName, &pTmpNode->DestinationFileName);
							RtlUnicodeStringCatString(&SrcOnDiskNxlFileName, NXRMFLT_NXL_DOTEXT);

							SrcOnDiskFileNameWithoutExtension.Buffer = SrcOnDiskNxlFileName.Buffer;
							SrcOnDiskFileNameWithoutExtension.MaximumLength = SrcOnDiskNxlFileName.MaximumLength;
							SrcOnDiskFileNameWithoutExtension.Length = pTmpNode->DestinationFileName.Length;
						}
						else
						{
							PT_DBG_PRINT(PTDBG_TRACE_CRITICAL, ("nxrmflt!nxrmfltPostSetInformation: Failed to allocate source file name from lookaside!\n"));

							break;
						}

						DstOnDiskNxlFileName.Buffer			= ExAllocateFromPagedLookasideList(&Global.FullPathLookaside);

						if(DstOnDiskNxlFileName.Buffer)
						{
							DstOnDiskNxlFileName.MaximumLength	= NXRMFLT_FULLPATH_BUFFER_SIZE;
							DstOnDiskNxlFileName.Length			= 0;

							RtlUnicodeStringCat(&DstOnDiskNxlFileName, &Ccb->DestinationFileNameInfo->Name);
							RtlUnicodeStringCatString(&DstOnDiskNxlFileName, NXRMFLT_NXL_DOTEXT);
						}
						else
						{
							PT_DBG_PRINT(PTDBG_TRACE_CRITICAL, ("nxrmflt!nxrmfltPostSetInformation: Failed to allocate destination file name from lookaside!\n"));

							break;
						}

						AttachStreamCtxToRenamedFile = TRUE;
						break;
					}
				}

				FltReleasePushLock(&Global.RenameListLock);

				if(AttachStreamCtxToRenamedFile)
				{
					NXL_CACHE_NODE	*pNewCacheNode = NULL;
					NXRMFLT_STREAM_CONTEXT	*NewCtx = NULL;
					BOOLEAN FreeNewCacheNode = FALSE;
					
					PT_DBG_PRINT(PTDBG_TRACE_RENAME, 
								 ("nxrmflt!nxrmfltPostSetInformation: Need to attach stream Ctx to file %wZ\n", 
								 &Ccb->DestinationFileNameInfo->Name));

					do 
					{

						Status = FltAllocateContext(Global.Filter, 
													FLT_STREAM_CONTEXT, 
													sizeof(NXRMFLT_STREAM_CONTEXT), 
													NonPagedPool, 
													(PFLT_CONTEXT*)&NewCtx);

						if (!NT_SUCCESS(Status))
						{
							PT_DBG_PRINT(PTDBG_TRACE_CRITICAL, ("nxrmflt!nxrmfltPostSetInformation: Failed to allocate stream Ctx!\n"));

							break;
						}

						//
						// Initialize Ctx
						//
						memset(NewCtx, 0, sizeof(NXRMFLT_STREAM_CONTEXT));

						//
						// make sure CtxCleanup won't free NULL point in case there is error when building this Ctx
						//
						NewCtx->ReleaseFileName = FALSE;

						InterlockedIncrement(&Global.TotalContext);

						Status = nxrmfltBuildNamesInStreamContext(NewCtx, &Ccb->DestinationFileNameInfo->Name);

						if(!NT_SUCCESS(Status))
						{
							PT_DBG_PRINT(PTDBG_TRACE_CRITICAL, ("nxrmflt!nxrmfltPostSetInformation: Failed to build names in stream Ctx! Status is %x\n", Status));

							break;
						}

						NewCtx->OriginalInstance = FltInstance;

						Status = FltGetRequestorSessionId(Data, &NewCtx->RequestorSessionId);

						if (!NT_SUCCESS(Status))
						{
							PT_DBG_PRINT(PTDBG_TRACE_CRITICAL, ("nxrmflt!nxrmfltPostSetInformation: FltGetRequestorSessionId failed! Status is %x\n", Status));

							NewCtx->RequestorSessionId = NXRMFLT_INVALID_SESSION_ID;
						}

						FltInitializePushLock(&NewCtx->CtxLock);

						NewCtx->ContentDirty = 1;

						Status = FltSetStreamContext(FltInstance,
													 FileObject,
													 FLT_SET_CONTEXT_KEEP_IF_EXISTS,
													 NewCtx,
													 NULL);

						if (!NT_SUCCESS(Status))
						{
							PT_DBG_PRINT(PTDBG_TRACE_CRITICAL, ("nxrmflt!nxrmfltPostSetInformation: Failed to set stream Ctx! This could be normal. Status is %x\n", Status));

							break;
						}

						FltAcquirePushLockShared(&Global.NxlFileCacheLock);

						pNewCacheNode = FindNXLNodeInCache(&Global.NxlFileCache, &NewCtx->FileName);

						if(pNewCacheNode)
						{
							//
							// Chance for code run to here is really low
							//
							SetFlag(pNewCacheNode->Flags, NXRMFLT_FLAG_CTX_ATTACHED);
						}

						FltReleasePushLock(&Global.NxlFileCacheLock);

						if(!pNewCacheNode)
						{
							
							PT_DBG_PRINT(PTDBG_TRACE_RENAME,
										 ("nxrmflt!nxrmfltPostSetInformation: As expected, couldn't find the newly renamed file %wZ in cache!\n", 
										 &NewCtx->FileName));

							//
							// we need to build CacheNode here
							//

							pNewCacheNode = ExAllocateFromPagedLookasideList(&Global.NXLCacheLookaside);

							if(!pNewCacheNode)
							{
								PT_DBG_PRINT(PTDBG_TRACE_CRITICAL, ("nxrmflt!nxrmfltPostSetInformation: Failed to allocate cache node from lookaside!\n"));

								Status = STATUS_INSUFFICIENT_RESOURCES;
								break;
							}

							memset(pNewCacheNode, 0, sizeof(NXL_CACHE_NODE));

							ExInitializeRundownProtection(&pNewCacheNode->NodeRundownRef);

							pNewCacheNode->FileAttributes			= 0;
							pNewCacheNode->FileID.QuadPart			= 0;
							pNewCacheNode->Flags					= NXRMFLT_FLAG_CTX_ATTACHED;
							pNewCacheNode->Instance					= FltInstance;
							pNewCacheNode->OnRemoveOrRemovableMedia = FALSE;
							pNewCacheNode->LastWriteProcessId		= NULL;

							RtlHashUnicodeString(&NewCtx->FullPathParentDir, TRUE, HASH_STRING_ALGORITHM_X65599, &pNewCacheNode->ParentDirectoryHash);

							//
							// build "ReparseFileName" file name field
							//

							Status = build_nxlcache_reparse_name(pNewCacheNode, &Ccb->DestinationFileNameInfo->Name);

							if(!NT_SUCCESS(Status))
							{
								PT_DBG_PRINT(PTDBG_TRACE_CRITICAL, 
											 ("nxrmflt!nxrmfltPostSetInformation: Failed to build reparse name for %wZ! Status is %x\n", 
											 &Ccb->DestinationFileNameInfo->Name,
											 Status));
								break;
							}

							//
							// Ignore cases like renaming to different folder
							//

							Status = build_nxlcache_file_name(pNewCacheNode, &(NewCtx->FullPathParentDir), &Ccb->DestinationFileNameInfo->FinalComponent);

							if(!NT_SUCCESS(Status))
							{
								PT_DBG_PRINT(PTDBG_TRACE_CRITICAL, 
											 ("nxrmflt!nxrmfltPostSetInformation: Failed to build cache node name for %wZ! Status is %x\n", 
											 &Ccb->DestinationFileNameInfo->Name,
											 Status));

								break;
							}

							//
							// copy source file if there is any
							//

							do 
							{
								NXL_CACHE_NODE *pSrcCacheNodeTmp = NULL;

								FltAcquirePushLockShared(&Global.NxlFileCacheLock);

								pSrcCacheNodeTmp = FindNXLNodeInCache(&Global.NxlFileCache, &SrcOnDiskFileNameWithoutExtension);

								if (pSrcCacheNodeTmp)
								{
									if (!ExAcquireRundownProtection(&pSrcCacheNodeTmp->NodeRundownRef))
									{
										pSrcCacheNodeTmp = NULL;
									}
								}

								FltReleasePushLock(&Global.NxlFileCacheLock);

								if (!pSrcCacheNodeTmp)
								{
									break;
								}

								if (!pSrcCacheNodeTmp->OnRemoveOrRemovableMedia)
								{
									ExReleaseRundownProtection(&pSrcCacheNodeTmp->NodeRundownRef);
									break;
								}

								pNewCacheNode->OnRemoveOrRemovableMedia = pSrcCacheNodeTmp->OnRemoveOrRemovableMedia;

								build_nxlcache_source_file_name(pNewCacheNode, &pSrcCacheNodeTmp->SourceFileName);

								ExReleaseRundownProtection(&pSrcCacheNodeTmp->NodeRundownRef);

							} while (FALSE);


							FltAcquirePushLockExclusive(&Global.NxlFileCacheLock);

							//
							// there is no chance this will fail because renaming hold VCB lock which means no two rename on the same volume can happen at the same time
							//
							if(!AddNXLNodeToCache(&Global.NxlFileCache, pNewCacheNode))
							{
								FreeNewCacheNode = TRUE;
							}
	
							FltReleasePushLock(&Global.NxlFileCacheLock);

							if(FreeNewCacheNode)
							{
								FreeNXLCacheNode(pNewCacheNode);
								pNewCacheNode = NULL;
							}

							SkipUpdateNxlCache = TRUE;

							Status = nxrmfltCopyOnDiskNxlFile(FltInstance,
															  &SrcOnDiskNxlFileName,
															  FltInstance,
															  &DstOnDiskNxlFileName);

							if(!NT_SUCCESS(Status))
							{
								PT_DBG_PRINT(PTDBG_TRACE_CRITICAL, 
											 ("nxrmflt!nxrmfltPostSetInformation: Failed to copy on disk Nxl file from %wZ to %wZ. Status is %x\n",
											 &SrcOnDiskNxlFileName,
											 &DstOnDiskNxlFileName,
											 Status));
								break;
							}
						}
					
					} while (FALSE);

					if(NewCtx)
					{
						FltReleaseContext(NewCtx);
						NewCtx = NULL;
					}

					if(!NT_SUCCESS(Status) && pNewCacheNode)
					{
						PT_DBG_PRINT(PTDBG_TRACE_CRITICAL, 
									 ("nxrmflt!nxrmfltPostSetInformation: Failed to build new Ctx or build new cache node for %wZ! Status is %x. pNewCacheNode is %p\n", 
									 &Ccb->DestinationFileNameInfo->Name,
									 Status,
									 pNewCacheNode));

						FreeNXLCacheNode(pNewCacheNode);
						pNewCacheNode = NULL;
					}
				}
				else
				{
					PT_DBG_PRINT(PTDBG_TRACE_RENAME,
								 ("nxrmflt!nxrmfltPostSetInformation: No need to track file %wZ\n",
								 &Ccb->DestinationFileNameInfo->Name));
				}

			} while (FALSE);

			if(SrcOnDiskNxlFileName.Buffer)
			{
				ExFreeToPagedLookasideList(&Global.FullPathLookaside, SrcOnDiskNxlFileName.Buffer);

				RtlInitUnicodeString(&SrcOnDiskNxlFileName, NULL);
			}

			if(DstOnDiskNxlFileName.Buffer)
			{
				ExFreeToPagedLookasideList(&Global.FullPathLookaside, DstOnDiskNxlFileName.Buffer);

				RtlInitUnicodeString(&DstOnDiskNxlFileName, NULL);
			}

		}

		if(SkipUpdateNxlCache)
		{
			PT_DBG_PRINT(PTDBG_TRACE_RENAME,
						 ("nxrmflt!nxrmfltPostSetInformation: No need to update cache node for file %wZ\n",
						 &Ccb->DestinationFileNameInfo->Name));

			break;
		}

		if (!Ctx)
		{
			//
			// in the case of explorer or other applications renaming nxl file
			// other applications may download content from network and rename saved temp file to nxl file
			//

			if (IsNXLFile(&(Ccb->SourceFileNameInfo->Extension)) &&
				IsNXLFile(&(Ccb->DestinationFileNameInfo->Extension)))
			{
				UNICODE_STRING SrcFileNameWithoutNXLExtension = {0};
				UNICODE_STRING DstFileNameWithoutNXLExtension = {0};

				SrcFileNameWithoutNXLExtension.Buffer			= Ccb->SourceFileNameInfo->Name.Buffer;
				SrcFileNameWithoutNXLExtension.Length			= Ccb->SourceFileNameInfo->Name.Length - 4 * sizeof(WCHAR);
				SrcFileNameWithoutNXLExtension.MaximumLength	= Ccb->SourceFileNameInfo->Name.MaximumLength;

				//
				// build new cache node, rename on disk nxl file and complete this I/O
				//

				do 
				{
					NXL_CACHE_NODE	*pExistingCacheNode = NULL;
					NXL_CACHE_NODE	*pNewRenamedNode = NULL;

					FltAcquirePushLockShared(&Global.NxlFileCacheLock);

					pExistingCacheNode = FindNXLNodeInCache(&Global.NxlFileCache, &SrcFileNameWithoutNXLExtension);

					if (pExistingCacheNode)
					{
						if (!ExAcquireRundownProtection(&pExistingCacheNode->NodeRundownRef))
						{
							pExistingCacheNode = NULL;
						}
					}

					FltReleasePushLock(&Global.NxlFileCacheLock);

					if (!pExistingCacheNode)
					{
						break;
					}

					//
					// following block of code builds pNode
					//
					do 
					{

						pNewRenamedNode = ExAllocateFromPagedLookasideList(&Global.NXLCacheLookaside);

						if(!pNewRenamedNode)
						{
							PT_DBG_PRINT(PTDBG_TRACE_CRITICAL, ("nxrmflt!nxrmfltPostSetInformation: Failed to allocate cache node from lookaside.\n"));

							break;
						}

						memset(pNewRenamedNode, 0, sizeof(NXL_CACHE_NODE));

						ExInitializeRundownProtection(&pNewRenamedNode->NodeRundownRef);

						pNewRenamedNode->FileAttributes				= pExistingCacheNode->FileAttributes;
						pNewRenamedNode->FileID						= pExistingCacheNode->FileID;
						pNewRenamedNode->Flags						= pExistingCacheNode->Flags;
						pNewRenamedNode->Instance					= pExistingCacheNode->Instance;
						pNewRenamedNode->ParentDirectoryHash		= pExistingCacheNode->ParentDirectoryHash;
						pNewRenamedNode->OnRemoveOrRemovableMedia	= pExistingCacheNode->OnRemoveOrRemovableMedia;
						pNewRenamedNode->LastWriteProcessId			= pExistingCacheNode->LastWriteProcessId;

						ClearFlag(pNewRenamedNode->Flags, NXRMFLT_FLAG_CTX_ATTACHED);
						SetFlag(pNewRenamedNode->Flags, NXRMFLT_FLAG_ATTACHING_CTX);

						DstFileNameWithoutNXLExtension.Buffer			= Ccb->DestinationFileNameInfo->Name.Buffer;
						DstFileNameWithoutNXLExtension.Length			= Ccb->DestinationFileNameInfo->Name.Length - 4 * sizeof(WCHAR);
						DstFileNameWithoutNXLExtension.MaximumLength	= Ccb->DestinationFileNameInfo->Name.MaximumLength;

						//
						// build "ReparseFileName" file name field
						//

						Status = build_nxlcache_reparse_name(pNewRenamedNode, &DstFileNameWithoutNXLExtension);

						if(!NT_SUCCESS(Status))
						{
							PT_DBG_PRINT(PTDBG_TRACE_CRITICAL, ("nxrmflt!nxrmfltPostSetInformation: Failed to build reparse name in cache node. Status is %x\n", Status));

							FreeNXLCacheNode(pNewRenamedNode);
							
							pNewRenamedNode = NULL;

							break;
						}

						//
						// Ignore cases like renaming to different folder
						//

						Status = build_nxlcache_file_name_ex(pNewRenamedNode, &DstFileNameWithoutNXLExtension);

						if(!NT_SUCCESS(Status))
						{
							PT_DBG_PRINT(PTDBG_TRACE_CRITICAL, ("nxrmflt!nxrmfltPostSetInformation: Failed to build cache name in cache node. Status is %x\n", Status));

							FreeNXLCacheNode(pNewRenamedNode);

							pNewRenamedNode = NULL;

							break;
						}

					} while (FALSE);

					//
					// Let's remove and delete the old Node
					//

					FltAcquirePushLockExclusive(&Global.NxlFileCacheLock);

					do 
					{
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

						if (pNewRenamedNode && SetFileInfoCtx->bDesIsInSafeDir)
						{
							//
							// Add newly created node into the cache
							//
							if(!AddNXLNodeToCache(&Global.NxlFileCache, pNewRenamedNode))
							{
								//
								// Other thread add this new nxl file into cache already
								//

							}
							else
							{
								//
								// don't free pNode
								//
								pNewRenamedNode = NULL;
							}
						}

					} while (FALSE);


					FltReleasePushLock(&Global.NxlFileCacheLock);

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

					if (pNewRenamedNode)
					{
						FreeNXLCacheNode(pNewRenamedNode);
						pNewRenamedNode = NULL;
					}

				} while (FALSE);

			}
			else if (!IsNXLFile(&(Ccb->SourceFileNameInfo->Extension)) &&
					 IsNXLFile(&(Ccb->DestinationFileNameInfo->Extension)) &&
					 RtlEqualUnicodeString(&Ccb->SourceFileNameInfo->ParentDir, &Ccb->DestinationFileNameInfo->ParentDir, TRUE))
			{
				//
				// in the case of explorer (no ctx) or other applications download content from network and rename saved temp file
				//
				NXL_CACHE_NODE	*pNewRenamedNode = NULL;
				UNICODE_STRING	DstFileNameWithoutNXLExtension = {0};

				ULONG DirHash = 0;

				LARGE_INTEGER FileId = {0};

				ULONG FileAttributes = 0;

				//
				// following block of code builds pNode
				//
				do 
				{
					pNewRenamedNode = ExAllocateFromPagedLookasideList(&Global.NXLCacheLookaside);

					if(!pNewRenamedNode)
					{
						PT_DBG_PRINT(PTDBG_TRACE_CRITICAL, ("nxrmflt!nxrmfltPostSetInformation: Failed to allocate cache node from lookaside.\n"));

						break;
					}

					memset(pNewRenamedNode, 0, sizeof(NXL_CACHE_NODE));

					RtlHashUnicodeString(&Ccb->DestinationFileNameInfo->ParentDir, TRUE, HASH_STRING_ALGORITHM_X65599, &DirHash);

					get_file_id_and_attribute(FltInstance, &Ccb->DestinationFileNameInfo->Name, &FileId, &FileAttributes);

					ExInitializeRundownProtection(&pNewRenamedNode->NodeRundownRef);

					pNewRenamedNode->FileAttributes				= FileAttributes;
					pNewRenamedNode->FileID						= FileId;
					pNewRenamedNode->Flags						= 0;
					pNewRenamedNode->Instance					= FltInstance;
					pNewRenamedNode->ParentDirectoryHash		= DirHash;
					pNewRenamedNode->OnRemoveOrRemovableMedia	= FALSE;
					pNewRenamedNode->LastWriteProcessId			= NULL;

					DstFileNameWithoutNXLExtension.Buffer			= Ccb->DestinationFileNameInfo->Name.Buffer;
					DstFileNameWithoutNXLExtension.Length			= Ccb->DestinationFileNameInfo->Name.Length - 4 * sizeof(WCHAR);
					DstFileNameWithoutNXLExtension.MaximumLength	= Ccb->DestinationFileNameInfo->Name.MaximumLength;

					//
					// build "ReparseFileName" file name field
					//

					Status = build_nxlcache_reparse_name(pNewRenamedNode, &DstFileNameWithoutNXLExtension);

					if(!NT_SUCCESS(Status))
					{
						PT_DBG_PRINT(PTDBG_TRACE_CRITICAL, ("nxrmflt!nxrmfltPostSetInformation: Failed to build reparse name in cache node. Status is %x\n", Status));

						FreeNXLCacheNode(pNewRenamedNode);

						pNewRenamedNode = NULL;

						break;
					}

					//
					// Ignore cases like renaming to different folder
					//

					Status = build_nxlcache_file_name_ex(pNewRenamedNode, &DstFileNameWithoutNXLExtension);

					if(!NT_SUCCESS(Status))
					{
						PT_DBG_PRINT(PTDBG_TRACE_CRITICAL, ("nxrmflt!nxrmfltPostSetInformation: Failed to build cache name in cache node. Status is %x\n", Status));

						FreeNXLCacheNode(pNewRenamedNode);

						pNewRenamedNode = NULL;

						break;
					}

				} while (FALSE);

				FltAcquirePushLockExclusive(&Global.NxlFileCacheLock);

				if (pNewRenamedNode)
				{
					//
					// Add newly created node into the cache
					//
					if(!AddNXLNodeToCache(&Global.NxlFileCache, pNewRenamedNode))
					{
						//
						// Other thread add this new nxl file into cache already
						//

					}
					else
					{
						//
						// don't free pNode
						//
						pNewRenamedNode = NULL;
					}
				}

				FltReleasePushLock(&Global.NxlFileCacheLock);

				if (pNewRenamedNode)
				{
					FreeNXLCacheNode(pNewRenamedNode);
					pNewRenamedNode = NULL;
				}
			}
			else if (IsNXLFile(&Ccb->SourceFileNameInfo->Extension) &&
				     !IsNXLFile(&Ccb->DestinationFileNameInfo->Extension))
			{
				//
				// in the case of explorer rename a NXL file to other file
				// we need to update NXL cache node and rename on disk decrypted
				// file
				//
				NXL_CACHE_NODE	*pNewRenamedNode = NULL;
				UNICODE_STRING	DstDecryptedFile = {0};
				UNICODE_STRING	DstNXLFileName = {0};
				UNICODE_STRING	SrcNXLFileName = {0};
				ULONG DirHash = 0;

				LARGE_INTEGER FileId = {0};

				ULONG FileAttributes = 0;

				do 
				{
					SrcNXLFileName.Buffer			= Ccb->SourceFileNameInfo->Name.Buffer;
					SrcNXLFileName.Length			= Ccb->SourceFileNameInfo->Name.Length - 4 * sizeof(WCHAR);
					SrcNXLFileName.MaximumLength	= Ccb->SourceFileNameInfo->Name.MaximumLength;

					//
					// step 1: find the source in cache node
					//
					FltAcquirePushLockExclusive(&Global.NxlFileCacheLock);

					pCacheNode = FindNXLNodeInCache(&Global.NxlFileCache, &SrcNXLFileName);

					if (pCacheNode)
					{
						if (!ExAcquireRundownProtection(&pCacheNode->NodeRundownRef))
						{
							pCacheNode = NULL;
						}
					}

					FltReleasePushLock(&Global.NxlFileCacheLock);

					if (!pCacheNode)
					{
						break;
					}

					//
					// step 2: build DstNXLFileName
					//
					DstNXLFileName.Buffer = ExAllocateFromPagedLookasideList(&Global.FullPathLookaside);

					if(!DstNXLFileName.Buffer)
					{
						PT_DBG_PRINT(PTDBG_TRACE_CRITICAL, ("nxrmflt!nxrmfltPostSetInformation: Failed to allocate file name buffer from lookaside.\n"));
						break;
					}

					memset(DstNXLFileName.Buffer, 0, NXRMFLT_FULLPATH_BUFFER_SIZE);

					DstNXLFileName.MaximumLength	= NXRMFLT_FULLPATH_BUFFER_SIZE;
					DstNXLFileName.Length			= 0;

					RtlUnicodeStringCat(&DstNXLFileName, &Ccb->DestinationFileNameInfo->Name);
					RtlUnicodeStringCatString(&DstNXLFileName, NXRMFLT_NXL_DOTEXT);
					
					//
					// step 3: build DstDecryptedFileName
					//
					DstDecryptedFile.Buffer			= Ccb->DestinationFileNameInfo->Name.Buffer;
					DstDecryptedFile.Length			= Ccb->DestinationFileNameInfo->Name.Length;
					DstDecryptedFile.MaximumLength	= Ccb->DestinationFileNameInfo->Name.MaximumLength;

					//
					// step 4: build pNewRenamedNode
					//

					pNewRenamedNode = ExAllocateFromPagedLookasideList(&Global.NXLCacheLookaside);

					if(!pNewRenamedNode)
					{
						PT_DBG_PRINT(PTDBG_TRACE_CRITICAL, ("nxrmflt!nxrmfltPostSetInformation: Failed to allocate cache node from lookaside.\n"));
						break;
					}

					memset(pNewRenamedNode, 0, sizeof(NXL_CACHE_NODE));

					ExInitializeRundownProtection(&pNewRenamedNode->NodeRundownRef);

					RtlHashUnicodeString(&Ccb->DestinationFileNameInfo->ParentDir, 
										 TRUE, HASH_STRING_ALGORITHM_X65599, 
										 &DirHash);

					pNewRenamedNode->FileAttributes				= pCacheNode->FileAttributes;
					pNewRenamedNode->FileID						= pCacheNode->FileID;
					pNewRenamedNode->Flags						= 0;						// clear flags because we are going to delete decrypted file
					pNewRenamedNode->Instance					= pCacheNode->Instance;
					pNewRenamedNode->ParentDirectoryHash		= DirHash;
					pNewRenamedNode->OnRemoveOrRemovableMedia	= pCacheNode->OnRemoveOrRemovableMedia;
					pNewRenamedNode->LastWriteProcessId			= pCacheNode->LastWriteProcessId;

					//
					// step 5: build "ReparseFileName" field
					//

					Status = build_nxlcache_reparse_name(pNewRenamedNode, &DstDecryptedFile);

					if(!NT_SUCCESS(Status))
					{
						PT_DBG_PRINT(PTDBG_TRACE_CRITICAL, ("nxrmflt!nxrmfltPostSetInformation: Failed to build reparse name in cache node. Status is %x\n", Status));
						break;
					}

					//
					// step 6: build "FileName" field
					//
					Status = build_nxlcache_file_name_ex(pNewRenamedNode, &DstDecryptedFile);

					if(!NT_SUCCESS(Status))
					{
						PT_DBG_PRINT(PTDBG_TRACE_CRITICAL, ("nxrmflt!nxrmfltPostSetInformation: Failed to build cache name in cache node. Status is %x\n", Status));
						break;
					}

					//
					// step 7: delete decrypted file
					//
					if (FlagOn(pCacheNode->Flags, NXRMFLT_FLAG_CTX_ATTACHED) ||
						FlagOn(pCacheNode->Flags, NXRMFLT_FLAG_ATTACHING_CTX))
					{
						Status = nxrmfltDeleteFileByName(FltInstance, 
														 &pCacheNode->ReparseFileName);

						if(!NT_SUCCESS(Status))
						{
							PT_DBG_PRINT(PTDBG_TRACE_CRITICAL, ("nxrmflt!nxrmfltPostSetInformation: Can't delete file %wZ! Status is %x\n", &pCacheNode->ReparseFileName, Status));
							break;
						}
					}

					//
					// step 8: rename NXL file
					//
					Status = nxrmfltRenameOnDiskNXLFile(FltInstance,
														&Ccb->DestinationFileNameInfo->Name,
														&DstNXLFileName);

					if(!NT_SUCCESS(Status))
					{
						PT_DBG_PRINT(PTDBG_TRACE_CRITICAL, 
									("nxrmflt!nxrmfltPostSetInformation: Failed to rename %wZ to %wZ. Status is %x\n", 
									&Ccb->DestinationFileNameInfo->Name, 
									&DstNXLFileName,
									Status));

						break;
					}

					//
					// update file Id and attribute
					// not all file system keep file ID the same
					//
					get_file_id_and_attribute(FltInstance, &DstNXLFileName, &FileId, &FileAttributes);

					pNewRenamedNode->FileID			= FileId;
					pNewRenamedNode->FileAttributes = FileAttributes;


					//
					// new Node is fully ready. all on disk files have been renamed. Let's remove and delete the old Node and add new node
					//

					FltAcquirePushLockExclusive(&Global.NxlFileCacheLock);

					do 
					{
						//
						// Delete old cache node from cache. NOTE: pCacheNode still valid after delete
						// We only delete the node from cache. We don't free resource here
						//
						DeleteNXLNodeInCache(&Global.NxlFileCache, pCacheNode);

						//
						// Add newly created node into the cache
						//
						if (SetFileInfoCtx->bDesIsInSafeDir)
						{
							if (AddNXLNodeToCache(&Global.NxlFileCache, pNewRenamedNode))
							{
								pNewRenamedNode = NULL;
							}
						}
					} while (FALSE);

					FltReleasePushLock(&Global.NxlFileCacheLock);

					//
					// purge rights cache
					//
					nxrmfltPurgeRightsCache(pCacheNode->Instance, pCacheNode->FileNameHash);

					//
					// release old cache node rundown protection
					//
					ExReleaseRundownProtection(&pCacheNode->NodeRundownRef);
					//
					// free old cache node
					//
					FreeNXLCacheNode(pCacheNode);
					pCacheNode = NULL;

				} while (FALSE);

				if (DstNXLFileName.Buffer)
				{
					ExFreeToPagedLookasideList(&Global.FullPathLookaside, DstNXLFileName.Buffer);
					RtlInitUnicodeString(&DstNXLFileName, NULL);
				}

				//
				// in the case of error, pCacheNode is not NULL
				//
				if (pCacheNode)
				{
					ExReleaseRundownProtection(&pCacheNode->NodeRundownRef);
					pCacheNode = NULL;
				}

				//
				// in the case of other thread add new node to cache, pNewRenamedNode is not NULL
				//
				if (pNewRenamedNode)
				{
					FreeNXLCacheNode(pNewRenamedNode);
					pNewRenamedNode = NULL;
				}
			}
			else if (!IsNXLFile(&(Ccb->SourceFileNameInfo->Extension)) &&
					 !IsNXLFile(&(Ccb->DestinationFileNameInfo->Extension)))
			{
				if (RtlEqualUnicodeString(&Ccb->SourceFileNameInfo->ParentDir, &Ccb->DestinationFileNameInfo->ParentDir, TRUE))
				{
					FltAcquirePushLockShared(&Global.NxlFileCacheLock);

					pCacheNode = FindNXLNodeInCache(&Global.NxlFileCache, &Ccb->DestinationFileNameInfo->Name);

					if (pCacheNode)
					{
						Ccb->EncryptDestinationFile = TRUE;
					}

					FltReleasePushLock(&Global.NxlFileCacheLock);

					pCacheNode = NULL;
				}

				{
					ADOBE_RENAME_NODE *pAdobeRenameNode = NULL;

					FltAcquirePushLockExclusive(&Global.AdobeRenameExpireTableLock);

					FOR_EACH_LIST(ite, &Global.AdobeRenameExpireTable)
					{
						pAdobeRenameNode = CONTAINING_RECORD(ite, ADOBE_RENAME_NODE, Link);

						if (RtlEqualUnicodeString(&pAdobeRenameNode->SourceFileName, &Ccb->DestinationFileNameInfo->Name, TRUE))
						{
							RemoveEntryList(ite);
							break;
						}
						else
						{
							pAdobeRenameNode = NULL;
						}
					}

					FltReleasePushLock(&Global.AdobeRenameExpireTableLock);

					if (pAdobeRenameNode)
					{
						NTSTATUS LocalStatus = STATUS_SUCCESS;

						ULONG RequestorSessionId = NXRMFLT_INVALID_SESSION_ID;

						//
						// ignore the return status
						// RequestorSessionId is NXRMFLT_INVALID_SESSION_ID if FltGetRequestorSessionId fails
						//
						if (STATUS_UNSUCCESSFUL == FltGetRequestorSessionId(Data, &RequestorSessionId))
						{
							RequestorSessionId = NXRMFLT_INVALID_SESSION_ID;
						}

						LocalStatus = nxrmfltDuplicateNXLFileAndItsRecords(getProcessIDFromData(Data),
																		   RequestorSessionId,
																		   &pAdobeRenameNode->DestinationFileName,
																		   &Ccb->DestinationFileNameInfo->Name,
																		   FltInstance,
																		   &Ccb->DestinationFileNameInfo->ParentDir);
						
						if (!NT_SUCCESS(LocalStatus))
						{
							PT_DBG_PRINT(PTDBG_TRACE_CRITICAL, 
										 ("nxrmflt!nxrmfltPostSetInformation: Failed to duplicate %wZ to %wZ. Status is %x\n", 
										 &pAdobeRenameNode->DestinationFileName, 
										 &Ccb->DestinationFileNameInfo->Name,
										 LocalStatus));
						}

						nxrmfltFreeAdobeRenameNode(pAdobeRenameNode);
						pAdobeRenameNode = NULL;
					}
				}
			}

			break;
		}

		FltAcquirePushLockShared(&Global.NxlFileCacheLock);

		pCacheNode = FindNXLNodeInCache(&Global.NxlFileCache, &Ctx->FileName);

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
			if (FlagOn(pCacheNode->Flags, NXRMFLT_FLAG_STOGRAGE_EJECTED))
			{
				MediaEjected = TRUE;
			}

			if (FileInformationClass == FileBasicInformation)
			{
				FileBasicInfo = (PFILE_BASIC_INFORMATION)Data->Iopb->Parameters.SetFileInformation.InfoBuffer;

				if (FileBasicInfo)
				{
					if (FileBasicInfo->FileAttributes != pCacheNode->FileAttributes)
					{
						pCacheNode->FileAttributes = FileBasicInfo->FileAttributes;

						InterlockedExchange(&Ctx->ContentDirty, 1);

						PT_DBG_PRINT(PTDBG_TRACE_CRITICAL, ("nxrmflt!nxrmfltPostSetInformation: Set content dirty flag.\n"));
					}
				}
			}
			else if(FileInformationClass == FileRenameInformation || FileInformationClass == FileRenameInformationEx)
			{
				NT_ASSERT(NameInfo);

				PT_DBG_PRINT(PTDBG_TRACE_CRITICAL, ("nxrmflt!nxrmfltPostSetInformation: Set content dirty flag when renaming. Ctx->FileName is %wZ\n", &Ctx->FileName));

				do 
				{

					//Status = FltGetTunneledName(Data, NameInfo, &TunnelNameInfo);

					//if(!TunnelNameInfo)
					//{
					//	break;
					//}

					//FltReleaseFileNameInformation(NameInfo);
					//NameInfo = TunnelNameInfo;

					//Status = FltParseFileNameInformation(NameInfo);

					//if(!NT_SUCCESS(Status))
					//{
					//	break;
					//}

					OnDiskNXLFileName.Buffer = ExAllocateFromPagedLookasideList(&Global.FullPathLookaside);

					if(!OnDiskNXLFileName.Buffer)
					{
						PT_DBG_PRINT(PTDBG_TRACE_CRITICAL, ("nxrmflt!nxrmfltPostSetInformation: Failed to allocate file name buffer from lookaside.\n"));

						Status = STATUS_INSUFFICIENT_RESOURCES;
						break;
					}

					memset(OnDiskNXLFileName.Buffer, 0, NXRMFLT_FULLPATH_BUFFER_SIZE);

					OnDiskNXLFileName.MaximumLength = NXRMFLT_FULLPATH_BUFFER_SIZE;
					OnDiskNXLFileName.Length		= 0;

					RtlUnicodeStringCat(&OnDiskNXLFileName, &NameInfo->Name);
					RtlUnicodeStringCatString(&OnDiskNXLFileName, NXRMFLT_NXL_DOTEXT);

					//
					// issuing rename request to rename the real NXL file. 
					//

					Status = nxrmfltRenameOnDiskNXLFile(Ctx->OriginalInstance,
													    &pCacheNode->OriginalFileName,
													    &OnDiskNXLFileName);


					if(!NT_SUCCESS(Status))
					{
						PT_DBG_PRINT(PTDBG_TRACE_CRITICAL, 
									 ("nxrmflt!nxrmfltPostSetInformation: Failed to rename %wZ to %wZ. Status is %x\n", 
									 &pCacheNode->OriginalFileName, 
									 &OnDiskNXLFileName,
									 Status));

						break;
					}

					InterlockedExchange(&Ctx->ContentDirty, 1); //update content dirty flag after rename

					PT_DBG_PRINT(PTDBG_TRACE_RENAME,
								 ("nxrmflt!nxrmfltPostSetInformation: Renamed on disk file %wZ to %wZ.\n",
								 &pCacheNode->OriginalFileName,
								 &OnDiskNXLFileName));

					pNode = ExAllocateFromPagedLookasideList(&Global.NXLCacheLookaside);

					if(!pNode)
					{
						PT_DBG_PRINT(PTDBG_TRACE_CRITICAL, ("nxrmflt!nxrmfltPostSetInformation: Failed to allocate cache node from lookaside.\n"));

						Status = STATUS_INSUFFICIENT_RESOURCES;
						break;
					}

					memset(pNode, 0, sizeof(NXL_CACHE_NODE));

					ExInitializeRundownProtection(&pNode->NodeRundownRef);

					pNode->FileAttributes			= pCacheNode->FileAttributes;
					pNode->FileID					= pCacheNode->FileID;
					pNode->Flags					= pCacheNode->Flags;
					pNode->Instance					= pCacheNode->Instance;
					pNode->ParentDirectoryHash		= pCacheNode->ParentDirectoryHash;
					pNode->OnRemoveOrRemovableMedia = pCacheNode->OnRemoveOrRemovableMedia;
					pNode->LastWriteProcessId		= pCacheNode->LastWriteProcessId;

					//
					// build "ReparseFileName" file name field
					//

					Status = build_nxlcache_reparse_name(pNode, &NameInfo->Name);

					if(!NT_SUCCESS(Status))
					{
						PT_DBG_PRINT(PTDBG_TRACE_CRITICAL, ("nxrmflt!nxrmfltPostSetInformation: Failed to build reparse name in cache node. Status is %x\n", Status));

						break;
					}
					

					Status = build_nxlcache_file_name_ex(pNode, &NameInfo->Name);

					if(!NT_SUCCESS(Status))
					{
						PT_DBG_PRINT(PTDBG_TRACE_CRITICAL, ("nxrmflt!nxrmfltPostSetInformation: Failed to build cache name in cache node. Status is %x\n", Status));

						break;
					}

					//
					// copy source file name field if there is any
					//
					if (pCacheNode->OnRemoveOrRemovableMedia)
					{
						Status = build_nxlcache_source_file_name(pNode, &pCacheNode->SourceFileName);

						if (!NT_SUCCESS(Status))
						{
							PT_DBG_PRINT(PTDBG_TRACE_CRITICAL, ("nxrmflt!nxrmfltPostSetInformation: Failed to build source file name in cache node. Status is %x\n", Status));
							break;
						}
					}
					else
					{
						RtlInitUnicodeString(&pNode->SourceFileName, NULL);
					}
					
					//
					// update Ctx
					//

					Status = nxrmfltBuildNamesInStreamContext(Ctx, &pNode->FileName);

					if(!NT_SUCCESS(Status))
					{
						PT_DBG_PRINT(PTDBG_TRACE_CRITICAL, ("nxrmflt!nxrmfltPostSetInformation: Failed to build names in stream Ctx. Status is %x\n", Status));

						break;
					}

					//
					// new Node is fully ready. Let's remove and delete the old Node
					//

					FltAcquirePushLockExclusive(&Global.NxlFileCacheLock);

					do 
					{
						//
						// Delete old cache node from cache. NOTE: pCacheNode still valid after delete
						// We only delete the node from cache. We don't free resource here
						//
						DeleteNXLNodeInCache(&Global.NxlFileCache, pCacheNode);

						//
						// Add newly created node into the cache
						//
						if (SetFileInfoCtx->bDesIsInSafeDir)
						{
							if (!AddNXLNodeToCache(&Global.NxlFileCache, pNode))
							{
								PT_DBG_PRINT(PTDBG_TRACE_CRITICAL, ("nxrmflt!nxrmfltPostSetInformation: FATAL ERROR!!!!!!!!!!!!!"));
							}
						}
					} while (FALSE);

					FltReleasePushLock(&Global.NxlFileCacheLock);

					//
					// purge rights cache
					//
					nxrmfltPurgeRightsCache(pCacheNode->Instance, pCacheNode->FileNameHash);

					//
					// release old cache node rundown protection
					//
					ExReleaseRundownProtection(&pCacheNode->NodeRundownRef);
					//
					// free old cache node
					//
					FreeNXLCacheNode(pCacheNode);
					pCacheNode = NULL;

				} while (FALSE);

				if(!NT_SUCCESS(Status))
				{
					//
					// in this case, OS successfully rename the real file but we failed to update our record
					// THIS IS A NORMAL CASE and we need to recall the rename operation on the real file
					//
					NTSTATUS RevertStatus = STATUS_SUCCESS;

					RevertStatus = nxrmfltRenameFile(FltInstance, &(NameInfo->Name), &(pCacheNode->FileName));

					if (!NT_SUCCESS(RevertStatus))
					{
						PT_DBG_PRINT(PTDBG_TRACE_CRITICAL, ("nxrmflt!nxrmfltPostSetInformation: FATAL ERROR! Failed to revert rename operation!\n"));
					}

					Data->IoStatus.Status = Status;
					
					if(pNode)
					{
						FreeNXLCacheNode(pNode);
						pNode = NULL;
					}

					break;
				}

				//
				// build Adobe rename node
				//
				if (is_adobe_like_process())
				{
					nxrmfltBuildAdobeRenameNode(&Ccb->SourceFileNameInfo->Name, &Ccb->DestinationFileNameInfo->Name);
				}

			}
			else
			{
				InterlockedExchange(&Ctx->ContentDirty, 1);

				PT_DBG_PRINT(PTDBG_TRACE_CRITICAL, ("nxrmflt!nxrmfltPostSetInformation: Set content dirty flag\n"));
			}
		}
		else
		{
			if (FileInformationClass == FileRenameInformation || FileInformationClass == FileRenameInformationEx)
			{
				NT_ASSERT(NameInfo);

				Status = nxrmfltBuildNamesInStreamContext(Ctx, &NameInfo->Name);

				if(!NT_SUCCESS(Status))
				{
					PT_DBG_PRINT(PTDBG_TRACE_CRITICAL, ("nxrmflt!nxrmfltPostSetInformation: Failed to build names in stream Ctx. Status is %x\n", Status));
				}
			}
			else
			{
				PT_DBG_PRINT(PTDBG_TRACE_CRITICAL,("nxrmflt!nxrmfltPostSetInformation: FATAL ERROR!!! Can't find cached record of %wZ.\n", Ctx->FileName));
			}
		}

		if (MediaEjected)
		{
			Data->IoStatus.Status = STATUS_LOST_WRITEBEHIND_DATA;
			Data->IoStatus.Information = 0;
		}

	} while (FALSE);

	if (Ctx)
	{
		FltReleaseContext(Ctx);
	}

	if (Ccb)
	{
		FltReleaseContext(Ccb);
	}

	if (pCacheNode)
	{
		ExReleaseRundownProtection(&pCacheNode->NodeRundownRef);
	}

	if (NameInfo && ReleaseNameInfo)
	{
		FltReleaseFileNameInformation(NameInfo);
	}

	if (OnDiskNXLFileName.Buffer)
	{
		ExFreeToPagedLookasideList(&Global.FullPathLookaside, OnDiskNXLFileName.Buffer);
	}

	if (pRenameNode)
	{
		nxrmfltFreeRenameNode(pRenameNode);

		ExFreeToPagedLookasideList(&Global.RenameCacheLookaside, pRenameNode);

		pRenameNode = NULL;
	}

	if (SetFileInfoCtx)
	{
		ExFreeToPagedLookasideList(&Global.SetInformationCtxLookaside, SetFileInfoCtx);
		
		SetFileInfoCtx = NULL;
	}

	return CallbackStatus;
}

static NTSTATUS build_nxlcache_file_name(NXL_CACHE_NODE *pNode, UNICODE_STRING *ParentDirName, UNICODE_STRING *FinalComponent)
{
	//
	// FinalComponent does NOT include ".nxl" extension when Global.HideNXLExtension is TRUE
	//
	if (Global.HideNXLExtension)
	{
		return build_nxlcache_file_name_from_name_without_nxl_extension(pNode, ParentDirName, FinalComponent);
	}
	else
	{
		return build_nxlcache_file_name_from_name_with_nxl_extension(pNode, ParentDirName, FinalComponent);
	}

}

static NTSTATUS build_nxlcache_file_name_from_name_with_nxl_extension(NXL_CACHE_NODE *pNode, UNICODE_STRING *ParentDirName, UNICODE_STRING *FinalComponent)
{
	NTSTATUS Status = STATUS_SUCCESS;
	
	USHORT NewFileNameLength = 0;

	do 
	{
		NewFileNameLength = ParentDirName->Length + FinalComponent->Length;

		if(NewFileNameLength > sizeof(pNode->FileNameFastBuffer))
		{
			pNode->FileName.Buffer = pNode->OriginalFileName.Buffer = ExAllocatePoolWithTag(PagedPool, NewFileNameLength, NXRMFLT_NXLCACHE_TAG);

			if(!pNode->FileName.Buffer)
			{
				Status = STATUS_INSUFFICIENT_RESOURCES;
				break;
			}

			pNode->ReleaseFileName = TRUE;

			pNode->FileName.MaximumLength = pNode->OriginalFileName.MaximumLength = NewFileNameLength;
			pNode->FileName.Length = pNode->OriginalFileName.Length = NewFileNameLength;
		}
		else
		{
			pNode->ReleaseFileName = FALSE;

			pNode->FileName.Buffer = pNode->OriginalFileName.Buffer = pNode->FileNameFastBuffer;
			pNode->FileName.MaximumLength = pNode->OriginalFileName.MaximumLength = sizeof(pNode->FileNameFastBuffer);
			pNode->FileName.Length	= pNode->OriginalFileName.Length = NewFileNameLength;

		}

		memcpy(pNode->FileName.Buffer,
			   ParentDirName->Buffer,
			   ParentDirName->Length);

		memcpy(pNode->FileName.Buffer + ParentDirName->Length / sizeof(WCHAR),
			   FinalComponent->Buffer,
			   FinalComponent->Length);

		RtlHashUnicodeString(&pNode->FileName, TRUE, HASH_STRING_ALGORITHM_X65599, &pNode->FileNameHash);

	} while (FALSE);

	return Status;
}

static NTSTATUS build_nxlcache_file_name_from_name_without_nxl_extension(NXL_CACHE_NODE *pNode, UNICODE_STRING *ParentDirName, UNICODE_STRING *FinalComponent)
{
	NTSTATUS Status = STATUS_SUCCESS;

	USHORT NewFileNameLength = 0;

	do 
	{
		NewFileNameLength = ParentDirName->Length + FinalComponent->Length + sizeof(NXRMFLT_NXL_DOTEXT) - sizeof(WCHAR);

		if(NewFileNameLength > sizeof(pNode->FileNameFastBuffer))
		{
			pNode->FileName.Buffer = pNode->OriginalFileName.Buffer = ExAllocatePoolWithTag(PagedPool, NewFileNameLength, NXRMFLT_NXLCACHE_TAG);

			if(!pNode->FileName.Buffer)
			{
				Status = STATUS_INSUFFICIENT_RESOURCES;
				break;
			}

			pNode->ReleaseFileName = TRUE;

			pNode->FileName.MaximumLength = pNode->OriginalFileName.MaximumLength = NewFileNameLength;
			pNode->FileName.Length = ParentDirName->Length + FinalComponent->Length;
			pNode->OriginalFileName.Length = NewFileNameLength;
		}
		else
		{
			pNode->ReleaseFileName = FALSE;

			pNode->FileName.Buffer = pNode->OriginalFileName.Buffer = pNode->FileNameFastBuffer;
			pNode->FileName.MaximumLength = pNode->OriginalFileName.MaximumLength = sizeof(pNode->FileNameFastBuffer);
			pNode->FileName.Length	= ParentDirName->Length + FinalComponent->Length;
			pNode->OriginalFileName.Length = NewFileNameLength;
		}

		memcpy(pNode->FileName.Buffer,
			   ParentDirName->Buffer,
			   ParentDirName->Length);

		memcpy(pNode->FileName.Buffer + ParentDirName->Length / sizeof(WCHAR),
			   FinalComponent->Buffer,
			   FinalComponent->Length);

		memcpy(pNode->FileName.Buffer + ((ParentDirName->Length + FinalComponent->Length) / sizeof(WCHAR)), 
			   NXRMFLT_NXL_DOTEXT, 
			   (sizeof(NXRMFLT_NXL_DOTEXT)-sizeof(WCHAR)));

		RtlHashUnicodeString(&pNode->FileName, TRUE, HASH_STRING_ALGORITHM_X65599, &pNode->FileNameHash);

	} while (FALSE);

	return Status;
}

static NTSTATUS build_nxlcache_reparse_name(NXL_CACHE_NODE *pNode, UNICODE_STRING *NewReparseName)
{
	NTSTATUS Status = STATUS_SUCCESS;
	
	do 
	{
		if(NewReparseName->Length > sizeof(pNode->ReparseFileNameFastBuffer))
		{
			pNode->ReparseFileName.Buffer = ExAllocatePoolWithTag(PagedPool, NewReparseName->Length, NXRMFLT_NXLCACHE_TAG);

			if(!pNode->ReparseFileName.Buffer)
			{
				Status = STATUS_INSUFFICIENT_RESOURCES;
				break;
			}

			pNode->ReparseFileName.MaximumLength	= NewReparseName->Length;
			pNode->ReparseFileName.Length			= 0;

			pNode->ReleaseReparseName = TRUE;

		}
		else
		{
			pNode->ReparseFileName.Buffer			= pNode->ReparseFileNameFastBuffer;
			pNode->ReparseFileName.MaximumLength	= sizeof(pNode->ReparseFileNameFastBuffer);
			pNode->ReparseFileName.Length			= 0;

			pNode->ReleaseReparseName = FALSE;
		}

		RtlUnicodeStringCat(&pNode->ReparseFileName, NewReparseName);

	} while (FALSE);

	return Status;
}

//////////////////////////////////////////////////////////////////////////
//
// junk code
//
//////////////////////////////////////////////////////////////////////////

static NTSTATUS build_nxlcache_file_name_ex(NXL_CACHE_NODE *pNode, UNICODE_STRING *FullPathFileName)
{
	//
	// FinalComponent does NOT include ".nxl" extension when Global.HideNXLExtension is TRUE
	//
	if (Global.HideNXLExtension)
	{
		return build_nxlcache_file_name_from_name_without_nxl_extension_ex(pNode, FullPathFileName);
	}
	else
	{
		return build_nxlcache_file_name_from_name_with_nxl_extension_ex(pNode, FullPathFileName);
	}

}

static NTSTATUS build_nxlcache_file_name_from_name_with_nxl_extension_ex(NXL_CACHE_NODE *pNode, UNICODE_STRING *FullPathFileName)
{
	NTSTATUS Status = STATUS_SUCCESS;

	USHORT NewFileNameLength = 0;

	do 
	{
		NewFileNameLength = FullPathFileName->Length;

		if(NewFileNameLength > sizeof(pNode->FileNameFastBuffer))
		{
			pNode->FileName.Buffer = pNode->OriginalFileName.Buffer = ExAllocatePoolWithTag(PagedPool, NewFileNameLength, NXRMFLT_NXLCACHE_TAG);

			if(!pNode->FileName.Buffer)
			{
				Status = STATUS_INSUFFICIENT_RESOURCES;
				break;
			}

			pNode->ReleaseFileName = TRUE;

			pNode->FileName.MaximumLength = pNode->OriginalFileName.MaximumLength = NewFileNameLength;
			pNode->FileName.Length = pNode->OriginalFileName.Length = NewFileNameLength;
		}
		else
		{
			pNode->ReleaseFileName = FALSE;

			pNode->FileName.Buffer			= pNode->OriginalFileName.Buffer = pNode->FileNameFastBuffer;
			pNode->FileName.MaximumLength	= pNode->OriginalFileName.MaximumLength = sizeof(pNode->FileNameFastBuffer);
			pNode->FileName.Length			= pNode->OriginalFileName.Length = NewFileNameLength;

		}

		memcpy(pNode->FileName.Buffer,
			   FullPathFileName->Buffer,
			   FullPathFileName->Length);

		RtlHashUnicodeString(&pNode->FileName, TRUE, HASH_STRING_ALGORITHM_X65599, &pNode->FileNameHash);

	} while (FALSE);

	return Status;
}

static NTSTATUS build_nxlcache_file_name_from_name_without_nxl_extension_ex(NXL_CACHE_NODE *pNode, UNICODE_STRING *FullPathFileName)
{
	NTSTATUS Status = STATUS_SUCCESS;

	USHORT NewFileNameLength = 0;

	do 
	{
		NewFileNameLength = FullPathFileName->Length + sizeof(NXRMFLT_NXL_DOTEXT) - sizeof(WCHAR);

		if(NewFileNameLength > sizeof(pNode->FileNameFastBuffer))
		{
			pNode->FileName.Buffer = pNode->OriginalFileName.Buffer = ExAllocatePoolWithTag(PagedPool, NewFileNameLength, NXRMFLT_NXLCACHE_TAG);

			if(!pNode->FileName.Buffer)
			{
				Status = STATUS_INSUFFICIENT_RESOURCES;
				break;
			}

			pNode->ReleaseFileName = TRUE;

			pNode->FileName.MaximumLength	= pNode->OriginalFileName.MaximumLength = NewFileNameLength;
			pNode->FileName.Length			= FullPathFileName->Length;
			pNode->OriginalFileName.Length	= NewFileNameLength;
		}
		else
		{
			pNode->ReleaseFileName = FALSE;

			pNode->FileName.Buffer			= pNode->OriginalFileName.Buffer = pNode->FileNameFastBuffer;
			pNode->FileName.MaximumLength	= pNode->OriginalFileName.MaximumLength = sizeof(pNode->FileNameFastBuffer);
			pNode->FileName.Length			= FullPathFileName->Length;
			pNode->OriginalFileName.Length	= NewFileNameLength;
		}

		memcpy(pNode->FileName.Buffer,
			   FullPathFileName->Buffer,
			   FullPathFileName->Length);

		memcpy(pNode->FileName.Buffer + ((FullPathFileName->Length) / sizeof(WCHAR)), 
			   NXRMFLT_NXL_DOTEXT, 
			   (sizeof(NXRMFLT_NXL_DOTEXT)-sizeof(WCHAR)));

		RtlHashUnicodeString(&pNode->FileName, TRUE, HASH_STRING_ALGORITHM_X65599, &pNode->FileNameHash);

	} while (FALSE);

	return Status;
}

static NTSTATUS build_nxlcache_source_file_name(NXL_CACHE_NODE *pNode, UNICODE_STRING *SourceFileName)
{
	NTSTATUS Status = STATUS_SUCCESS;

	USHORT SourceFileNameLength = 0;

	do 
	{
		SourceFileNameLength = SourceFileName->Length;

		if (SourceFileNameLength > (USHORT)sizeof(pNode->SourceFileNameFastBuffer))
		{
			pNode->SourceFileName.Buffer = (WCHAR*)ExAllocatePoolWithTag(PagedPool, SourceFileNameLength, NXRMFLT_NXLCACHE_TAG);

			if (!pNode->SourceFileName.Buffer)
			{
				Status = STATUS_INSUFFICIENT_RESOURCES;
				break;
			}

			memset(pNode->SourceFileName.Buffer, 0, SourceFileNameLength);

			memcpy(pNode->SourceFileName.Buffer, SourceFileName->Buffer, SourceFileNameLength);

			pNode->SourceFileName.Length = SourceFileNameLength;
			pNode->SourceFileName.MaximumLength = SourceFileNameLength;
			pNode->ReleaseSourceFileName = TRUE;
		}
		else
		{
			pNode->SourceFileName.Buffer = pNode->SourceFileNameFastBuffer;

			memset(pNode->SourceFileName.Buffer, 0, SourceFileNameLength);

			memcpy(pNode->SourceFileName.Buffer, SourceFileName->Buffer, SourceFileNameLength);

			pNode->SourceFileName.Length = SourceFileNameLength;
			pNode->SourceFileName.MaximumLength = (USHORT)sizeof(pNode->SourceFileNameFastBuffer);
			pNode->ReleaseSourceFileName = FALSE;
		}

	} while (FALSE);

	return Status;
}
