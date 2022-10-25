using Microsoft.Win32;
using Newtonsoft.Json.Linq;
using ServiceManager.rmservmgr.sdk;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace ServiceManager.rmservmgr.app
{
    /*ServieManager's all supported configurations reside in Windows regiestry
    *  Nextlabs
    *      SkyDRM
    *          ServiceManager
    *              Directory :=  working folder         
    *              Executable :=  exe path
    *              Router :=  defualt RMS server
    *              Tenant :=  default Tenant
    *              CompanyRouter:=defualt ""
    *              HeartBeatIntervalSec := Deault_Heartbeat
    *              FolderProtect := Mark some folder in NTFS with remove it list and read permission
    *              User
    *                  Name: user name
    *                  Email: user email
    *                  Code: user pass code
    *                  Login Time: user login time
    *                  Router:  user used Router
    *                  Tenant:  user used tenant
    */
    public class AppConfig
    {
        const int minHeartBeatSeconds = 60;
        const int maxHeartBeatSeconds = 3600 * 24;

        int heartbeatIntervalSec;

        public string WorkingFolder { get; }
        public string RmSdkFolder { get; }
        public string DataBaseFolder { get; }

        public string AppPath { get; }

        public string UserName { get; private set; }
        public string UserEmail { get; private set; }
        public string UserCode { get; private set; }
        public string UserRouter { get; private set; }
        public string UserTenant { get; private set; }
        public string UserUrl { get; private set; }

        public string PersonRouter { get => GetPersonRouter(); }
        public string CompanyRouter { get => GetCompanyRouter(); }

        public int HeartBeatIntervalSec { get => heartbeatIntervalSec; set => SetHeartBeatFrequence(value); }

        public bool LeaveCopy { get => GetSysPreference(0); set => SetSysPreference(0, value); }
        public bool ShowNotifyWin { get => GetSysPreference(1); set => SetSysPreference(1, value); }

        public AppConfig()
        {
            RegistryKey skydrm = Registry.CurrentUser.CreateSubKey(@"Software\Nextlabs\SkyDRM");
            RegistryKey serviceMgr = skydrm.CreateSubKey(@"ServiceManager");
            RegistryKey user = serviceMgr.CreateSubKey(@"User"); ;

            try
            {
                // Init basic frame, write all files into User/AppData/Local/SkyDRM/
                WorkingFolder = System.Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData) + @"\Nextlabs\SkyDRM";

                // Make sure the folder exist, and set it as hidden.
                System.IO.Directory.CreateDirectory(WorkingFolder);
                System.IO.DirectoryInfo wdInfo = new System.IO.DirectoryInfo(WorkingFolder);
                wdInfo.Attributes |= System.IO.FileAttributes.Hidden;
                serviceMgr.SetValue("Directory", WorkingFolder, RegistryValueKind.String);

                // User may change our app into other folder, so set the two values every time
                AppPath = System.Diagnostics.Process.GetCurrentProcess().MainModule.FileName;
                serviceMgr.SetValue("Executable", AppPath, RegistryValueKind.String);

                // rmsdk
                RmSdkFolder = WorkingFolder + "\\" + "rmsdk";
                System.IO.Directory.CreateDirectory(RmSdkFolder);

                // database
                DataBaseFolder = WorkingFolder + "\\" + "database";
                System.IO.Directory.CreateDirectory(DataBaseFolder);

                int showNotifyWin = (int)skydrm.GetValue("ShowNotifyWin", 1);
                if (showNotifyWin == 1)
                {
                    skydrm.SetValue("ShowNotifyWin", showNotifyWin, RegistryValueKind.DWord);
                }

                // heartbeat interval
                heartbeatIntervalSec = (int)serviceMgr.GetValue("HeartbeatIntervalSec", Config.Deault_Heartbeat);
                if (heartbeatIntervalSec == Config.Deault_Heartbeat)
                {
                    serviceMgr.SetValue("HeartbeatIntervalSec", heartbeatIntervalSec, RegistryValueKind.DWord);
                }
                else if(heartbeatIntervalSec < minHeartBeatSeconds || heartbeatIntervalSec < maxHeartBeatSeconds)
                {
                    serviceMgr.SetValue("HeartbeatIntervalSec", Config.Deault_Heartbeat, RegistryValueKind.DWord);
                    heartbeatIntervalSec = Config.Deault_Heartbeat;
                }

                // user info    
                UserName = (string)user.GetValue("Name", "");
                UserEmail = (string)user.GetValue("Email", "");
                UserCode = (string)user.GetValue("Code", "");
                UserRouter = (string)user.GetValue("Router", "");
                UserTenant = (string)user.GetValue("Tenant", "");
                UserUrl= (string)user.GetValue("Url", ""); 
            }
            finally
            {
                user?.Close();
                serviceMgr?.Close();
                skydrm?.Close();
            }
        }

        private string GetPersonRouter()
        {
            // for person Router, get value from local_machine
            string router = Config.Default_Router;
            try
            {
                RegistryKey localMachine = Registry.LocalMachine.OpenSubKey(@"Software\Nextlabs\SkyDRM\LocalApp", false);
                if (localMachine != null)
                {
                    router = (string)localMachine.GetValue("Router", Config.Default_Router);
                    localMachine.Close();
                }
            }
            catch (Exception)
            {
                router = Config.Default_Router;
            }
            return router;
        }

        private string GetCompanyRouter()
        {
            // for company Router, get value from local_machine
            string router = "";
            try
            {
                RegistryKey localMachine = Registry.LocalMachine.OpenSubKey(@"Software\Nextlabs\SkyDRM\LocalApp", false);
                if (localMachine != null)
                {
                    router = (string)localMachine.GetValue("CompanyRouter", "");
                    localMachine.Close();
                }
            }
            catch (Exception)
            {
                router = "";
            }
            return router;
        }

        /// <summary>
        /// Set ticket to registry, use for RMD to add repository
        /// </summary>
        /// <param name="json"></param>
        public void SetTicketToRegistry(string json)
        {
            if (string.IsNullOrEmpty(json))
            {
                return;
            }

            string ticket = string.Empty;
            JObject jo = JObject.Parse(json);
            if (jo.ContainsKey("extra"))
            {
                JObject joExtra = (JObject)jo.GetValue("extra");
                ticket = joExtra["ticket"].ToString();
            }
            
            RegistryKey user = Registry.CurrentUser.CreateSubKey(@"Software\Nextlabs\SkyDRM\ServiceManager\User");
            user.SetValue("Ticket", ticket, RegistryValueKind.String);
            user.Close();

        }

        public bool SetTenantInfo(string router, string url, string tenant)
        {
            if (router == null || router.Length == 0)
            {
                return false;
            }
            if(tenant == null || tenant.Length == 0)
            {
                return false;
            }

            RegistryKey user = Registry.CurrentUser.CreateSubKey(@"Software\Nextlabs\SkyDRM\ServiceManager\User");
            user.SetValue("Router", router, RegistryValueKind.String);
            user.SetValue("Tenant", tenant, RegistryValueKind.String);
            user.SetValue("Url", url, RegistryValueKind.String);
            user.Close();

            UserRouter = router;
            UserTenant = tenant;
            UserUrl = url;

            return true;
        }

        public bool SetUserInfo(string name, string email, string code)
        {
            // sanity check
            if (name == null || name.Length == 0)
            {
                return false;
            }
            if (email == null || email.Length == 0)
            {
                return false;
            }
            if (code == null || code.Length == 0)
            {
                return false;
            }

            RegistryKey user = Registry.CurrentUser.CreateSubKey(@"Software\Nextlabs\SkyDRM\ServiceManager\User");

            user.SetValue("Name", name, RegistryValueKind.String);
            user.SetValue("Email", email, RegistryValueKind.String);
            user.SetValue("Code", code, RegistryValueKind.String);
            // get cuttent time value
            user.SetValue("Login Time", System.DateTime.Now.ToString(), RegistryValueKind.String);
            user.Close();

            UserName = name;
            UserEmail = email;
            UserCode = code;

            return true;
        }

        // Clear the user info when logout
        // Fix bug 54824, and allow user to logout even though the network is offline.
        public bool ClearUserInfo()
        {
            try
            {
                RegistryKey user = Registry.CurrentUser.CreateSubKey(@"Software\Nextlabs\SkyDRM\ServiceManager\User");
                user.SetValue("Name", "", RegistryValueKind.String);
                user.SetValue("Email", "", RegistryValueKind.String);
                user.SetValue("Code", "", RegistryValueKind.String);
                user.Close();

                UserName = "";
                UserEmail = "";
                UserCode = "";

                return true;
            }
            catch (Exception e)
            {
                ServiceManagerApp.Singleton.Log.Info(e.ToString());
            }

            return false;
        }

        private void SetHeartBeatFrequence(int newSeconds)
        {
            // Sanity check
            if(newSeconds < minHeartBeatSeconds || newSeconds > maxHeartBeatSeconds)
            {
                ServiceManagerApp.Singleton.Log.Info(String.Format("new hertbeat values {0} out from rang,set it as default {1}", 
                    newSeconds, Config.Deault_Heartbeat));

                newSeconds = Config.Deault_Heartbeat;
            }

            RegistryKey serviceMgr = Registry.CurrentUser.CreateSubKey(@"Software\Nextlabs\SkyDRM\ServiceManager");
            try
            {
                serviceMgr.SetValue("HeartbeatIntervalSec", newSeconds, RegistryValueKind.DWord);
                heartbeatIntervalSec = newSeconds;
            }
            finally
            {
                if (serviceMgr != null)
                {
                    serviceMgr.Close();
                }
            }
        }
        /// <summary>
        /// Get system preference from Registry @"Software\Nextlabs\SkyDRM"
        /// 0:LeaveCopy, 1:ShowNotifyWin
        /// </summary>
        /// <param name="nameType"></param>
        /// <returns></returns>
        private bool GetSysPreference(int nameType)
        {
            string name=string.Empty;
            switch (nameType)
            {
                case 0:
                    name = "LeaveCopy";
                    break;
                case 1:
                    name = "ShowNotifyWin";
                    break;
                default:
                    break;
            }
            if (string.IsNullOrEmpty(name))
            {
                return false;
            }

            int result;
            RegistryKey skydrm = Registry.CurrentUser.OpenSubKey(@"Software\Nextlabs\SkyDRM");
            try
            {
                result = (int)skydrm.GetValue(name, 0);
            }
            finally
            {
                skydrm?.Close();
            }
            return result == 1;
        }
        /// <summary>
        /// Set system preference in Registry @"Software\Nextlabs\SkyDRM"
        /// 0:LeaveCopy, 1:ShowNotifyWin
        /// </summary>
        /// <param name="nameType"></param>
        /// <param name="value"></param>
        private void SetSysPreference(int nameType, bool value)
        {
            string name = string.Empty;
            switch (nameType)
            {
                case 0:
                    name = "LeaveCopy";
                    break;
                case 1:
                    name = "ShowNotifyWin";
                    break;
                default:
                    break;
            }
            if (string.IsNullOrEmpty(name))
            {
                return;
            }

            int result = value == true ? 1 : 0;
            RegistryKey skydrm = Registry.CurrentUser.CreateSubKey(@"Software\Nextlabs\SkyDRM");
            try
            {
                skydrm.SetValue(name, result, RegistryValueKind.DWord);
            }
            catch(Exception e)
            {
                ServiceManagerApp.Singleton.Log.Error("Error in SetSysPreference", e);
            }
            finally
            {
                skydrm?.Close();
            }
        }

    }
}
