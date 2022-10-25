using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;

namespace CommonDialog.sdk
{
    class Session
    {
        private IntPtr hSession;
        public Session(IntPtr hSession)
        {
            this.hSession = hSession;
        }

        public Session(IntPtr hSession, User user)
        {
            this.hSession = hSession;
            this.user = user;
        }

        ~Session()
        {
            DeleteSession();
        }


        //session need to hold the hUser
        private User user = null;

        public IntPtr Handle { get { return hSession; } }

        public User User { get { return user; } }

        // used to release res
        public void DeleteSession()
        {
            if (this.hSession == IntPtr.Zero)
            {
                return;
            }
            DeleteSession(this);
            this.hSession = IntPtr.Zero;
        }

        //public void Initialize(string Router, string Tenant)
        //{
        //    SDK_Initialize(Handle, Router, Tenant);
        //}

        public void Initialize(string WorkingFolder, string Router, string Tenant)
        {
            uint rt = Boundary.SDK_Initialize(
                Handle, WorkingFolder, Router, Tenant);
            if (0 != rt)
            {
                // init may failed,  like: user try to change another server
                ExceptionFactory.BuildThenThrow("Initialize", rt);
            }
        }

        public void SaveSession(string Folder)
        {
            Boundary.SDK_SaveSession(Handle, Folder);
        }

        public void GetLogingParams(out string loginURL, out Dictionary<string, string> values)
        {
            values = new Dictionary<string, string>();
            int size;
            IntPtr pcookies = IntPtr.Zero;
            uint rt = Boundary.SDWL_Session_GetLoginParams(
                Handle, out loginURL, out pcookies, out size);
            if (rt != 0)
            {
                ExceptionFactory.BuildThenThrow("GetLogingParams", rt);
            }
            // handle datas from c++
            if (size <= 0)
            {
                return;
            }
            // convert all values to Dictionary format
            // c# c+= inter_op
            // copy non-managed array into manged arrars
            IntPtr pp = pcookies;
            Cookie[] cookies = new Cookie[size];
            for (int i = 0; i < size; i++)
            {
                // extract each k,v from Inptr[i]         
                cookies[i] = (Cookie)Marshal.PtrToStructure(pp, typeof(Cookie));
                pp += Marshal.SizeOf(typeof(Cookie));
            }
            Marshal.FreeCoTaskMem(pcookies);

            // fill out param values
            foreach (var c in cookies)
            {
                if (values.ContainsKey(c.key))
                {
                    values.Remove(c.key);
                }
                values.Add(c.key, c.value);
            }

        }

        public Tenant GetCurrentTenant()
        {
            IntPtr hTenant = IntPtr.Zero;
            uint rt = Boundary.SDK_GetCurrentTenant(hSession, out hTenant);
            if (rt != 0)
            {
                ExceptionFactory.BuildThenThrow("GetCurrentTenant", rt);
            }
            return new Tenant(hTenant);
        }

        // This used for Service manager
        public void SetLoginRequest(string loginstr)
        {
            IntPtr hUser = IntPtr.Zero;
            string security = "{6829b159-b9bb-42fc-af19-4a6af3c9fcf6}";
            uint rt = Boundary.SDK_SetLoginRequest(hSession, loginstr, security, out hUser);
            if (rt != 0)
            {
                ExceptionFactory.BuildThenThrow("SetLoginRequest", rt);
            }
            this.user = new User(hUser);

        }

        // This used for service manager to recover user.
        public bool RecoverUser(string email, string passcode)
        {
            try
            {
                // sanity check
                if (email == null || email.Length < 5)
                {
                    throw new Exception();
                }
                if (passcode == null || passcode.Length < 5)
                {
                    throw new Exception();
                }

                IntPtr hUser = IntPtr.Zero;
                uint rt = Boundary.SDWL_Session_GetLoginUser(
                    hSession, email, passcode, out hUser);
                if (rt != 0 || hUser == IntPtr.Zero)
                {
                    ExceptionFactory.BuildThenThrow("RecoverUser", rt);
                }
                this.user = new User(hUser);

                return true;
            }
            catch (Exception)
            {
            }

            return false;
        }


        private static void DeleteSession(Session session)
        {
            uint rt = Boundary.DeleteSDKSession(session.Handle);
            if (rt != 0)
            {
                //ExceptionFactory.BuildThenThrow("DeleteSession", rt);
            }
        }

        [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Unicode)]
        private struct Cookie
        {
            public string key;
            public string value;

        }

        public void RPMGetFileRights(string filePath, out List<FileRights> rights, out WaterMarkInfo watermark)
        {
            rights = new List<FileRights>();
            watermark = new WaterMarkInfo()
            {
                fontColor = "",
                fontName = "",
                text = "",
                fontSize = 0,
                repeat = 0,
                rotation = 0,
                transparency = 0
            };

            IntPtr addr = IntPtr.Zero;
            int size = 0;

            // option		0: get user rights on file; 1: get file rights set in NXL header if ad-hoc file
            var rt = Boundary.SDWL_RPM_GetFileRights(hSession, filePath, out addr, out size, out watermark, 1);
            if (rt != 0)
            {
                Trace.WriteLine(" -----> Error: Failed to get file rights.");
                ExceptionFactory.BuildThenThrow("SDWL_RPM_GetRights", rt);
            }

            if (size < 0)
            {
                return;
            }

            IntPtr pp = addr;

            for (int i = 0; i < size; i++)
            {
                int r = Marshal.ReadInt32(pp);
                rights.Add((FileRights)r);
                pp += sizeof(FileRights);
            }

        }

        public string RPMReadFileTags(string filePath)
        {
            string tags;
            var rt = Boundary.SDWL_RPM_ReadFileTags(hSession, filePath, out tags);
            if (rt != 0)
            {
                Trace.WriteLine(" -----> Error: Failed to read the protected file tags.");
                ExceptionFactory.BuildThenThrow("SDWL_RPM_ReadFileTags", rt);
            }

            return tags;
        }

        // User RPM Folder
        public string GetUserRPMFolder()
        {
            string rpmPath;
            var rt = Boundary.SDWL_RPM_GetUserRPMFolder(hSession, out rpmPath);
            if (rt != 0)
            {
                ExceptionFactory.BuildThenThrow("SDWL_RPM_GetUserRPMFolder", rt);
            }

            return rpmPath;
        }

        public void AddUserRPMFolder(string path, uint option = 0)
        {
            var rt = Boundary.SDWL_RPM_AddUserRPMFolder(hSession, path, option);
            if (rt != 0)
            {
                ExceptionFactory.BuildThenThrow("AddUserRPMFolder", rt);
            }
        }

        public void RemoveUserRPMFolder(string path, bool bForce = false)
        {
            var rt = Boundary.SDWL_RPM_RemoveUserRPMFolder(hSession, path, bForce);
            if (rt != 0)
            {
                ExceptionFactory.BuildThenThrow("RemoveUserRPMFolder", rt);
            }
        }

        public bool RPM_IsDriverExist()
        {
            bool isExist;
            var rt = Boundary.SDWL_RPM_IsRPMDriverExist(
                hSession, out isExist);
            if (rt != 0)
            {
                ExceptionFactory.BuildThenThrow("IsRPMDriverExist", rt);
            }
            return isExist;
        }

        public void RPM_AddDir(string dir)
        {
            var rt = Boundary.SDWL_RPM_AddRPMDir(Handle, dir, 0, 1);
            if (rt != 0)
            {
                ExceptionFactory.BuildThenThrow("AddRPMDir", rt);
            }
        }

        public void RPM_RemoveDir(string dir)
        {
            var rt = Boundary.SDWL_RPM_RemoveRPMDir(Handle, dir, false);
            if (rt != 0)
            {
                ExceptionFactory.BuildThenThrow("RemoveRPMDir", rt);
            }
        }


        /**
         rpm only handle nxl file, pass the nxl as @param srcPath
         anything is ok, path @returnParam from an RPM Folder,
         other steps depend on @returnParam
         for example:
            office process, use it as docoment to edit    
         */
        public string RPM_EditFile(string srcPath)
        {
            string file;
            var rt = Boundary.SDWL_RPM_EditFile(hSession, srcPath, out file);
            if (rt != 0)
            {
                ExceptionFactory.BuildThenThrow("RPMEditFile", rt);
            }
            return file;
        }


        public void RPM_DeleteFile(string srcPath)
        {
            var rt = Boundary.SDWL_RPM_DeleteFile(hSession, srcPath);
            if (rt != 0)
            {
                //  ExceptionFactory.BuildThenThrow("RPM_DeleteFile", rt);
            }
        }


        public void RPM_RegisterApp(string appPath)
        {
            var rt = Boundary.SDWL_RPM_RegisterApp(hSession, appPath);
            if (rt != 0)
            {
                //  ExceptionFactory.BuildThenThrow("SDWL_RPM_RegisterApp", rt);
            }
        }

        public void RPM_UnregisterApp(string appPath)
        {
            var rt = Boundary.SDWL_RPM_UnregisterApp(hSession, appPath);
            if (rt != 0)
            {
                //  ExceptionFactory.BuildThenThrow("SDWL_RPM_RegisterApp", rt);
            }
        }

        public bool RMP_AddTrustedProcess(int pid)
        {
            bool result = true;
            var rt = Boundary.SDWL_RPM_AddTrustedProcess(hSession, pid);
            if (rt != 0)
            {
                result = false;
                //  ExceptionFactory.BuildThenThrow("RMP_AddTrustedProcess", rt);
            }
            return result;
        }

        public void RMP_RemoveTrustedProcess(int pid)
        {
            var rt = Boundary.SDWL_RPM_RemoveTrustedProcess(hSession, pid);
            if (rt != 0)
            {
                //  ExceptionFactory.BuildThenThrow("RMP_RemoveTrustedProcess", rt);
            }
        }

        public bool RMP_IsSafeFolder(string path)
        {
            bool result = false;
            var rt = Boundary.SDWL_RPM_IsSafeFolder(hSession, path, ref result);
            if (rt != 0)
            {
                // RMP ignore exception
                //  ExceptionFactory.BuildThenThrow("SDWL_RPM_IsSafeFolder", rt);
            }
            return result;
        }

        public bool SDWL_RPM_GetFileStatus(string path, out int dirstatus, out bool filestatus)
        {
            bool result = true;

            var rt = Boundary.SDWL_RPM_GetFileStatus(hSession, path, out dirstatus, out filestatus);
            if (rt != 0)
            {
                result = false;
                //  ExceptionFactory.BuildThenThrow("RMP_RemoveTrustedProcess", rt);
            }
            return result;
        }

        /// <summary>
        /// Request to login
        /// </summary>
        /// <param name="callbackCmd">Generally is executable full path, 
        /// when login successfully, it will run this COMMAND as the callback</param>
        /// <param name="callbackCmdPara">it is the callback command's run param</param>
        /// <returns></returns>
        public bool SDWL_RPM_RequestLogin(string callbackCmd, string callbackCmdPara = "")
        {
            var rt = Boundary.SDWL_RPM_RequestLogin(hSession, callbackCmd, callbackCmdPara);
            return rt == 0;
        }

        /// <summary>
        /// Request if allow logout by sending broadcast message, so all recipients(e.g. rmd, viewer) should register
        /// the message code(uint type) both 'WM_CHECK_IF_ALLOW_LOGOUT'(50005) and 'WM_START_LOGOUT_ACTION'(50006) if want to listen.
        /// </summary>
        /// <param name="option"> 0:execute logout action (default value); 1:Check if allow to logout now.</param>
        /// <param name="isAllow">the result that if allow logout, the value is true or false.</param>
        /// <returns></returns>
        public void SDWL_RPM_RequestLogout(out bool isAllow, uint option = 0)
        {
            var rt = Boundary.SDWL_RPM_RequestLogout(hSession, out isAllow, option);
            if (rt != 0)
            {
                ExceptionFactory.BuildThenThrow("SDWL_RPM_RequestLogout", rt);
            }
        }

        /// <summary>
        ///  Notify or log messages to RPM service manager
        /// </summary>
        /// <param name="app">application name (caller)</param>
        /// <param name="target">the target of this activity (example, a new protected file name)</param>
        /// <param name="message">the message needs to be logged or notified (example, "You are not authorized to view the file.")</param>
        /// <param name="msgtype"> 0: log message; 1: popup bubble to notify</param>
        /// <param name="operation">the operation of the activity (example, "Upload")</param>
        /// <param name="result">the operation result, 0: failed, 1: succeed</param>
        /// <param name="fileStatus">Used to display file icon by file status, 0: Unknown, 1: Online, 2: Offline, 3: WaitingUpload</param>
        /// <returns></returns>
        public bool SDWL_RPM_NotifyMessage(string app, string target, string message,
            uint msgtype, string operation = "", uint result = 0, uint fileStatus = 0)
        {
            var rt = Boundary.SDWL_RPM_NotifyMessage(hSession, app, target, message, msgtype, operation, result, fileStatus);
            return rt == 0;
        }

        public void SDWL_RPM_NotifyRMXStatus(bool running)
        {
            var rt = Boundary.SDWL_RPM_NotifyRMXStatus(hSession, running);
            if (rt != 0)
            {
                //  ExceptionFactory.BuildThenThrow("RMP_RemoveTrustedProcess", rt);
            }
        }

        public bool IsPluginWell(string wszAppType, string wszPlatform)
        {
            return Boundary.IsPluginWell(wszAppType, wszPlatform);
        }


        public void SDWL_RPM_MonitorRegValueDeleted(string rmpPathInReg, Boundary.RegChangedCallback callback)
        {

            var rt = Boundary.SDWL_SYSHELPER_MonitorRegValueDeleted(rmpPathInReg, callback);
            if (rt != 0)
            {
                ExceptionFactory.BuildThenThrow("SDWL_SYSHELPER_RegChangeMonitor", rt);
            }
        }


    }
}
