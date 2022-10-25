using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;

namespace CommonDialog.sdk
{
    // Impl all stubs that cross c# and c++ boundaris
    class Boundary
    {
        #region LocalFiles
        [DllImport(Config.DLL_NAME, CallingConvention = CallingConvention.Cdecl,
            CharSet = CharSet.Unicode, EntryPoint = "SDWL_File_GetListNumber")]
        public static extern uint SDWL_File_GetListNumber(IntPtr hLocalFiles, out int size);

        [DllImport(Config.DLL_NAME, CallingConvention = CallingConvention.Cdecl,
            CharSet = CharSet.Unicode, EntryPoint = "SDWL_File_GetList")]
        public static extern uint SDWL_File_GetFiles(IntPtr hLocalFile, out IntPtr array, out int arraySize);

        [DllImport(Config.DLL_NAME, CallingConvention = CallingConvention.Cdecl,
            CharSet = CharSet.Unicode, EntryPoint = "SDWL_File_GetFile")]
        public static extern uint SDWL_File_GetFile(IntPtr hLocalFiles, int index, out IntPtr hNxlFile);

        [DllImport(Config.DLL_NAME, CallingConvention = CallingConvention.Cdecl,
            CharSet = CharSet.Unicode, EntryPoint = "SDWL_File_GetFile2")]
        public static extern uint SDWL_File_GetFile2(IntPtr hLocalFiles, string name, out IntPtr hNxlFile);

        [DllImport(Config.DLL_NAME, CallingConvention = CallingConvention.Cdecl,
            CharSet = CharSet.Unicode, EntryPoint = "SDWL_File_RemoveFile")]
        public static extern uint SDWL_File_RemoveFile(IntPtr hLocalFiles, IntPtr hNxlFile, out bool result);
        #endregion

        #region NxlFile
        [DllImport(Config.DLL_NAME, CallingConvention = CallingConvention.Cdecl,
            CharSet = CharSet.Unicode, EntryPoint = "SDWL_NXL_File_GetFileName")]
        public static extern uint SDWL_NXL_File_GetFileName(IntPtr hNxlFile, out string name);

        [DllImport(Config.DLL_NAME,
            CallingConvention = CallingConvention.Cdecl,
            CharSet = CharSet.Unicode,
            EntryPoint = "SDWL_NXL_File_GetTags")]
        public static extern uint SDWL_NXL_File_GetTags(IntPtr hNxlFile, out string tags);


        [DllImport(Config.DLL_NAME, CallingConvention = CallingConvention.Cdecl,
            CharSet = CharSet.Unicode, EntryPoint = "SDWL_NXL_File_GetFileSize")]
        public static extern uint SDWL_NXL_File_GetFileSize(IntPtr hNxlFile, out Int64 size);

        [DllImport(Config.DLL_NAME, CallingConvention = CallingConvention.Cdecl,
            CharSet = CharSet.Unicode, EntryPoint = "SDWL_NXL_File_IsValidNxl")]
        public static extern uint SDWL_NXL_File_IsValidNxl(IntPtr hNxlFile, out bool result);

        [DllImport(Config.DLL_NAME, CallingConvention = CallingConvention.Cdecl,
            CharSet = CharSet.Unicode, EntryPoint = "SDWL_NXL_File_GetRights")]
        public static extern uint SDWL_NXL_File_GetRights(IntPtr hNxlFile, out IntPtr pArray, out int pSize);

        [DllImport(Config.DLL_NAME,
            CallingConvention = CallingConvention.Cdecl,
            CharSet = CharSet.Unicode,
            EntryPoint = "SDWL_NXL_File_GetWaterMark")]
        public static extern uint SDWL_NXL_File_GetWaterMark(IntPtr hNxlFile, out WaterMarkInfo pWaterMark);

        [DllImport(Config.DLL_NAME,
            CallingConvention = CallingConvention.Cdecl,
            CharSet = CharSet.Unicode,
            EntryPoint = "SDWL_NXL_File_GetExpiration")]
        public static extern uint SDWL_NXL_File_GetExpiration(IntPtr hNxlFile, out Expiration pWaterMark);

        [DllImport(Config.DLL_NAME, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode, EntryPoint = "SDWL_NXL_File_CheckRights")]
        public static extern uint SDWL_NXL_File_CheckRights(IntPtr hNxlFile, int right, out bool result);

        //[DllImport(Config.DLL_NAME, 
        //    CallingConvention = CallingConvention.Cdecl, 
        //    CharSet = CharSet.Ansi, 
        //    EntryPoint = "SDWL_NXL_File_GetClassificationSetting")]
        //private static extern uint SDWL_NXL_File_GetClassificationSetting(IntPtr hNxlFile, out string result);

        [DllImport(Config.DLL_NAME,
            CallingConvention = CallingConvention.Cdecl,
            CharSet = CharSet.Unicode,
            EntryPoint = "SDWL_NXL_File_IsUploadToRMS")]
        public static extern uint SDWL_NXL_File_IsUploadToRMS(IntPtr hNxlFile, out bool result);


        [DllImport(Config.DLL_NAME,
            CallingConvention = CallingConvention.Cdecl,
            CharSet = CharSet.Unicode,
            EntryPoint = "SDWL_NXL_File_GetActivityInfo")]
        public static extern uint SDWL_NXL_File_GetActivityInfo(
            [MarshalAs(UnmanagedType.LPWStr)]string fileName, out IntPtr pArray, out int pSize);

        [DllImport(Config.DLL_NAME, CallingConvention = CallingConvention.Cdecl,
            CharSet = CharSet.Unicode,
            EntryPoint = "SDWL_NXL_File_GetAdhocWatermarkString")]
        public static extern uint SDWL_NXL_File_GetAdhocWatermarkString(IntPtr hNxlFile, out string watermark);

        #endregion

        #region User
        [DllImport(Config.DLL_NAME, CallingConvention = CallingConvention.Cdecl,
            CharSet = CharSet.Unicode, EntryPoint = "SDWL_User_GetUserName")]
        public static extern uint SDWL_User_GetUserName(IntPtr hUser, out string name);

        [DllImport(Config.DLL_NAME, CallingConvention = CallingConvention.Cdecl,
            CharSet = CharSet.Unicode, EntryPoint = "SDWL_User_GetUserEmail")]
        public static extern uint SDWL_User_GetUserEmail(IntPtr hUser, out string email);

        [DllImport(Config.DLL_NAME, CallingConvention = CallingConvention.Cdecl,
            CharSet = CharSet.Unicode, EntryPoint = "SDWL_User_GetUserType")]
        public static extern uint SDWL_User_GetUserType(IntPtr hUser, ref int type);

        [DllImport(Config.DLL_NAME, CallingConvention = CallingConvention.Cdecl,
            CharSet = CharSet.Unicode, EntryPoint = "SDWL_User_GetPasscode")]
        public static extern uint SDWL_User_GetPasscode(IntPtr hUser, out string code);

        [DllImport(Config.DLL_NAME, CallingConvention = CallingConvention.Cdecl,
         CharSet = CharSet.Unicode, EntryPoint = "SDWL_User_GetUserId")]
        public static extern uint SDWL_User_GetUserId(IntPtr hUser, out uint userId);

        [DllImport(Config.DLL_NAME, CallingConvention = CallingConvention.Cdecl,
            CharSet = CharSet.Unicode, EntryPoint = "SDWL_User_UpdateUserInfo")]
        public static extern uint SDWL_User_UpdateUserInfo(IntPtr hUser);

        [DllImport(Config.DLL_NAME, CallingConvention = CallingConvention.Cdecl,
            CharSet = CharSet.Unicode, EntryPoint = "SDWL_User_UpdateMyDriveInfo")]
        public static extern uint SDWL_User_UpdateMyDriveInfo(IntPtr hUser);

        [DllImport(Config.DLL_NAME, CallingConvention = CallingConvention.Cdecl,
            CharSet = CharSet.Unicode, EntryPoint = "SDWL_User_LogoutUser")]
        public static extern uint SDWL_User_LogoutUser(IntPtr hUser);

        [DllImport(Config.DLL_NAME, CallingConvention = CallingConvention.Cdecl,
            CharSet = CharSet.Unicode, EntryPoint = "SDWL_User_GetMyDriveInfo")]
        public static extern uint SDWL_User_GetMyDriveInfo(IntPtr hUser, ref Int64 usage, ref Int64 total);

        [DllImport(Config.DLL_NAME, CallingConvention = CallingConvention.Cdecl,
            CharSet = CharSet.Unicode, EntryPoint = "SDWL_User_GetLocalFile")]
        public static extern uint SDWL_User_GetLocalFile(IntPtr hUser, out IntPtr hLocalFile);

        [DllImport(Config.DLL_NAME, CallingConvention = CallingConvention.Cdecl,
            CharSet = CharSet.Unicode, EntryPoint = "SDWL_User_RemoveLocalFile")]
        public static extern uint SDWL_User_RemoveLocalFile(
            IntPtr hUser,
            [MarshalAs(UnmanagedType.LPWStr)] string nxlFilePath,
            out bool result);

        [DllImport(Config.DLL_NAME, CallingConvention = CallingConvention.Cdecl, EntryPoint = "SDWL_User_ProtectFile")]
        public static extern uint SDWL_User_ProtectFile(IntPtr hUser,
            [MarshalAs(UnmanagedType.LPWStr)] string path,
            int[] rights,
            int lenRights,
            WaterMarkInfo waterMark,
            Expiration expiration,
            [MarshalAs(UnmanagedType.LPWStr)]string tags,
            [MarshalAs(UnmanagedType.LPWStr)] out string outPath
            );

        [DllImport(Config.DLL_NAME, CallingConvention = CallingConvention.Cdecl,
            CharSet = CharSet.Unicode,
            EntryPoint = "SDWL_User_ProjectUploadFile")]
        public static extern uint SDWL_User_ProjectUploadFile(
            IntPtr hUser,
            int projectId,
            [MarshalAs(UnmanagedType.LPWStr)]string rmsParentFolder,
            IntPtr hNxlFile);

        [DllImport(Config.DLL_NAME, CallingConvention = CallingConvention.Cdecl,
            CharSet = CharSet.Unicode,
            EntryPoint = "SDWL_User_ProjectUploadFile2")]
        public static extern uint SDWL_User_ProjectUploadFile(
            IntPtr hUser,
            int projectId,
            [MarshalAs(UnmanagedType.LPWStr)]string rmsParentFolder,
            [MarshalAs(UnmanagedType.LPWStr)] string nxlFilePath);

        [DllImport(Config.DLL_NAME, CallingConvention = CallingConvention.Cdecl,
            CharSet = CharSet.Unicode,
            EntryPoint = "UploadEditedProjectFile")]
        public static extern uint UploadEditedProjectFile(
            IntPtr hUser,
            int projectId,
            [MarshalAs(UnmanagedType.LPWStr)]string rmsParentFolder,
            [MarshalAs(UnmanagedType.LPWStr)] string nxlFilePath);


        [DllImport(Config.DLL_NAME, CallingConvention = CallingConvention.Cdecl,
            EntryPoint = "SDWL_User_ProtectFileToProject")]
        public static extern uint SDWL_User_ProtectFileToProject(
            int projectId,
            IntPtr hUser,
            [MarshalAs(UnmanagedType.LPWStr)] string path,
            int[] rights,
            int lenRights,
            WaterMarkInfo waterMark,
            Expiration expiration,
            [MarshalAs(UnmanagedType.LPWStr)]string tags,
            [MarshalAs(UnmanagedType.LPWStr)] out string outPath
            );

        [DllImport(Config.DLL_NAME, CallingConvention = CallingConvention.Cdecl, EntryPoint = "SDWL_User_UpdateRecipients")]
        public static extern uint SDWL_User_UpdateRecipients(IntPtr hUser,
                                    IntPtr hNxlFile,
                                   [MarshalAs(UnmanagedType.LPArray, ArraySubType = UnmanagedType.LPWStr)] string[] adds,
                                   int lenAdd,
                                   [MarshalAs(UnmanagedType.LPArray, ArraySubType = UnmanagedType.LPWStr)]string[] dels,
                                   int lenDels
                                   );

        [DllImport(Config.DLL_NAME, CallingConvention = CallingConvention.Cdecl, EntryPoint = "SDWL_User_UpdateRecipients2")]
        public static extern uint SDWL_User_UpdateRecipients(IntPtr hUser,
                                   [MarshalAs(UnmanagedType.LPWStr)] string nxlFilePath,
                                   [MarshalAs(UnmanagedType.LPArray, ArraySubType = UnmanagedType.LPWStr)] string[] adds,
                                   int lenAdd,
                                   [MarshalAs(UnmanagedType.LPArray, ArraySubType = UnmanagedType.LPWStr)]string[] dels,
                                   int lenDels
                                   );


        [DllImport(Config.DLL_NAME, CallingConvention = CallingConvention.Cdecl, EntryPoint = "SDWL_User_GetRecipients")]
        public static extern uint SDWL_User_GetRecipients(IntPtr hUser,
                                    IntPtr hNxlFile,
                                    out IntPtr array, out int arraySize,
                                    out IntPtr arrayAdd, out int arraySizeAdd,
                                    out IntPtr arrayRemove, out int arraySizeRemove
                                   );

        [DllImport(Config.DLL_NAME, CallingConvention = CallingConvention.Cdecl, EntryPoint = "SDWL_User_GetRecipients2")]
        public static extern uint SDWL_User_GetRecipients(IntPtr hUser,
                                    [MarshalAs(UnmanagedType.LPWStr)] string nxlFilePath,
                                    out IntPtr array, out int arraySize,
                                    out IntPtr arrayAdd, out int arraySizeAdd,
                                    out IntPtr arrayRemove, out int arraySizeRemove
                                   );

        [DllImport(Config.DLL_NAME, CallingConvention = CallingConvention.Cdecl, EntryPoint = "SDWL_User_GetRecipents3")]
        public static extern uint SDWL_User_GetRecipents(IntPtr hUser,
                            [MarshalAs(UnmanagedType.LPWStr)] string nxlFilePath,
                            [MarshalAs(UnmanagedType.LPWStr)] out string recipents,
                            [MarshalAs(UnmanagedType.LPWStr)] out string recipentsAdd,
                            [MarshalAs(UnmanagedType.LPWStr)] out string recipentsRemove);

        [DllImport(Config.DLL_NAME, CallingConvention = CallingConvention.Cdecl, EntryPoint = "SDWL_User_UploadFile")]
        public static extern uint SDWL_User_UploadFile(IntPtr hUser, IntPtr hNxlFile);

        [DllImport(Config.DLL_NAME, CallingConvention = CallingConvention.Cdecl,
            EntryPoint = "SDWL_User_UploadMyVaultFile")]
        public static extern uint SDWL_User_UploadMyVaultFile(IntPtr hUser,
                                    [MarshalAs(UnmanagedType.LPWStr)] string nxlFilePath,
                                    [MarshalAs(UnmanagedType.LPWStr)] string sourcePath,
                                    [MarshalAs(UnmanagedType.LPWStr)] string recipents = "",
                                    [MarshalAs(UnmanagedType.LPWStr)] string comments = "");


        [DllImport(Config.DLL_NAME, CallingConvention = CallingConvention.Cdecl,
            CharSet = CharSet.Unicode, EntryPoint = "SDWL_User_OpenFile")]
        public static extern uint SDWL_User_OpenFile(IntPtr hUser,
                                    string nxlPath,
                                    out IntPtr hNxlFile
                                   );

        [DllImport(Config.DLL_NAME, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode, EntryPoint = "SDWL_User_CacheRPMFileToken")]
        public static extern uint SDWL_User_CacheRPMFileToken(IntPtr hUser,
                                    string nxlPath
                                   );

        [DllImport(Config.DLL_NAME, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode, EntryPoint = "SDWL_User_DecryptNXLFile")]
        public static extern uint SDWL_User_DecryptNXLFile(IntPtr hUser,
                                    IntPtr hNxlFile,
                                    string outPath
                                   );

        [DllImport(Config.DLL_NAME, CallingConvention = CallingConvention.Cdecl,
            CharSet = CharSet.Unicode, EntryPoint = "SDWL_User_CloseFile")]
        public static extern uint SDWL_User_CloseNxlFile(
                                    IntPtr hUser,
                                    IntPtr hNxlFile
                                   );

        [DllImport(Config.DLL_NAME, CallingConvention = CallingConvention.Cdecl,
            CharSet = CharSet.Unicode, EntryPoint = "SDWL_User_ForceCloseFile")]
        public static extern uint SDWL_User_ForceCloseFile(
                                    IntPtr hUser,
                                    [MarshalAs(UnmanagedType.LPWStr)] string nxlFilePath
                                   );


        [DllImport(Config.DLL_NAME,
            CallingConvention = CallingConvention.Cdecl,
            CharSet = CharSet.Unicode,
            EntryPoint = "SDWL_User_GetWaterMarkInfo")]
        public static extern uint SDWL_User_GetWaterMarkInfo(IntPtr hUser,
                                    out WaterMarkInfo pWaterMark
                                   );

        [DllImport(Config.DLL_NAME,
            CallingConvention = CallingConvention.Cdecl,
            EntryPoint = "SDWL_User_UploadActivityLogs")]
        public static extern uint SDWL_User_UploadActivityLogs(IntPtr hUser);

        //
        // for project section
        //
        [DllImport(Config.DLL_NAME,
            CallingConvention = CallingConvention.Cdecl,
            EntryPoint = "SDWL_User_GetProjectsInfo")]
        public static extern uint SDWL_User_GetProjectsInfo(IntPtr hUser, out IntPtr pArray, out int pSize);

        [DllImport(Config.DLL_NAME,
            CallingConvention = CallingConvention.Cdecl,
            CharSet = CharSet.Unicode,
            EntryPoint = "SDWL_User_GetListProjtects")]
        public static extern uint SDWL_User_GetListProjtects(IntPtr hUser, int pagedId, int pageSize,
            string orderBy, ProjectFilterType filter);

        [DllImport(Config.DLL_NAME,
            CallingConvention = CallingConvention.Cdecl,
            CharSet = CharSet.Unicode,
            EntryPoint = "SDWL_User_CheckProjectEnableAdhoc")]
        public static extern uint SDWL_User_CheckProjectEnableAdhoc(IntPtr hUser,
            [MarshalAs(UnmanagedType.LPWStr)]string projectTenandId,
            ref bool isEnabled
            );

        [DllImport(Config.DLL_NAME,
            CallingConvention = CallingConvention.Cdecl,
            CharSet = CharSet.Unicode,
            EntryPoint = "SDWL_User_CheckSystemBucketEnableAdhoc")]
        public static extern uint SDWL_User_CheckSystemBucketEnableAdhoc(IntPtr hUser, ref bool isEnabled);


        [DllImport(Config.DLL_NAME,
            CallingConvention = CallingConvention.Cdecl,
            CharSet = CharSet.Unicode,
            EntryPoint = "SDWL_User_CheckInPlaceProtection")]
        public static extern uint SDWL_User_CheckInPlaceProtection(IntPtr hUser,
            [MarshalAs(UnmanagedType.LPWStr)]string projectTenandId,
            ref bool deleteSource
            );

        [DllImport(Config.DLL_NAME,
            CallingConvention = CallingConvention.Cdecl,
            CharSet = CharSet.Unicode,
            EntryPoint = "SDWL_User_CheckSystemProject")]
        public static extern uint SDWL_User_CheckSystemProject(IntPtr hUser,
            [MarshalAs(UnmanagedType.LPWStr)]string projectTenandId,
            ref int sysprojectid,
            [MarshalAs(UnmanagedType.LPWStr)]out string sysProjectTenandId
            );

        [DllImport(Config.DLL_NAME,
            CallingConvention = CallingConvention.Cdecl,
            CharSet = CharSet.Unicode,
            EntryPoint = "SDWL_User_ProjectListFiles")]
        public static extern uint SDWL_User_ProjectListFiles(IntPtr hUser,
            int projectId, int pagedId,
            int pagedSize, string orderby,
            [MarshalAs(UnmanagedType.LPWStr)]string pathId,
            [MarshalAs(UnmanagedType.LPWStr)]string searchStr,
            out IntPtr pArray, out int pSize);

        [DllImport(Config.DLL_NAME,
            CharSet = CharSet.Unicode,
            CallingConvention = CallingConvention.Cdecl,
            EntryPoint = "SDWL_User_ProjectClassifacation")]
        public static extern uint SDWL_User_ProjectClassifacation(IntPtr hUser,
            [MarshalAs(UnmanagedType.LPWStr)]string tenantid,
            out IntPtr pArray,
            out int pSize);

        [DllImport(Config.DLL_NAME,
            CallingConvention = CallingConvention.Cdecl,
            CharSet = CharSet.Unicode,
            EntryPoint = "SDWL_User_ProjectDownloadFile")]
        public static extern uint SDWL_User_ProjectDownloadFile(IntPtr hUser,
            int projectId,
            [MarshalAs(UnmanagedType.LPWStr)]string pathId,
            [MarshalAs(UnmanagedType.LPWStr)]string downloadPath,
            int type,
            [MarshalAs(UnmanagedType.LPWStr)] out string outPath
            );

        [DllImport(Config.DLL_NAME,
            CallingConvention = CallingConvention.Cdecl,
            CharSet = CharSet.Unicode,
            EntryPoint = "SDWL_User_ProjectDownloadPartialFile")]
        public static extern uint SDWL_User_ProjectDownloadPartialFile(IntPtr hUser,
            int projectId,
            [MarshalAs(UnmanagedType.LPWStr)]string pathId,
            [MarshalAs(UnmanagedType.LPWStr)]string downloadPath,
            int type,
            [MarshalAs(UnmanagedType.LPWStr)] out string outPath
            );
        //
        // end project section
        //

        [DllImport(Config.DLL_NAME,
            CallingConvention = CallingConvention.Cdecl,
            CharSet = CharSet.Unicode,
            EntryPoint = "SDWL_User_GetHeartBeatInfo")]
        public static extern uint SDWL_User_GetHeartBeatInfo(IntPtr hUser);

        [DllImport(Config.DLL_NAME,
            CallingConvention = CallingConvention.Cdecl,
            CharSet = CharSet.Unicode,
            EntryPoint = "SDWL_User_GetHeartBeatFrequency")]
        public static extern uint SDWL_User_GetHeartBeatFrequency(IntPtr hUser, out Int32 nHeartBeatFrequenceSeconds);


        [DllImport(Config.DLL_NAME,
            CallingConvention = CallingConvention.Cdecl,
            CharSet = CharSet.Unicode,
            EntryPoint = "SDWL_User_ListMyVaultFiles")]
        public static extern uint SDWL_User_ListMyVaultFiles(IntPtr hUser,
                UInt32 pageId, UInt32 pageSize,
                string orderBy, string searchString,
                out IntPtr pArray, out int psize);

        [DllImport(Config.DLL_NAME,
            CallingConvention = CallingConvention.Cdecl,
            CharSet = CharSet.Unicode,
            EntryPoint = "SDWL_User_DownloadMyVaultFiles")]
        public static extern uint SDWL_User_DownloadMyVaultFiles(
            IntPtr hUser,
            [MarshalAs(UnmanagedType.LPWStr)]string rmsFilePathId,
            [MarshalAs(UnmanagedType.LPWStr)]string downloadPath,
            int type,
            [MarshalAs(UnmanagedType.LPWStr)] out string outPath
            );

        [DllImport(Config.DLL_NAME,
            CallingConvention = CallingConvention.Cdecl,
            CharSet = CharSet.Unicode,
            EntryPoint = "SDWL_User_DownloadMyVaultPartialFiles")]
        public static extern uint SDWL_User_DownloadMyVaultPartialFiles(
            IntPtr hUser,
            [MarshalAs(UnmanagedType.LPWStr)]string rmsFilePathId,
            [MarshalAs(UnmanagedType.LPWStr)]string downloadPath,
            int type,
            [MarshalAs(UnmanagedType.LPWStr)] out string outPath
            );

        [DllImport(Config.DLL_NAME,
            CallingConvention = CallingConvention.Cdecl,
            CharSet = CharSet.Unicode,
            EntryPoint = "SDWL_User_ListSharedWithMeFiles")]
        public static extern uint SDWL_User_ListSharedWithMeFiles(
            IntPtr hFile,
            int pageId, int pageSize,
            [MarshalAs(UnmanagedType.LPWStr)]string orderBy,
            [MarshalAs(UnmanagedType.LPWStr)]string searchString,
            out IntPtr pArray,
            out int pSize
            );

        [DllImport(Config.DLL_NAME,
            CallingConvention = CallingConvention.Cdecl,
            CharSet = CharSet.Unicode,
            EntryPoint = "SDWL_User_DownloadSharedWithMeFiles")]
        public static extern uint SDWL_User_DownloadSharedWithMeFiles(
            IntPtr hUser,
            string transactionId,
            string transactionCode,
            string downlaodDestLocalFolder,
            bool forViewer,
            [MarshalAs(UnmanagedType.LPWStr)] out string outPath
            );

        [DllImport(Config.DLL_NAME,
            CallingConvention = CallingConvention.Cdecl,
            CharSet = CharSet.Unicode,
            EntryPoint = "SDWL_User_DownloadSharedWithMePartialFiles")]
        public static extern uint SDWL_User_DownloadSharedWithMePartialFiles(
            IntPtr hUser,
            string transactionId,
            string transactionCode,
            string downlaodDestLocalFolder,
            bool forViewer,
            [MarshalAs(UnmanagedType.LPWStr)] out string outPath
            );

        [DllImport(Config.DLL_NAME,
            CallingConvention = CallingConvention.Cdecl,
            CharSet = CharSet.Unicode,
            EntryPoint = "SDWL_User_AddNxlFileLog")]
        public static extern uint SDWL_User_AddNxlFileLog(
            IntPtr hUser,
            string filePath,
            int Oper,
            bool isAllow);
        [DllImport(Config.DLL_NAME,
            CallingConvention = CallingConvention.Cdecl,
            CharSet = CharSet.Unicode,
            EntryPoint = "SDWL_User_EvaulateNxlFileRights")]
        public static extern uint SDWL_User_EvaulateNxlFileRights(
            IntPtr hUser,
            string filePath,
            out IntPtr pArray,
            out int pSize,
            out WaterMarkInfo pWaterMark);

        [DllImport(Config.DLL_NAME,
            CallingConvention = CallingConvention.Cdecl,
            CharSet = CharSet.Unicode,
            EntryPoint = "SDWL_User_GetPreference")]
        public static extern uint SDWL_User_GetPreference(
            IntPtr hUser,
            out Expiration expiration,
            out string watermark
            );

        [DllImport(Config.DLL_NAME,
            CallingConvention = CallingConvention.Cdecl,
            CharSet = CharSet.Unicode,
            EntryPoint = "SDWL_User_UpdatePreference")]
        public static extern uint SDWL_User_UpdatePreference(
            IntPtr hUser,
            Expiration expiration,
            [MarshalAs(UnmanagedType.LPWStr)] string watermark
            );

        [DllImport(Config.DLL_NAME,
            CallingConvention = CallingConvention.Cdecl,
            CharSet = CharSet.Unicode,
            EntryPoint = "SDWL_User_GetNxlFileFingerPrint")]
        public static extern uint SDWL_User_GetNxlFileFingerPrint(
            IntPtr hUser,
            string nxlPath,
            out User.InternalFingerPrint fingerprint);

        [DllImport(Config.DLL_NAME,
            CallingConvention = CallingConvention.Cdecl,
            CharSet = CharSet.Unicode,
            EntryPoint = "SDWL_User_GetNxlFileTagsWithoutToken")]
        public static extern uint SDWL_User_GetNxlFileTagsWithoutToken(
            IntPtr hUser,
            string nxlPath,
            out string tags);

        [DllImport(Config.DLL_NAME,
            CallingConvention = CallingConvention.Cdecl,
            CharSet = CharSet.Unicode,
            EntryPoint = "SDWL_User_UpdateNxlFileRights")]
        public static extern uint SDWL_User_UpdateNxlFileRights(
            IntPtr hUser,
            [MarshalAs(UnmanagedType.LPWStr)] string nxlPath,
            int[] rights,
            int lenRights,
            WaterMarkInfo waterMark, Expiration expiration,
            [MarshalAs(UnmanagedType.LPWStr)]string tags);

        [DllImport(Config.DLL_NAME,
            CallingConvention = CallingConvention.Cdecl,
            CharSet = CharSet.Unicode,
            EntryPoint = "SDWL_User_UpdateProjectNxlFileRights")]
        public static extern uint SDWL_User_UpdateProjectNxlFileRights(
            IntPtr hUser,
            [MarshalAs(UnmanagedType.LPWStr)]string nxlFilePath,
            UInt32 projectId,
            [MarshalAs(UnmanagedType.LPWStr)]string fileName,
            [MarshalAs(UnmanagedType.LPWStr)]string parentPathId,
            int[] rights, int rightsLength,
            WaterMarkInfo waterMark, Expiration expiration,
            [MarshalAs(UnmanagedType.LPWStr)]string tags);

        [DllImport(Config.DLL_NAME,
            CallingConvention = CallingConvention.Cdecl,
            CharSet = CharSet.Unicode,
            EntryPoint = "SDWL_User_GetMyVaultFileMetaData")]
        public static extern uint SDWL_User_GetMyVaultFileMetaData(
            IntPtr hUser,
            [MarshalAs(UnmanagedType.LPWStr)] string nxlPath,
            [MarshalAs(UnmanagedType.LPWStr)] string pathId,
            out User.InternalMyVaultMetaData metaData);

        [DllImport(Config.DLL_NAME,
            CallingConvention = CallingConvention.Cdecl,
            CharSet = CharSet.Unicode,
            EntryPoint = "SDWL_User_MyVaultShareFile")]
        public static extern uint SDWL_User_MyVaultShareFile(
            IntPtr hUser,
            [MarshalAs(UnmanagedType.LPWStr)]string nxlLocalPath,
            [MarshalAs(UnmanagedType.LPWStr)]string recipents,
            [MarshalAs(UnmanagedType.LPWStr)]string repositoryId,
            [MarshalAs(UnmanagedType.LPWStr)]string fileName,
            [MarshalAs(UnmanagedType.LPWStr)]string filePathId,
            [MarshalAs(UnmanagedType.LPWStr)]string filePath,
            [MarshalAs(UnmanagedType.LPWStr)]string comments);

        [DllImport(Config.DLL_NAME,
            CallingConvention = CallingConvention.Cdecl,
            CharSet = CharSet.Unicode,
            EntryPoint = "SDWL_User_SharedWithMeReshareFile")]
        public static extern uint SDWL_User_SharedWitheMeReshareFile(
            IntPtr pUser,
            [MarshalAs(UnmanagedType.LPWStr)] string transactionId,
            [MarshalAs(UnmanagedType.LPWStr)] string transactionCode,
            [MarshalAs(UnmanagedType.LPWStr)] string emails);

        [DllImport(Config.DLL_NAME,
            CallingConvention = CallingConvention.Cdecl,
            CharSet = CharSet.Unicode,
            EntryPoint = "SDWL_User_ResetSourcePath")]
        public static extern uint SDWL_User_ResetSourcePath(
            IntPtr pUser,
            [MarshalAs(UnmanagedType.LPWStr)]string nxlFilePath,
            [MarshalAs(UnmanagedType.LPWStr)]string sourcePath);

        [DllImport(Config.DLL_NAME,
            CallingConvention = CallingConvention.Cdecl,
            CharSet = CharSet.Unicode,
            EntryPoint = "SDWL_User_GetFileRightsFromCentralPoliciesByTenant")]
        public static extern uint SDWL_User_GetFileRightsFromCentralPolicyByTenant(
            IntPtr pUser,
            [MarshalAs(UnmanagedType.LPWStr)]string tenantName,
            [MarshalAs(UnmanagedType.LPWStr)]string tags,
            out IntPtr pArray,
            out int pArrSize);

        [DllImport(Config.DLL_NAME,
            CallingConvention = CallingConvention.Cdecl,
            CharSet = CharSet.Unicode,
            EntryPoint = "SDWL_User_GetFileRightsFromCentralPolicyByProjectId")]
        public static extern uint SDWL_User_GetFileRightsFromCentralPolicyByProjectID(
            IntPtr pUser,
            UInt32 projectId,
            [MarshalAs(UnmanagedType.LPWStr)]string tags,
            out IntPtr pArray,
            out int pArrSize);

        #endregion

        #region Tenant
        [DllImport(Config.DLL_NAME, CallingConvention = CallingConvention.Cdecl,
            CharSet = CharSet.Unicode, EntryPoint = "SDWL_Tenant_GetTenant")]
        public static extern uint SDK_SDWL_GetTenant(
            IntPtr hTenant, out string Tenant);

        [DllImport(Config.DLL_NAME, CallingConvention = CallingConvention.Cdecl,
            CharSet = CharSet.Unicode, EntryPoint = "SDWL_Tenant_GetRouterURL")]
        public static extern uint SDK_SDWL_GetRouterURL(
            IntPtr hTenant, out string RouterURL);

        [DllImport(Config.DLL_NAME, CallingConvention = CallingConvention.Cdecl,
            CharSet = CharSet.Unicode, EntryPoint = "SDWL_Tenant_GetRMSURL")]
        public static extern uint SDK_SDWL_GetRMSURL(
            IntPtr hTenant, out string RMSURL);

        [DllImport(Config.DLL_NAME, CallingConvention = CallingConvention.Cdecl,
            CharSet = CharSet.Unicode, EntryPoint = "SDWL_Tenant_ReleaseTenant")]
        public static extern uint SDK_SDWL_ReleaseTenant(
            IntPtr hTenant);

        #endregion

        #region API

        [DllImport(Config.DLL_NAME, CallingConvention = CallingConvention.Cdecl,
         CharSet = CharSet.Unicode, EntryPoint = "SdkLibInit")]
        public static extern void SdkLibInit();

        [DllImport(Config.DLL_NAME, CallingConvention = CallingConvention.Cdecl,
            CharSet = CharSet.Unicode, EntryPoint = "SdkLibCleanup")]
        public static extern void SdkLibCleanup();

        [DllImport(Config.DLL_NAME, CallingConvention = CallingConvention.Cdecl,
            CharSet = CharSet.Unicode, EntryPoint = "GetSDKVersion")]
        public static extern uint GetSDKVersion();

        [DllImport(Config.DLL_NAME, CallingConvention = CallingConvention.Cdecl,
            CharSet = CharSet.Unicode, EntryPoint = "CreateSDKSession")]
        public static extern uint CreateSDKSession(string TempPath, out IntPtr SessionHandle);

        [DllImport(Config.DLL_NAME, CallingConvention = CallingConvention.Cdecl,
            CharSet = CharSet.Unicode, EntryPoint = "GetCurrentLoggedInUser")]
        public static extern uint GetCurrentLoggedInUser(out IntPtr SessionHandle, out IntPtr UserHandle);

        [DllImport(Config.DLL_NAME, CallingConvention = CallingConvention.Cdecl,
        CharSet = CharSet.Unicode, EntryPoint = "WaitInstanceInitFinish")]
        public static extern uint WaitInstanceInitFinish();

        #endregion

        #region Session
        [DllImport(Config.DLL_NAME, CallingConvention = CallingConvention.Cdecl,
            CharSet = CharSet.Unicode, EntryPoint = "DeleteSDKSession")]
        public static extern uint DeleteSDKSession(IntPtr SessionHandle);

        [DllImport(Config.DLL_NAME, CallingConvention = CallingConvention.Cdecl,
            CharSet = CharSet.Unicode, EntryPoint = "SDWL_Session_Initialize")]
        public static extern uint SDK_Initialize(IntPtr hSession, string Router, string Tenant);

        [DllImport(Config.DLL_NAME, CallingConvention = CallingConvention.Cdecl,
            CharSet = CharSet.Unicode, EntryPoint = "SDWL_Session_Initialize2")]
        public static extern uint SDK_Initialize(IntPtr hSession, string WorkingFolder, string Router, string Tenant);

        [DllImport(Config.DLL_NAME, CallingConvention = CallingConvention.Cdecl,
            CharSet = CharSet.Unicode, EntryPoint = "SDWL_Session_SaveSession")]
        public static extern uint SDK_SaveSession(IntPtr hSession, string folder);

        //NXSDK_API DWORD GetCurrentTenant(HANDLE hSession, HANDLE* hTenant);
        [DllImport(Config.DLL_NAME, CallingConvention = CallingConvention.Cdecl,
            CharSet = CharSet.Unicode, EntryPoint = "SDWL_Session_GetCurrentTenant")]
        public static extern uint SDK_GetCurrentTenant(IntPtr hSession, out IntPtr hTenant);

        [DllImport(Config.DLL_NAME, CallingConvention = CallingConvention.Cdecl,
            CharSet = CharSet.Unicode, EntryPoint = "SDWL_Session_SetLoginRequest")]
        public static extern uint SDK_SetLoginRequest(IntPtr hSession, string loginstr, string security, out IntPtr hUser);

        [DllImport(Config.DLL_NAME, CallingConvention = CallingConvention.Cdecl,
            CharSet = CharSet.Unicode, EntryPoint = "SDWL_Session_GetLoginParams")]
        public static extern uint SDWL_Session_GetLoginParams(IntPtr hSession, out string loginUrl, out IntPtr Cookies, out int size);

        [DllImport(Config.DLL_NAME, CallingConvention = CallingConvention.Cdecl,
            CharSet = CharSet.Unicode, EntryPoint = "SDWL_Session_GetLoginUser")]
        public static extern uint SDWL_Session_GetLoginUser(IntPtr hSession,
            string UserEmail, string PassCode, out IntPtr hUser);

        #endregion

        #region RPM
        // add RPM features

        [DllImport(Config.DLL_NAME,
            CallingConvention = CallingConvention.Cdecl,
            CharSet = CharSet.Unicode,
            EntryPoint = "SDWL_RPM_GetFileRights")]
        public static extern uint SDWL_RPM_GetFileRights(
            IntPtr hSession,
            string filePath,
            out IntPtr pArray,
            out int pSize,
            out WaterMarkInfo pWaterMark, int option = 1);

        [DllImport(Config.DLL_NAME, CallingConvention = CallingConvention.Cdecl,
            CharSet = CharSet.Unicode, EntryPoint = "SDWL_RPM_ReadFileTags")]
        public static extern uint SDWL_RPM_ReadFileTags(IntPtr hSession,
            [MarshalAs(UnmanagedType.LPWStr)] string filePath,
            [MarshalAs(UnmanagedType.LPWStr)] out string rpmPath);


        // User RPM folder
        [DllImport(Config.DLL_NAME, CallingConvention = CallingConvention.Cdecl,
            CharSet = CharSet.Unicode, EntryPoint = "SDWL_RPM_GetUserRPMFolder")]
        public static extern uint SDWL_RPM_GetUserRPMFolder(IntPtr hSession, [MarshalAs(UnmanagedType.LPWStr)] out string rpmPath);

        [DllImport(Config.DLL_NAME, CallingConvention = CallingConvention.Cdecl,
            CharSet = CharSet.Unicode, EntryPoint = "SDWL_RPM_AddUserRPMFolder")]
        public static extern uint SDWL_RPM_AddUserRPMFolder(
            IntPtr hSession,
            [MarshalAs(UnmanagedType.LPWStr)] string path, uint option = 0);

        [DllImport(Config.DLL_NAME, CallingConvention = CallingConvention.Cdecl,
            CharSet = CharSet.Unicode, EntryPoint = "SDWL_RPM_RemoveUserRPMFolder")]
        public static extern uint SDWL_RPM_RemoveUserRPMFolder(
            IntPtr hSession,
            [MarshalAs(UnmanagedType.LPWStr)] string path, bool bForce = false);
        // --- End User RPM folder.

        [DllImport(Config.DLL_NAME, CallingConvention = CallingConvention.Cdecl,
            CharSet = CharSet.Unicode, EntryPoint = "SDWL_RPM_IsRPMDriverExist")]
        public static extern uint SDWL_RPM_IsRPMDriverExist(IntPtr hSession, out bool IsExist);

        [DllImport(Config.DLL_NAME, CallingConvention = CallingConvention.Cdecl,
            CharSet = CharSet.Unicode, EntryPoint = "SDWL_RPM_AddRPMDir")]
        public static extern uint SDWL_RPM_AddRPMDir(
            IntPtr hSession,
            [MarshalAs(UnmanagedType.LPWStr)] string path, int option, int foldertype);

        [DllImport(Config.DLL_NAME, CallingConvention = CallingConvention.Cdecl,
            CharSet = CharSet.Unicode, EntryPoint = "SDWL_RPM_RemoveRPMDir")]
        public static extern uint SDWL_RPM_RemoveRPMDir(
            IntPtr hSession,
            [MarshalAs(UnmanagedType.LPWStr)] string path, bool bForce);

        [DllImport(Config.DLL_NAME, CallingConvention = CallingConvention.Cdecl,
            CharSet = CharSet.Unicode, EntryPoint = "SDWL_RPM_EditFile")]
        public static extern uint SDWL_RPM_EditFile(
            IntPtr hSession,
            [MarshalAs(UnmanagedType.LPWStr)] string path,
            out string outPath);

        [DllImport(Config.DLL_NAME, CallingConvention = CallingConvention.Cdecl,
            CharSet = CharSet.Unicode, EntryPoint = "SDWL_RPM_DeleteFile")]
        public static extern uint SDWL_RPM_DeleteFile(
            IntPtr hSession,
            [MarshalAs(UnmanagedType.LPWStr)] string path);


        [DllImport(Config.DLL_NAME, CallingConvention = CallingConvention.Cdecl,
            CharSet = CharSet.Unicode, EntryPoint = "SDWL_RPM_RegisterApp")]
        public static extern uint SDWL_RPM_RegisterApp(
            IntPtr hSession,
            [MarshalAs(UnmanagedType.LPWStr)] string path);

        [DllImport(Config.DLL_NAME, CallingConvention = CallingConvention.Cdecl,
            CharSet = CharSet.Unicode, EntryPoint = "SDWL_RPM_NotifyRMXStatus")]
        public static extern uint SDWL_RPM_NotifyRMXStatus(
            IntPtr hSession,
            bool running);

        [DllImport(Config.DLL_NAME, CallingConvention = CallingConvention.Cdecl,
            CharSet = CharSet.Unicode, EntryPoint = "SDWL_RPM_UnregisterApp")]
        public static extern uint SDWL_RPM_UnregisterApp(
            IntPtr hSession,
            [MarshalAs(UnmanagedType.LPWStr)] string path);

        [DllImport(Config.DLL_NAME, CallingConvention = CallingConvention.Cdecl,
            CharSet = CharSet.Unicode, EntryPoint = "SDWL_RPM_AddTrustedProcess")]
        public static extern uint SDWL_RPM_AddTrustedProcess(
            IntPtr hSession,
            int pid);

        [DllImport(Config.DLL_NAME, CallingConvention = CallingConvention.Cdecl,
            CharSet = CharSet.Unicode, EntryPoint = "SDWL_RPM_RemoveTrustedProcess")]
        public static extern uint SDWL_RPM_RemoveTrustedProcess(
            IntPtr hSession,
            int pid);

        [DllImport(Config.DLL_NAME, CallingConvention = CallingConvention.Cdecl,
            CharSet = CharSet.Unicode, EntryPoint = "SDWL_RPM_IsSafeFolder")]
        public static extern uint SDWL_RPM_IsSafeFolder(
            IntPtr hSession,
            [MarshalAs(UnmanagedType.LPWStr)] string path,
            ref bool result);

        [DllImport(Config.DLL_NAME, CallingConvention = CallingConvention.Cdecl,
      CharSet = CharSet.Unicode, EntryPoint = "SDWL_RPM_GetFileStatus")]
        public static extern uint SDWL_RPM_GetFileStatus(
          IntPtr hSession,
          [MarshalAs(UnmanagedType.LPWStr)] string path,
          out int dirstatus,
          out bool filestatus);

        [DllImport(Config.DLL_NAME, CallingConvention = CallingConvention.Cdecl,
        CharSet = CharSet.Unicode, EntryPoint = "SDWL_RPM_RequestLogin")]
        public static extern uint SDWL_RPM_RequestLogin(
            IntPtr hSession,
            [MarshalAs(UnmanagedType.LPWStr)] string callbackCmd,
            [MarshalAs(UnmanagedType.LPWStr)] string callbackCmdPara = "");

        [DllImport(Config.DLL_NAME, CallingConvention = CallingConvention.Cdecl,
        CharSet = CharSet.Unicode, EntryPoint = "SDWL_RPM_RequestLogout")]
        public static extern uint SDWL_RPM_RequestLogout(
            IntPtr hSession,
            out bool isAllow,
            uint option = 0);

        [DllImport(Config.DLL_NAME, CallingConvention = CallingConvention.Cdecl,
        CharSet = CharSet.Unicode, EntryPoint = "SDWL_RPM_NotifyMessage")]
        public static extern uint SDWL_RPM_NotifyMessage(
            IntPtr hSession,
            [MarshalAs(UnmanagedType.LPWStr)] string app,
            [MarshalAs(UnmanagedType.LPWStr)] string target,
            [MarshalAs(UnmanagedType.LPWStr)] string message,
            uint msgtype = 0,
            [MarshalAs(UnmanagedType.LPWStr)] string operation = "",
            uint result = 0,
            uint fileStatus = 0
            );

        #endregion 

        // callback used by SDWL_SYSHELPER_RegKeyChangeMonitorSynced,
        // when c++ world deteack {regValueBeDeleted} had been deleted, notify c#
        public delegate void RegChangedCallback(
            [MarshalAs(UnmanagedType.LPWStr)]string regValueBeDeleted);

        #region SysHelper
        [DllImport(Config.DLL_NAME, CallingConvention = CallingConvention.Cdecl,
            CharSet = CharSet.Unicode, EntryPoint = "SDWL_SYSHELPER_MonitorRegValueDeleted")]
        public static extern uint SDWL_SYSHELPER_MonitorRegValueDeleted(
            [MarshalAs(UnmanagedType.LPWStr)] string rmpPath,
            RegChangedCallback callback);
        #endregion

        #region Offlice plugin

        //bool IsPluginWell(const wchar_t* wszAppType, const wchar_t* wszPlatform)

        [DllImport(Config.DLL_NAME, CallingConvention = CallingConvention.Cdecl,
            CharSet = CharSet.Unicode, EntryPoint = "IsPluginWell")]
        public static extern bool IsPluginWell(
            string wszAppType,
            string wszPlatform);

        #endregion
    }
}
