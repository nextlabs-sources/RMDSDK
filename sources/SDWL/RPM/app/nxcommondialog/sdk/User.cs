using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;
using CommonDialog.sdk.helper;

namespace CommonDialog.sdk
{
    class User
    {
        private IntPtr hUser;


        public User(IntPtr hUser)
        {
            this.hUser = hUser;
        }

        public uint UserId
        {
            get
            {
                uint userId;
                Boundary.SDWL_User_GetUserId(hUser, out userId);
                return userId;
            }
        }

        public string Name
        {
            get
            {
                string name;
                Boundary.SDWL_User_GetUserName(hUser, out name);
                return name;
            }
        }

        public string Email
        {
            get

            {
                string email;
                Boundary.SDWL_User_GetUserEmail(hUser, out email);
                return email;
            }
        }

        public string PassCode
        {
            get
            {
                string code;
                Boundary.SDWL_User_GetPasscode(hUser, out code);
                return code;
            }
        }

        public UserType UserType
        {
            get
            {
                int type = -1;
                Boundary.SDWL_User_GetUserType(hUser, ref type);
                return (UserType)type;
            }
        }

        public void UpdateUserInfo()
        {
            uint rt = Boundary.SDWL_User_UpdateUserInfo(hUser);
            if (rt != 0)
            {
                ExceptionFactory.BuildThenThrow("SDWL_User_UpdateUserInfo", rt);
            }
        }

        public void UpdateMyDriveInfo()
        {
            uint rt = Boundary.SDWL_User_UpdateMyDriveInfo(hUser);
            if (rt != 0)
            {
                ExceptionFactory.BuildThenThrow("SDWL_User_UpdateMyDriveInfo", rt);
            }
        }

        public void GetMyDriveInfo(ref Int64 usage, ref Int64 total)
        {
            uint rt = Boundary.SDWL_User_GetMyDriveInfo(hUser, ref usage, ref total);
            if (rt != 0)
            {
                ExceptionFactory.BuildThenThrow("SDWL_User_GetMyDriveInfo", rt);
            }
        }

        public void Logout()
        {
            uint rt = Boundary.SDWL_User_LogoutUser(hUser);
            // by osmond, for the bug 49212, when user logout, we just ignore any error code

            //if (rt != 0)
            //{
            //    string msg = String.Format("exception for SDWL_User_LogoutUser,err={0}", rt);
            //    Console.WriteLine(msg);
            //    throw new Exception(msg);
            //}
        }

        //public LocalFiles GetLocalFiles()
        //{
        //    IntPtr f = IntPtr.Zero;
        //    uint rt = Boundary.SDWL_User_GetLocalFile(hUser, out f);
        //    if (rt != 0 || f == IntPtr.Zero)
        //    {
        //        ExceptionFactory.BuildThenThrow("SDWL_User_GetLocalFile", rt);
        //    }

        //    return new LocalFiles(f);
        //}

        // this is a wrapepr of rmsdk::getLocalFileManager().remove()
        public bool RemoveLocalGeneratedFiles(string file)
        {
            bool result;
            uint rt = Boundary.SDWL_User_RemoveLocalFile(hUser, file, out result);
            if (rt != 0)
            {
                ExceptionFactory.BuildThenThrow("RemoveLocalGeneratedFiles", rt);
            }
            return result;
        }


        public string ProtectFile(string path, List<FileRights> rights,
            WaterMarkInfo waterMark, Expiration expiration, UserSelectTags tags)
        {
            List<int> r = new List<int>(rights.Count);

            foreach (var i in rights)
            {
                r.Add((int)i);
            }
            string outpath;
            uint rt = Boundary.SDWL_User_ProtectFile(hUser, path, r.ToArray(), r.Count, waterMark, expiration, tags.ToJsonString(), out outpath);
            if (rt != 0)
            {
                ExceptionFactory.BuildThenThrow("SDWL_User_ProtectFile", rt);
            }
            return outpath;
        }

        public string ProtectFileToProject(int projectId, string path, List<FileRights> rights,
            WaterMarkInfo waterMark, Expiration expiration, UserSelectTags tags)
        {
            List<int> r = new List<int>(rights.Count);

            foreach (var i in rights)
            {
                r.Add((int)i);
            }
            string outpath;
            uint rt = Boundary.SDWL_User_ProtectFileToProject(
                projectId, hUser, path, r.ToArray(),
                r.Count, waterMark, expiration,
                tags.ToJsonString(), out outpath);
            if (rt != 0)
            {
                ExceptionFactory.BuildThenThrow("SDWL_User_ProtectFileToProject", rt);
            }
            return outpath;
        }

        public string ProtectFileToSystemProject(int projectId, string plain, List<FileRights> rights,
            WaterMarkInfo waterMark, Expiration expiration, UserSelectTags tags)
        {
            return ProtectFileToProject(projectId, plain, rights, waterMark, expiration, tags);
        }


        //public void UpdateRecipients(NxlFile file, List<string> addEmails, List<string> delEmails)
        //{
        //    uint rt = Boundary.SDWL_User_UpdateRecipients(hUser, file.Handle,
        //        addEmails.ToArray(), addEmails.Count,
        //        delEmails.ToArray(), delEmails.Count);
        //    if (rt != 0)
        //    {
        //        ExceptionFactory.BuildThenThrow("SDWL_User_UpdateRecipients", rt);
        //    }
        //}

        // by osmond, add new api
        public void UpdateRecipients(string nxlFilePath, List<string> addEmails, List<string> delEmails)
        {
            uint rt = Boundary.SDWL_User_UpdateRecipients(hUser, nxlFilePath,
                addEmails.ToArray(), addEmails.Count,
                delEmails.ToArray(), delEmails.Count);
            if (rt != 0)
            {
                ExceptionFactory.BuildThenThrow("SDWL_User_UpdateRecipients", rt);
            }
        }

        //public void GetRecipients(NxlFile file, out string[] emails, out string[] addEmails, out string[] removeEmails)
        //{

        //    emails = new string[0];
        //    addEmails = new string[0];
        //    removeEmails = new string[0];
        //    //uint rt = 
        //    IntPtr pE;
        //    int sizeArray;
        //    IntPtr pEA;
        //    int sizeArrayAdd;
        //    IntPtr pER;
        //    int sizeArrayRemove;

        //    uint rt = Boundary.SDWL_User_GetRecipients(hUser, file.Handle,
        //                                    out pE, out sizeArray,
        //                                    out pEA, out sizeArrayAdd,
        //                                    out pER, out sizeArrayRemove);
        //    if (rt != 0)
        //    {
        //        return;
        //    }

        //    if (sizeArray > 0)
        //    {
        //        emails = new string[sizeArray];
        //        IntPtr[] ps = new IntPtr[sizeArray];
        //        Marshal.Copy(pE, ps, 0, sizeArray);
        //        for (int i = 0; i < sizeArray; i++)
        //        {
        //            emails[i] = Marshal.PtrToStringAnsi(ps[i]);
        //            Marshal.FreeCoTaskMem(ps[i]);
        //        }
        //        Marshal.FreeCoTaskMem(pE);
        //    }

        //    if (sizeArrayAdd > 0)
        //    {
        //        addEmails = new string[sizeArray];
        //        IntPtr[] ps = new IntPtr[sizeArray];
        //        Marshal.Copy(pEA, ps, 0, sizeArray);
        //        for (int i = 0; i < sizeArray; i++)
        //        {
        //            addEmails[i] = Marshal.PtrToStringAnsi(ps[i]);
        //            Marshal.FreeCoTaskMem(ps[i]);
        //        }
        //        Marshal.FreeCoTaskMem(pEA);
        //    }

        //    if (sizeArrayRemove > 0)
        //    {
        //        removeEmails = new string[sizeArray];
        //        IntPtr[] ps = new IntPtr[sizeArray];
        //        Marshal.Copy(pER, ps, 0, sizeArray);
        //        for (int i = 0; i < sizeArray; i++)
        //        {
        //            removeEmails[i] = Marshal.PtrToStringAnsi(ps[i]);
        //            Marshal.FreeCoTaskMem(ps[i]);
        //        }
        //        Marshal.FreeCoTaskMem(pER);
        //    }

        //}

        public void GetRecipients(string nxlFilePath, out string[] emails, out string[] addEmails, out string[] removeEmails)
        {

            emails = new string[0];
            addEmails = new string[0];
            removeEmails = new string[0];
            //uint rt = 
            IntPtr pE;
            int sizeArray;
            IntPtr pEA;
            int sizeArrayAdd;
            IntPtr pER;
            int sizeArrayRemove;

            uint rt = Boundary.SDWL_User_GetRecipients(hUser, nxlFilePath,
                                            out pE, out sizeArray,
                                            out pEA, out sizeArrayAdd,
                                            out pER, out sizeArrayRemove);
            if (rt != 0)
            {
                return;
            }

            if (sizeArray > 0)
            {
                emails = new string[sizeArray];
                IntPtr[] ps = new IntPtr[sizeArray];
                Marshal.Copy(pE, ps, 0, sizeArray);
                for (int i = 0; i < sizeArray; i++)
                {
                    emails[i] = Marshal.PtrToStringAnsi(ps[i]);
                    Marshal.FreeCoTaskMem(ps[i]);
                }
                Marshal.FreeCoTaskMem(pE);
            }

            if (sizeArrayAdd > 0)
            {
                addEmails = new string[sizeArray];
                IntPtr[] ps = new IntPtr[sizeArray];
                Marshal.Copy(pEA, ps, 0, sizeArray);
                for (int i = 0; i < sizeArray; i++)
                {
                    addEmails[i] = Marshal.PtrToStringAnsi(ps[i]);
                    Marshal.FreeCoTaskMem(ps[i]);
                }
                Marshal.FreeCoTaskMem(pEA);
            }

            if (sizeArrayRemove > 0)
            {
                removeEmails = new string[sizeArray];
                IntPtr[] ps = new IntPtr[sizeArray];
                Marshal.Copy(pER, ps, 0, sizeArray);
                for (int i = 0; i < sizeArray; i++)
                {
                    removeEmails[i] = Marshal.PtrToStringAnsi(ps[i]);
                    Marshal.FreeCoTaskMem(ps[i]);
                }
                Marshal.FreeCoTaskMem(pER);
            }

        }

        public void GetRecipents2(string nxlFilePath, out string[] emails, out string[] addEmails, out string[] removeEmails)
        {
            emails = new string[0];
            addEmails = new string[0];
            removeEmails = new string[0];

            var rt = Boundary.SDWL_User_GetRecipents(
                hUser, nxlFilePath,
                out string recipents,
                out string recipentsAdd,
                out string recipentsRemove);

            if (rt != 0)
            {
                ExceptionFactory.BuildThenThrow("GetRecipents2", rt);
            }

            emails = Utils.ParseRecipents(recipents).ToArray();
            addEmails = Utils.ParseRecipents(recipentsAdd).ToArray();
            removeEmails = Utils.ParseRecipents(recipentsRemove).ToArray();
        }

        public void UploadMyVaultFile(string nxlPath, string sourcePath, string recipents = "", string comments = "")
        {
            // SDK uploadFile interface is using comma if contains multiple emails -- fix bug 55808.
            string emails = recipents.Contains(";") ? recipents.Replace(';', ',') : recipents;

            uint rt = Boundary.SDWL_User_UploadMyVaultFile(hUser, nxlPath, sourcePath, emails, comments);
            if (rt != 0)
            {
                ExceptionFactory.BuildThenThrow("SDWL_User_UploadFile", rt,
                    RmSdkExceptionDomain.Rest_MyVault, RmSdkRestMethodKind.Upload);
            }
        }

        public MyVaultFileInfo[] ListMyVaultFiles()
        {
            IntPtr pArray;
            int size;
            var rt = Boundary.SDWL_User_ListMyVaultFiles(
                hUser, 1, 1000, "fileName", "", out pArray, out size);
            if (rt != 0 || size < 0)
            {
                ExceptionFactory.BuildThenThrow("SDWL_User_ListMyVaultFiles", rt,
                    RmSdkExceptionDomain.Rest_MyVault, RmSdkRestMethodKind.List);
            }
            if (size == 0)
            {
                return new MyVaultFileInfo[0];
            }

            // parse com mem to extract the array
            // marshal unmarshal
            MyVaultFileInfo[] pInfo = new MyVaultFileInfo[size];
            int structSize = Marshal.SizeOf(typeof(MyVaultFileInfo));
            IntPtr cur = pArray;
            for (int i = 0; i < size; i++)
            {
                pInfo[i] = (MyVaultFileInfo)Marshal.PtrToStructure(cur, typeof(MyVaultFileInfo));
                cur += structSize;
            }
            Marshal.FreeCoTaskMem(pArray);
            return pInfo;
        }

        public void DownloadMyVaultFile(string rmsPathId, ref string downlaodPath, DownlaodMyVaultFileType type)
        {
            string outpath;
            var rt = Boundary.SDWL_User_DownloadMyVaultFiles(
                hUser, rmsPathId, downlaodPath, (int)type, out outpath);
            if (rt != 0)
            {
                ExceptionFactory.BuildThenThrow("SDWL_User_DownloadMyVaultFiles", rt,
                    RmSdkExceptionDomain.Rest_MyVault, RmSdkRestMethodKind.Download);
            }
            downlaodPath = outpath;
        }

        public void DownloadMyVaultPartialFile(string rmsPathId, ref string downlaodPath, DownlaodMyVaultFileType type)
        {
            string outpath;
            var rt = Boundary.SDWL_User_DownloadMyVaultPartialFiles(
                hUser, rmsPathId, downlaodPath, (int)type, out outpath);
            if (rt != 0)
            {
                ExceptionFactory.BuildThenThrow("SDWL_User_DownloadMyVaultPartialFiles", rt,
                    RmSdkExceptionDomain.Rest_MyVault, RmSdkRestMethodKind.Download);
            }
            downlaodPath = outpath;
        }

        public void CacheRPMFileToken(string NxlFilePath)
        {
            uint rt = Boundary.SDWL_User_CacheRPMFileToken(hUser, NxlFilePath);
            if (rt != 0)
            {
            }

            return;
        }

        // Defined Excpetion:
        //      RmSdkInsufficientRightsException - you dont have any rights
        //public NxlFile OpenNxlFile(string NxlFilePath)
        //{
        //    IntPtr hFile = IntPtr.Zero;
        //    uint rt = Boundary.SDWL_User_OpenFile(hUser, NxlFilePath, out hFile);
        //    if (rt != 0)
        //    {
        //        ExceptionFactory.BuildThenThrow("SDWL_User_OpenFile", rt);
        //    }
        //    return new NxlFile(hFile);
        //}

        //public void CloseNxlFile(NxlFile File)
        //{
        //    uint rt = Boundary.SDWL_User_CloseNxlFile(hUser, File.Handle);
        //    if (rt != 0)
        //    {
        //        ExceptionFactory.BuildThenThrow("SDWL_User_CloseNxlFile", rt);
        //    }
        //}

        // this is a work around ,to force sdk to release file handle,
        public void ForceCloseFile_NoThrow(string nxlFilePath)
        {
            try
            {
                Boundary.SDWL_User_ForceCloseFile(hUser, nxlFilePath);
            }
            catch (Exception)
            {
            }
        }

        // this is a work around ,to force sdk to release file handle,
        //public void CloseNxlFile_NoThrow(string file)
        //{
        //    try
        //    {
        //        CloseNxlFile(OpenNxlFile(file));
        //    }
        //    catch(Exception ignored)
        //    {
        //        SkydrmLocalApp.Singleton.Log.Warn(ignored.Message, ignored);
        //    }
        //}


        //public void DecryptNxlFile(NxlFile File, string OutputPath)
        //{
        //    uint rt = Boundary.SDWL_User_DecryptNXLFile(hUser, File.Handle, OutputPath);
        //    if (rt != 0)
        //    {
        //        ExceptionFactory.BuildThenThrow("SDWL_User_DecryptNXLFile", rt);
        //    }
        //}

        public void UploadActivityLogs()
        {
            uint rt = Boundary.SDWL_User_UploadActivityLogs(hUser);
            if (rt != 0)
            {
                ExceptionFactory.BuildThenThrow("SDWL_User_UploadActivityLogs", rt);
            }
        }

        public ProjectInfo[] UpdateProjectInfo()
        {
            // as sdk required, sync first and then to get the latest info
            uint rt = Boundary.SDWL_User_GetListProjtects(hUser, 1, 1000, "name", ProjectFilterType.All);
            if (rt != 0)
            {
                ExceptionFactory.BuildThenThrow("SDWL_User_GetListProjtects", rt);
            }

            IntPtr pArray;
            int size;
            rt = Boundary.SDWL_User_GetProjectsInfo(hUser, out pArray, out size);
            if (rt != 0 || size < 0)
            {
                ExceptionFactory.BuildThenThrow("SDWL_User_GetProjectsInfo", rt);
            }
            if (size == 0)
            {
                return new ProjectInfo[0];
            }
            // marshal & unmarshal
            ProjectInfo[] pInfo = new ProjectInfo[size];
            int structSize = Marshal.SizeOf(typeof(ProjectInfo));
            IntPtr cur = pArray;
            for (int i = 0; i < size; i++)
            {
                pInfo[i] = (ProjectInfo)Marshal.PtrToStructure(cur, typeof(ProjectInfo));
                cur += structSize;
            }
            Marshal.FreeCoTaskMem(pArray);

            return pInfo;
        }

        public bool IsEnabledAdhocForProject(string ProjectTenandId)
        {
            bool isEnabled = false;
            // call sdk
            uint rt = Boundary.SDWL_User_CheckProjectEnableAdhoc(hUser, ProjectTenandId, ref isEnabled);
            if (rt != 0)
            {
                ExceptionFactory.BuildThenThrow("IsEnabledAdhocForProject", rt);
            }

            return isEnabled;
        }

        public bool IsEnabledAdhocForSystemBucket()
        {
            bool isEnabled = false;
            // call sdk
            uint rt = Boundary.SDWL_User_CheckSystemBucketEnableAdhoc(hUser, ref isEnabled);
            if (rt != 0)
            {
                ExceptionFactory.BuildThenThrow("IsEnabledAdhocForProject", rt);
            }

            return isEnabled;
        }


        public int GetSystemProjectId()
        {
            int sysprojectid = 0;
            string sysprojecttenantid = "";
            // call sdk
            uint rt = Boundary.SDWL_User_CheckSystemProject(hUser, "", ref sysprojectid, out sysprojecttenantid);
            if (rt != 0)
            {
                ExceptionFactory.BuildThenThrow("GetSystemProjectId", rt);
            }

            return sysprojectid;
        }

        public string GetSystemProjectTenantId()
        {
            int sysprojectid = 0;
            string sysprojecttenantid = "";
            // call sdk
            uint rt = Boundary.SDWL_User_CheckSystemProject(hUser, "", ref sysprojectid, out sysprojecttenantid);
            if (rt != 0)
            {
                Trace.WriteLine(" -----> Error: Failed to get tenant preference.");
                ExceptionFactory.BuildThenThrow("GetSystemProjectId", rt);
            }

            return sysprojecttenantid;
        }

        public bool GetIsDeleteSource(string ProjectTenandId = "")
        {
            bool isDeleteSource = false;
            // call sdk
            uint rt = Boundary.SDWL_User_CheckInPlaceProtection(hUser, "", ref isDeleteSource);
            if (rt != 0)
            {
                ExceptionFactory.BuildThenThrow("GetIsDeleteSource", rt);
            }

            return isDeleteSource;
        }

        public ProjectFileInfo[] ListProjectsFiles(int projectId, string pathId)
        {
            IntPtr pArray;
            int size;
            uint rt = Boundary.SDWL_User_ProjectListFiles(hUser, projectId, 1, 1000, "name", pathId, "", out pArray, out size);
            if (rt != 0 || size < 0)
            {
                ExceptionFactory.BuildThenThrow("SDWL_User_ProjectListFiles", rt);
            }
            if (size == 0)
            {
                return new ProjectFileInfo[0];
            }
            // marshal & unmarshal
            ProjectFileInfo[] pInfo = new ProjectFileInfo[size];
            int structSize = Marshal.SizeOf(typeof(ProjectFileInfo));
            IntPtr cur = pArray;
            for (int i = 0; i < size; i++)
            {
                pInfo[i] = (ProjectFileInfo)Marshal.PtrToStructure(cur, typeof(ProjectFileInfo));
                cur += structSize;
            }
            Marshal.FreeCoTaskMem(pArray);

            return pInfo;
        }

        //public void UploadProjectFile(int projectId, string rmsParentFolder, NxlFile nxlFile)
        //{
        //    uint rt = Boundary.SDWL_User_ProjectUploadFile(hUser, projectId, rmsParentFolder, nxlFile.Handle);
        //    if (rt != 0)
        //    {
        //        ExceptionFactory.BuildThenThrow("SDWL_User_ProjectUploadFile", rt);
        //    }
        //}

        public void UploadProjectFile(int projectId, string rmsParentFolder, string nxlFilePath)
        {
            uint rt = Boundary.SDWL_User_ProjectUploadFile(hUser, projectId, rmsParentFolder, nxlFilePath);
            if (rt != 0)
            {
                ExceptionFactory.BuildThenThrow("SDWL_User_ProjectUploadFile", rt);
            }
        }

        public void UploadEditedProjectFile(int projectId, string rmsParentFolder, string nxlFilePath)
        {
            uint rt = Boundary.UploadEditedProjectFile(hUser, projectId, rmsParentFolder, nxlFilePath);
            if (rt != 0)
            {
                ExceptionFactory.BuildThenThrow("SDWL_User_ProjectUploadFile", rt);
            }
        }


        [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Unicode, Pack = 8)]
        private struct Internal_ProjectClassification
        {
            public string name;
            public int isMultiSelect;
            public int isMandatory;
            public string labels;
            public string defaults;
        }

        public ProjectClassification[] GetProjectClassification(string tenantid)
        {
            IntPtr pArray;
            int size;
            uint rt = Boundary.SDWL_User_ProjectClassifacation(hUser, tenantid, out pArray, out size);
            if (rt != 0 || size < 0)
            {
                Trace.WriteLine(" -----> Error: Failed to get tenant classification.");
                ExceptionFactory.BuildThenThrow("SDWL_User_ProjectClassifacation", rt);
            }
            if (size == 0)
            {
                return new ProjectClassification[0];
            }

            Internal_ProjectClassification[] pPC = new Internal_ProjectClassification[size];
            int structSize = Marshal.SizeOf(typeof(Internal_ProjectClassification));
            IntPtr cur = pArray;
            for (int i = 0; i < size; i++)
            {
                pPC[i] = (Internal_ProjectClassification)Marshal.PtrToStructure(cur, typeof(Internal_ProjectClassification));
                cur += structSize;
            }
            Marshal.FreeCoTaskMem(pArray);

            //pPC to Outer Struct;
            ProjectClassification[] o = new ProjectClassification[size];
            for (int i = 0; i < size; i++)
            {
                o[i].name = pPC[i].name;
                o[i].isMultiSelect = pPC[i].isMultiSelect == 1;
                o[i].isMandatory = pPC[i].isMandatory == 1;
                o[i].labels = new Dictionary<String, bool>();
                //labels and defautls
                string[] l = pPC[i].labels.Split(new char[] { ';' });
                string[] d = pPC[i].defaults.Split(new char[] { ';' });
                for (int j = 0; j < l.Length; j++)
                {
                    if (l[j].Length > 0)
                    {
                        o[i].labels.Add(l[j], d[j].Equals("1"));
                    }
                }
            }
            return o;
        }

        // for win use, we only supprot offline , not the view, so set bViewOnly == false
        public void DownlaodProjectFile(int projectId, string pathId, ref string destFolder,
            ProjectFileDownloadType type = ProjectFileDownloadType.ForOffline)
        {
            string outpath;
            uint rt = Boundary.SDWL_User_ProjectDownloadFile(
                hUser, projectId, pathId, destFolder, (int)type, out outpath);
            if (rt != 0)
            {
                ExceptionFactory.BuildThenThrow("SDWL_User_ProjectDownloadFile", rt);
            }

            destFolder = outpath;
        }

        public void DownlaodProjectPartialFile(int projectId, string pathId, ref string destFolder, UInt16 type = 1)
        {
            string outpath;
            uint rt = Boundary.SDWL_User_ProjectDownloadPartialFile(
                hUser, projectId, pathId, destFolder, type, out outpath);
            if (rt != 0)
            {
                ExceptionFactory.BuildThenThrow("SDWL_User_ProjectDownloadPartialFile", rt);
            }

            destFolder = outpath;
        }

        public SharedWithMeFileInfo[] ListSharedWithMeFile()
        {
            IntPtr pArray;
            int size;
            uint rt = Boundary.SDWL_User_ListSharedWithMeFiles(hUser,
                1, 1000, "name", "", out pArray, out size);
            if (rt != 0 || size < 0)
            {
                ExceptionFactory.BuildThenThrow("SDWL_User_ListSharedWithMeFiles", rt,
                    RmSdkExceptionDomain.Rest_MySharedWithMe, RmSdkRestMethodKind.List);
            }
            if (size == 0)
            {
                return new SharedWithMeFileInfo[0];
            }

            // received COM encapsuled mem and convert to c# array
            SharedWithMeFileInfo[] info = new SharedWithMeFileInfo[size];
            int structSize = Marshal.SizeOf(typeof(SharedWithMeFileInfo));
            IntPtr cur = pArray;
            for (int i = 0; i < size; i++)
            {
                info[i] = (SharedWithMeFileInfo)Marshal.PtrToStructure(cur, typeof(SharedWithMeFileInfo));
                cur += structSize;
            }
            Marshal.FreeCoTaskMem(pArray);

            return info;
        }

        public void DownLoadSharedWithMeFile(string transactionId, string transactionCode,
            ref string DestLocalFodler, bool isForViewOnly = true)
        {
            string outpath;
            uint rt = Boundary.SDWL_User_DownloadSharedWithMeFiles(hUser,
                transactionId, transactionCode, DestLocalFodler, isForViewOnly, out outpath);
            if (rt != 0)
            {
                ExceptionFactory.BuildThenThrow("SDWL_User_DownloadSharedWithMeFiles", rt,
                    RmSdkExceptionDomain.Rest_MySharedWithMe, RmSdkRestMethodKind.Download);
            }
            DestLocalFodler = outpath;
        }

        public void DownLoadSharedWithMePartialFile(string transactionId, string transactionCode,
            ref string DestLocalFodler)
        {
            string outpath;
            uint rt = Boundary.SDWL_User_DownloadSharedWithMePartialFiles(hUser,
                transactionId, transactionCode, DestLocalFodler, true, out outpath);
            if (rt != 0)
            {
                ExceptionFactory.BuildThenThrow("SDWL_User_DownloadSharedWithMePartialFiles", rt,
                    RmSdkExceptionDomain.Rest_MySharedWithMe, RmSdkRestMethodKind.Download);
            }
            DestLocalFodler = outpath;
        }

        public void SyncHeartBeatInfo(out WaterMarkInfo waterMark, out Int32 heartbeatFrequenceSeconds)
        {
            uint rt = 0;
            rt = Boundary.SDWL_User_GetHeartBeatInfo(hUser);
            if (rt != 0)
            {
                ExceptionFactory.BuildThenThrow("SDWL_User_GetHeartBeatInfo", rt);
            }

            rt = Boundary.SDWL_User_GetWaterMarkInfo(hUser, out waterMark);
            if (rt != 0)
            {
                ExceptionFactory.BuildThenThrow("SDWL_User_GetWaterMarkInfo", rt);
            }

            rt = Boundary.SDWL_User_GetHeartBeatFrequency(hUser, out heartbeatFrequenceSeconds);
            if (rt != 0)
            {
                ExceptionFactory.BuildThenThrow("SDWL_User_GetHeartBeatFrequency", rt);
            }

        }

        public void AddLog(string nxlFilePath, NxlOpLog operation, bool isAllow)
        {
            uint rt = 0;
            rt = Boundary.SDWL_User_AddNxlFileLog(
                hUser, nxlFilePath, (int)operation, isAllow);
            if (rt != 0)
            {
                ExceptionFactory.BuildThenThrow("SDWL_User_AddNxlFileLog", rt);
            }
        }

        public void GetPreference(out Expiration expiration, out string watermark)
        {
            Expiration theExpiration;
            theExpiration.type = ExpiryType.NEVER_EXPIRE;
            theExpiration.Start = 0;
            theExpiration.End = 0;
            string theWatermark;

            var rt = Boundary.SDWL_User_GetPreference(hUser, out theExpiration, out theWatermark);
            if (rt != 0)
            {
                Trace.WriteLine(" -----> Error: Failed to call GetUserPreference.");
                ExceptionFactory.BuildThenThrow("SDWL_User_GetPreference", rt);
            }

            //rt
            expiration = theExpiration;
            watermark = theWatermark;
        }

        public void SetPreference(Expiration expiration, string watermark)
        {
            var rt = Boundary.SDWL_User_UpdatePreference(hUser, expiration, watermark);
            if (rt != 0)
            {
                ExceptionFactory.BuildThenThrow("SDWL_User_GetPreference", rt);
            }
        }

        [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Unicode, Pack = 8)]
        public struct InternalFingerPrint
        {
            public string name;
            public string localPath;
            public Int64 size;

            public Int64 created;
            public Int64 modified;

            public Int64 isOwner;

            public Int64 isFromMyVault;

            public Int64 isFromPorject;
            public Int64 isFromSystemBucket;
            public Int64 projectId;

            public Int64 isByAdHoc;
            public Int64 isByCentrolPolicy;

            public string tags;
            public Expiration expiration;
            public string adhocWatermark;
            public Int64 rights;

            public Int64 hasAdminRights;
            public string duid;
        }

        /// <summary>
        ///     may throw InsufficientRightsException, you can not touch this file
        ///         
        /// </summary>
        /// <param name="nxlpath"></param>
        /// <returns></returns>
        /// 
        public NxlFileFingerPrint GetNxlFileFingerPrint(string nxlpath)
        {
            //
            // Now comment out this line in order folder to make sdk can be shared link.
            //
            //SkydrmLocalApp.Singleton.Log.Info("GetNxlFileFingerPrint:" + nxlpath);

            InternalFingerPrint fp;
            var rt = Boundary.SDWL_User_GetNxlFileFingerPrint(hUser, nxlpath, out fp);
            if (rt != 0)
            {
                ExceptionFactory.BuildThenThrow("GetNxlFileFingerPrint", rt);
            }
            return Utils.Convert(fp);
        }

        public Dictionary<string, List<string>> GetNxlTagsWithoutToken(string nxlpath)
        {
            string tags;
            var rt = Boundary.SDWL_User_GetNxlFileTagsWithoutToken(hUser, nxlpath, out tags);
            if (rt != 0)
            {
                ExceptionFactory.BuildThenThrow("GetNxlFileFingerPrint", rt);
            }
            return Utils.ParseClassificationTag(tags);
        }

        public bool UpdateNxlFileRights(string nxlPath,
            List<FileRights> rights,
            WaterMarkInfo watermark, Expiration expiration,
            UserSelectTags tags)
        {
            // prepare for adhoc-rights.
            List<int> r = new List<int>(rights == null ? 0 : rights.Count);
            if (rights != null)
            {
                foreach (var i in rights)
                {
                    r.Add((int)i);
                }
            }

            // pass tags as json string.
            string jTags = tags.ToJsonString();

            var rt = Boundary.SDWL_User_UpdateNxlFileRights(
                hUser,
                nxlPath,
                r.ToArray(), r.Count,
                watermark, expiration,
                jTags);

            if (rt != 0)
            {
                ExceptionFactory.BuildThenThrow("UpdateNxlFileRights", rt);
            }

            return true;
        }

        public bool UpdateProjectNxlFileRights(string nxlLocalPath, UInt32 projectId, string fileName, string parentPathId,
            List<FileRights> rights, WaterMarkInfo waterMark, Expiration expiration, UserSelectTags tags)
        {
            // sanity check.
            if (string.IsNullOrEmpty(nxlLocalPath))
            {
                return false;
            }

            if (string.IsNullOrEmpty(fileName) || string.IsNullOrEmpty(parentPathId))
            {
                // require necessary params.
                return false;
            }

            // prepare for adhoc-rights.
            List<int> r = new List<int>(rights == null ? 0 : rights.Count);
            if (rights != null)
            {
                foreach (var i in rights)
                {
                    r.Add((int)i);
                }
            }

            // pass tags as json string.
            string jTags = tags.ToJsonString();

            var rt = Boundary.SDWL_User_UpdateProjectNxlFileRights(
                hUser, nxlLocalPath, projectId, fileName, parentPathId,
                r.ToArray(), r.Count,
                waterMark, expiration, jTags);

            if (rt != 0)
            {
                ExceptionFactory.BuildThenThrow("UpdateProjectNxlFileRights", rt);
            }

            return true;
        }

        [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Unicode, Pack = 8)]
        public struct InternalMyVaultMetaData
        {
            public string name;
            public string fileLink;

            public Int64 lastModified;
            public Int64 protectedOn;
            public Int64 sharedOn;
            public Int64 isShared;
            public Int64 isDeleted;
            public Int64 isRevoked;
            public Int64 protectionType;
            public Int64 isOwner;
            public Int64 isNxl;

            public string recipents;
            public string pathDisplay;
            public string pathId;
            public string tags;

            public Expiration expiration;
        }

        public MyVaultMetaData GetMyVaultFileMetaData(string nxlPath, string pathId)
        {
            var rt = Boundary.SDWL_User_GetMyVaultFileMetaData(hUser, nxlPath, pathId,
                out InternalMyVaultMetaData md);

            if (rt != 0)
            {
                ExceptionFactory.BuildThenThrow("GetMyVaultFileMetaData", rt);
            }

            return Utils.Convert(md);
        }

        public bool MyVaultShareFile(string nxlLocalPath, string[] recipents,
            string repositoryId, string fileName,
            string filePathId, string filePath,
            string comments)
        {
            if (string.IsNullOrEmpty(nxlLocalPath))
            {
                return false;
            }
            if (recipents == null || recipents.Length == 0)
            {
                return false;
            }

            StringBuilder recipentsBuilder = new StringBuilder();
            for (int i = 0; i < recipents.Length; i++)
            {
                var e = recipents[i];
                // filter out empty email.
                if (string.IsNullOrEmpty(e))
                {
                    continue;
                }
                recipentsBuilder.Append(e);
                if (i != recipents.Length - 1)
                {
                    recipentsBuilder.Append(",");
                }
            }

            if (string.IsNullOrEmpty(recipentsBuilder.ToString()))
            {
                // empty recipents recieved.
                return false;
            }

            var rt = Boundary.SDWL_User_MyVaultShareFile(hUser, nxlLocalPath,
                recipentsBuilder.ToString(), repositoryId,
                fileName, filePathId, filePath,
                comments);

            if (rt != 0)
            {
                ExceptionFactory.BuildThenThrow("MyVaultShareFile", rt);
            }

            return true;
        }

        public bool SharedWithMeReshareFile(string transactionId, string transactionCode, string[] emails)
        {
            if (string.IsNullOrEmpty(transactionId) || string.IsNullOrEmpty(transactionCode))
            {
                // params are needed.
                return false;
            }
            if (emails == null || emails.Length == 0)
            {
                // no share target exists at all.
                return false;
            }

            StringBuilder emailsEncapulsed = new StringBuilder();
            int size = emails.Length;
            for (int i = 0; i < size; i++)
            {
                emailsEncapulsed.Append(emails[i]);
                if (i != size - 1)
                {
                    // need take comma as each email's sperator if there is one more emails.
                    emailsEncapulsed.Append(",");
                }
            }

            var rt = Boundary.SDWL_User_SharedWitheMeReshareFile(
                hUser,
                transactionId, transactionCode, emailsEncapulsed.ToString());

            if (rt != 0)
            {
                ExceptionFactory.BuildThenThrow("SharedWithMeReshareFile", rt);
            }

            return true;
        }

        public bool ProjectNxlFileShare2PersonResetSourcePath(string nxlFilePath, string sourcePath)
        {
            if (string.IsNullOrEmpty(nxlFilePath) || string.IsNullOrEmpty(sourcePath))
            {
                return false;
            }

            var rt = Boundary.SDWL_User_ResetSourcePath(hUser, nxlFilePath, sourcePath);

            if (rt != 0)
            {
                ExceptionFactory.BuildThenThrow("ProjectNxlFileShare2PersonResetSourcePath", rt);
            }

            return true;
        }

        public void GetFileRightsFromCentralPolicyByTenant(string tenantName, UserSelectTags tags, out Dictionary<FileRights, List<WaterMarkInfo>> rightsAndWatermarks)
        {
            rightsAndWatermarks = new Dictionary<FileRights, List<WaterMarkInfo>>();

            string rawTags = tags.ToJsonString();

            IntPtr pArray;
            int pArrSize;
            var rt = Boundary.SDWL_User_GetFileRightsFromCentralPolicyByTenant(hUser,
                tenantName, rawTags,
                out pArray, out pArrSize);

            if (rt != 0)
            {
                ExceptionFactory.BuildThenThrow("GetFileRightsFromCentralPolicyByTenant", rt);
            }

            if (pArrSize > 0)
            {
                IntPtr cur = pArray;
                int structSize = Marshal.SizeOf(typeof(InternalCentralRights));
                for (int i = 0; i < pArrSize; i++)
                {
                    InternalCentralRights rights = (InternalCentralRights)Marshal.PtrToStructure(cur, typeof(InternalCentralRights));

                    Int64 wmSize = rights.wmSize;
                    if (wmSize > 0)
                    {
                        IntPtr wmCur = rights.waterMarks;
                        if (wmCur == IntPtr.Zero)
                        {
                            return;
                        }
                        int wmStructSize = Marshal.SizeOf(typeof(WaterMarkInfo));

                        List<WaterMarkInfo> waterMarks = new List<WaterMarkInfo>();
                        for (int j = 0; j < wmSize; j++)
                        {
                            WaterMarkInfo waterMarkInfo = (WaterMarkInfo)Marshal.PtrToStructure(wmCur, typeof(WaterMarkInfo));
                            waterMarks.Add(waterMarkInfo);
                            wmCur += wmStructSize;
                        }
                        rightsAndWatermarks.Add((FileRights)rights.rights, waterMarks);

                        //Release WaterMarkInfo arr in com mem.
                        Marshal.FreeCoTaskMem(rights.waterMarks);
                    }
                    else
                    {
                        rightsAndWatermarks.Add((FileRights)rights.rights, null);
                    }
                    cur += structSize;
                }

                //Release InternalCentralRights arr in com mem.
                Marshal.FreeCoTaskMem(pArray);
            }
        }

        public void GetFileRightsFromCentalPolicyByProjectId(int projectId, UserSelectTags tags, out Dictionary<FileRights, List<WaterMarkInfo>> rightsAndWatermarks)
        {
            rightsAndWatermarks = new Dictionary<FileRights, List<WaterMarkInfo>>();

            if (tags == null)
            {
                throw new ArgumentNullException("UserSelectTags is null when GetFileRightsFromCentalPolicyByProjectId");
            }

            string rawTags = tags.ToJsonString();
            IntPtr pArray;
            int pArrSize;
            var rt = Boundary.SDWL_User_GetFileRightsFromCentralPolicyByProjectID(hUser,
                (UInt32)projectId, rawTags,
                out pArray, out pArrSize);

            if (rt != 0)
            {
                ExceptionFactory.BuildThenThrow("GetFileRightsFromCentalPolicyByProjectId", rt);
            }

            if (pArrSize > 0)
            {
                IntPtr cur = pArray;
                int structSize = Marshal.SizeOf(typeof(InternalCentralRights));
                for (int i = 0; i < pArrSize; i++)
                {
                    InternalCentralRights rights = (InternalCentralRights)Marshal.PtrToStructure(cur, typeof(InternalCentralRights));

                    Int64 wmSize = rights.wmSize;
                    if (wmSize > 0)
                    {
                        IntPtr wmCur = rights.waterMarks;
                        if (wmCur == IntPtr.Zero)
                        {
                            return;
                        }
                        int wmStructSize = Marshal.SizeOf(typeof(WaterMarkInfo));

                        List<WaterMarkInfo> waterMarks = new List<WaterMarkInfo>();
                        for (int j = 0; j < wmSize; j++)
                        {
                            WaterMarkInfo waterMarkInfo = (WaterMarkInfo)Marshal.PtrToStructure(wmCur, typeof(WaterMarkInfo));
                            waterMarks.Add(waterMarkInfo);
                            wmCur += wmStructSize;
                        }
                        rightsAndWatermarks.Add((FileRights)rights.rights, waterMarks);

                        //Release WaterMarkInfo arr in com mem.
                        Marshal.FreeCoTaskMem(rights.waterMarks);
                    }
                    else
                    {
                        rightsAndWatermarks.Add((FileRights)rights.rights, null);
                    }
                    cur += structSize;
                }

                //Release InternalCentralRights arr in com mem.
                Marshal.FreeCoTaskMem(pArray);
            }
        }
    }
}
