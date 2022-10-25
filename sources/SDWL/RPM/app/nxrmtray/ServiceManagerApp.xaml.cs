using Microsoft.Win32;
using ServiceManager.resources.languages;
using ServiceManager.rmservmgr.app;
using ServiceManager.rmservmgr.app.recentNotification;
using ServiceManager.rmservmgr.app.user;
using ServiceManager.rmservmgr.common.components;
using ServiceManager.rmservmgr.common.helper;
using ServiceManager.rmservmgr.db;
using ServiceManager.rmservmgr.sdk;
using System;
using System.Collections.Generic;
using System.Configuration;
using System.Data;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.IO;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Interop;
using static ServiceManager.rmservmgr.app.UIMediator;

namespace ServiceManager
{
    /// <summary>
    /// Interaction logic for App.xaml
    /// </summary>
    public partial class ServiceManagerApp : Application
    {
        // AppID
        private static string AppID = "Nextlabs.Rmc.SkyDRM.nxrmtray";
        // Log
        private static readonly log4net.ILog log = log4net.LogManager.
            GetLogger(System.Reflection.MethodBase.GetCurrentMethod().DeclaringType);

        #region Field
        public static ServiceManagerApp Singleton { get => (ServiceManagerApp)Current; }
        public log4net.ILog Log { get => log; }

        private HeartBeater heartBeater;
        public AppConfig Config { get; private set; }
        public Session Session { get; private set; }
        public IUser User { get; private set; }
        public IRecentNotifications MyRecentNotifications { get; private set; }
        public DBProvider DBProvider { get; private set; }
        public UIMediator UIMediator { get; private set; }
        public TrayIconManager TrayIconManager { get; private set; }
        public ServiceManagerWin ServiceManagerWin { get; private set; }
        public NamedPipeServer NamedPipeServer { get; private set; }
        public bool IsPersonRouter { get; private set; }

        public event Action HeartBeatEvent;
        public void InvokeHeartBeatEvent()
        {
            HeartBeatEvent?.Invoke();
        }
        #endregion // Field

        private bool ShowDebugDialog()
        {
            bool result = false;
            try
            {
                Microsoft.Win32.RegistryKey skydrm = Microsoft.Win32.Registry.CurrentUser.CreateSubKey(@"Software\Nextlabs\SkyDRM");
                Microsoft.Win32.RegistryKey serviceMgr = skydrm.CreateSubKey(@"ServiceManager");
                int debugValue = (int)serviceMgr.GetValue("Debug", 0);
                if (debugValue == 1)
                {
                    result = true;
                }
            }
            catch (Exception)
            { }
            return result;
        }

        private void Application_Startup(object sender, StartupEventArgs e)
        {
            if (ShowDebugDialog())
            {
                MessageBox.Show("Debug nxrmtray!");
            }
            // Specifies a unique application-defined Application User Model ID (AppUserModelID), 
            // that identifies the current process to the taskbar.
            // This identifier allows an application to group its associated processes and windows under a single taskbar button.
            //Win32Common.SetCurrentProcessExplicitAppUserModelID(AppID);
            try
            {
                if (!InitComponents())
                {
                    Log.Fatal("Failed init components");
                    DialogHelper.ShowDlg_InitComponentFailed();
                    Environment.Exit(1);
                }

                Log.Info("Success for initializing major components, will try to startup");

                TryStartup();

                RegisterOfficeFileAssociation();

                Log.Info("Prepare NamedPipeServer");
                NamedPipeServer = new NamedPipeServer();
                NamedPipeServer.StartServer();
            }
            catch (Exception esg)
            {
                Log.Error("Error in Application_StartUp", esg);
                throw;
            }

        }

        /// <summary>
        /// Init major components.
        /// </summary>
        private bool InitComponents()
        {
            Log.Info("Enter --> InitComponents");

            try
            {
                // Init log
                log4net.Config.XmlConfigurator.Configure();



                // Init config
                Config = new AppConfig();

                // Init sdk
                Apis.SdkLibInit();

                // Init sdk
                Session = Apis.CreateSession(Config.RmSdkFolder);

                // Init mediator
                UIMediator = new UIMediator(this);

                // Init tray 
                TrayIconManager = new TrayIconManager(this);

                // Init db
                DBProvider = new DBProvider(Config.DataBaseFolder);

                Log.Info("Leave --> InitComponents");
                return true;
            }
            catch (Exception e)
            {
                Log.Error(e.ToString());
            }

            return false;
        }

        private void TryStartup()
        {
            try
            {
                Log.Info("Enter --> TryStartup");

                Session.Initialize(Config.RmSdkFolder, Config.UserRouter, "");
                Log.Info("Try to recover last session for the user " + Config.UserEmail);

                if (Session.RecoverUser(Config.UserEmail, Config.UserCode)
                    && DBProvider.OnUserRecovered(Session.User.Email, Config.UserRouter, Config.UserTenant) != -1)
                {
                    Log.Info("Recover user " + Session.User.Email + "  ok");

                    InitAfterLogin();
                }
                else
                {
                    Log.Info("Failed recover user, direct to Login");

                    // PM request, not auto show login Window
                    //UIMediator.OnShowChooseServerWin();
                }

                Log.Info("Leave --> TryStartup");
            }
            catch (Exception e)
            {
                Log.Error("Error in TryStartup:", e);
            }
        }

        public int Login(LoginPara para)
        {
            Log.Info("Enter --> Login");

            try
            {
                Session.SetLoginRequest(para.LoginStr);

                // set user info into app.config
                var user = Session.User;
                if (!Config.SetUserInfo(user.Name, user.Email, user.PassCode))
                {
                    return -1;
                }
                Tenant tenant = Session.GetCurrentTenant();
                if (!Config.SetTenantInfo(tenant.RouterURL, para.WebUrl, tenant.Name))
                {
                    return -1;
                }

                // set ticket, use for rmd add repository
                Config.SetTicketToRegistry(para.LoginStr);

                // upsert server table 
                if (!string.IsNullOrEmpty(tenant.RouterURL))
                {
                    // tell db weburl will be displayed -- using routerUrl and tenant as primary key intead of serverUrl -- fix bug 52730
                    DBProvider.UpsertServer(tenant.RouterURL, para.WebUrl, tenant.Name, !para.IsPersonal);
                }

                // tell DB User login
                DBProvider.OnUserLogin((int)user.UserId,
                                    user.Name, user.Email, user.PassCode, (int)user.UserType, para.LoginStr);

                Log.Info("Leave --> Login");
            }
            catch (RmRestApiException ske)
            {
                Log.Warn("error when preparing login user info," + ske.Message, ske);
                return ske.ErrorCode;
            }
            catch (Exception e)
            {
                Log.Warn("error when preparing login user info," + e.Message, e);
                return -1;
            }

            return 1;
        }

        /// <summary>
        /// Do some initialization after user login or recover.
        /// </summary>
        public void InitAfterLogin()
        {
            try
            {
                Log.Info("Prepare User session");

                User = new rmservmgr.app.user.User(DBProvider.GetUser());

                MyRecentNotifications = new MyRecentNotifications();

                // sm
                ServiceManagerWin = new ServiceManagerWin();
                // for add hook
                ServiceManagerWin.Show();

                // fix Bug 58716 - RPM folder icon is not removed after uninstall and install again
                GetUserRPMFolder();

                SetBrwDownloadPathToRPM();

                // Init tray
                TrayIconManager.IsLogin = true;
                TrayIconManager.OnGoing_Login = false;
                TrayIconManager.RefreshMenuItem();
                TrayIconManager.PopupTargetWin = ServiceManagerWin;

                // todo, start heartbeat
                heartBeater = new HeartBeater(this);
                heartBeater.WorkingBackground();

                // Set nxrmtray as trusted application since it monitor edited file.
                Process process = Process.GetCurrentProcess();
                Session.RPM_RegisterApp(process.MainModule.FileName);
                Session.SDWL_RPM_NotifyRMXStatus(true);
                Session.RMP_AddTrustedProcess(process.Id);
                Apis.WaitInstanceInitFinish();

                // Now Saas not support (myVault)token group instead of (systemBucket)defult token,
                // so rmd still use defult token when router is Saas, set IsPersonRouter = false
                //IsPersonRouter = IsSaasRouter();
                IsPersonRouter = false;

                //fixed bug 67712
                RemoveTempRPMDir();

                //fix Bug 70458 - Microstation load but not work for manage mode 
                ModifyMicrostationConfigFile();

                RecoverOfficeRMXLoadBehavior();
            }
            catch (Exception e)
            {
                Log.Error("Prepare User session failed", e);
            }

        }

        private void RecoverOfficeRMXLoadBehavior()
        {
            try
            {
                List<string> CurrentUserSubKeys = new List<string>();
                List<string> LocalMachineSubKeys = new List<string>();
                UIntPtr HKEY_CURRENT_USER = (UIntPtr)((long)0x80000001);
                UIntPtr HKEY_LOCAL_MACHINE = (UIntPtr)((long)0x80000002);

                //CurrentUser
                CurrentUserSubKeys.Add(@"Software\Microsoft\Office\Word\Addins\NxlRmAddin");
                CurrentUserSubKeys.Add(@"Software\Microsoft\Office\Excel\Addins\NxlRmAddin");
                CurrentUserSubKeys.Add(@"Software\Microsoft\Office\PowerPoint\Addins\NxlRmAddin");

                //LocalMachine
                LocalMachineSubKeys.Add(@"SOFTWARE\MICROSOFT\Office\Excel\Addins\NxlRmAddin");
                LocalMachineSubKeys.Add(@"SOFTWARE\MICROSOFT\Office\Word\Addins\NxlRmAddin");
                LocalMachineSubKeys.Add(@"SOFTWARE\MICROSOFT\Office\PowerPoint\Addins\NxlRmAddin");
                LocalMachineSubKeys.Add(@"SOFTWARE\Wow6432Node\Microsoft\Office\Excel\Addins\NxlRmAddin");
                LocalMachineSubKeys.Add(@"SOFTWARE\Wow6432Node\Microsoft\Office\Word\Addins\NxlRmAddin");
                LocalMachineSubKeys.Add(@"SOFTWARE\Wow6432Node\Microsoft\Office\PowerPoint\Addins\NxlRmAddin");

                //LocalMachine clickToRun
                LocalMachineSubKeys.Add(@"SOFTWARE\MICROSOFT\Office\ClickToRun\REGISTRY\MACHINE\SOFTWARE\Microsoft\Office\Excel\Addins\NxlRmAddin");
                LocalMachineSubKeys.Add(@"SOFTWARE\MICROSOFT\Office\ClickToRun\REGISTRY\MACHINE\SOFTWARE\Microsoft\Office\Word\Addins\NxlRmAddin");
                LocalMachineSubKeys.Add(@"SOFTWARE\MICROSOFT\Office\ClickToRun\REGISTRY\MACHINE\SOFTWARE\Microsoft\Office\PowerPoint\Addins\NxlRmAddin");
                LocalMachineSubKeys.Add(@"SOFTWARE\MICROSOFT\Office\ClickToRun\REGISTRY\MACHINE\SOFTWARE\Wow6432Node\Microsoft\Office\Excel\Addins\NxlRmAddin");
                LocalMachineSubKeys.Add(@"SOFTWARE\MICROSOFT\Office\ClickToRun\REGISTRY\MACHINE\SOFTWARE\Wow6432Node\Microsoft\Office\Word\Addins\NxlRmAddin");
                LocalMachineSubKeys.Add(@"SOFTWARE\MICROSOFT\Office\ClickToRun\REGISTRY\MACHINE\SOFTWARE\Wow6432Node\Microsoft\Office\PowerPoint\Addins\NxlRmAddin");

                string name = "LoadBehavior";
                uint value = 3;

                foreach (string key in CurrentUserSubKeys)
                {
                    try
                    {
                        //bool rt = Session.SDWL_Register_SetValue(HKEY_CURRENT_USER, key, name, value);
                        RegistryKey registryKey = Registry.CurrentUser.CreateSubKey(key, RegistryKeyPermissionCheck.ReadWriteSubTree);
                        registryKey.SetValue(name, value,RegistryValueKind.DWord);
                    }
                    catch (Exception ex)
                    {

                    }
                }

                foreach (string key in LocalMachineSubKeys)
                {
                    bool rt = Session.SDWL_Register_SetValue(HKEY_LOCAL_MACHINE, key, name, value);
                }
            }
            catch (Exception ex)
            {

            }
        }


        //fix Bug 70458 - Microstation load but not work for manage mode 
        private void ModifyMicrostationConfigFile()
        {
            try
            {
                //example:
                //"C:\\Users\\jrzhou\\AppData\\Local\\Bentley\\MicroStation\\10.0.0\\prefs\\Personal.ucf"
                string filePath = Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData) + @"\Bentley\MicroStation\10.0.0\prefs\Personal.ucf";

                bool haveFound = false;
                // Read the file and display it line by line.  
                foreach (string line in System.IO.File.ReadLines(filePath))
                {
                    if (line.Contains("MS_DGNAPPS") && line.Contains("NXMicrosRMX"))
                    {
                        haveFound = true;
                        break;
                    }
                }

                if (!haveFound)
                {
                    StreamWriter streamWriter = new StreamWriter(filePath, append: true);
                    streamWriter.WriteLine("MS_DGNAPPS > NXMicrosRMX");
                    streamWriter.Close();
                }
            }
            catch (Exception ex)
            {

            }
        }


        // In Session.GetUserRPMFolder() , if user rpm folder is invalid, will clear registry value and folder icon
        private string GetUserRPMFolder()
        {
            string path = "";
            try
            {
                path = Session.GetUserRPMFolder();
            }
            catch (Exception e)
            {
                Log.Error(e);
            }
            return path;
        }

        /// <summary>
        /// Set browser download path to rpm folder
        /// </summary>
        private void SetBrwDownloadPathToRPM()
        {
            try
            {
                int option;
                string tags = "";
                string path = GetBrowserDownloadPath.GetChromeDownloadPath();
                bool isRPMFolder = Session.RMP_IsSafeFolder(path, out option, out tags);
                if (!isRPMFolder)
                {
                    Session.RPM_AddDir(path,
                        (int)(SDRmRPMFolderOption.RPMFOLDER_OVERWRITE | SDRmRPMFolderOption.RPMFOLDER_EXT), "");
                }
            }
            catch (RmRestApiException e)
            {
                Log.Error(e);
            }
            catch (Exception e)
            {
                Log.Error(e);
                ServiceManagerWin?.ShowNotifyWindow(new MessagePara()
                {
                    Application = CultureStringInfo.Tray_SkyDRM,
                    Target = "Chrome",
                    Message = "Failed to set Chrome download directory to RPM folder."
                });
            }

            try
            {
                int option;
                string tags = "";
                string path = GetBrowserDownloadPath.GetEdgeDownloadPath();
                bool isRPMFolder = Session.RMP_IsSafeFolder(path, out option, out tags);
                if (!isRPMFolder)
                {
                    Session.RPM_AddDir(path,
                    (int)(SDRmRPMFolderOption.RPMFOLDER_OVERWRITE | SDRmRPMFolderOption.RPMFOLDER_EXT), "");
                }
            }
            catch (RmRestApiException e)
            {
                Log.Error(e);
            }
            catch (Exception e)
            {
                Log.Error(e);
                ServiceManagerWin?.ShowNotifyWindow(new MessagePara()
                {
                    Application = CultureStringInfo.Tray_SkyDRM,
                    Target = "Edge",
                    Message = "Failed to set Edge download directory to RPM folder."
                });
            }

            try
            {
                int option;
                string tags = "";
                string path = GetBrowserDownloadPath.GetFireFoxDownloadPath();
                bool isRPMFolder = Session.RMP_IsSafeFolder(path, out option, out tags);
                if (!isRPMFolder)
                {
                    Session.RPM_AddDir(path,
                    (int)(SDRmRPMFolderOption.RPMFOLDER_OVERWRITE | SDRmRPMFolderOption.RPMFOLDER_EXT), "");
                }
             }
            catch (RmRestApiException e)
            {
                Log.Error(e);
            }
            catch (Exception e)
            {
                Log.Error(e);
                ServiceManagerWin?.ShowNotifyWindow(new MessagePara()
                {
                    Application = CultureStringInfo.Tray_SkyDRM,
                    Target = "FireFox",
                    Message = "Failed to set FireFox download directory to RPM folder."
                });
            }

            try
            {
                int option;
                string tags = "";
                string path = GetBrowserDownloadPath.GetIEDownloadPath();
                bool isRPMFolder = Session.RMP_IsSafeFolder(path, out option, out tags);
                if (!isRPMFolder)
                {
                    Session.RPM_AddDir(path,
                   (int)(SDRmRPMFolderOption.RPMFOLDER_OVERWRITE | SDRmRPMFolderOption.RPMFOLDER_EXT), "");
                }
               
            }
            catch (RmRestApiException e)
            {
                Log.Error(e);
            }
            catch (Exception e)
            {
                Log.Error(e);
                ServiceManagerWin?.ShowNotifyWindow(new MessagePara()
                {
                    Application = CultureStringInfo.Tray_SkyDRM,
                    Target = "Internet Explorer",
                    Message = "Failed to set Internet Explorer download directory to RPM folder."
                });
            }
        }

        public uint WM_CHECK_IF_ALLOW_LOGOUT = 50005;
        public uint WM_START_LOGOUT_ACTION = 50006;

        /// <summary>
        /// Try to request to logout.
        /// </summary>
        public void RequestLogout()
        {
            bool isAllow = true;
            try
            {
                Session.SDWL_RPM_RequestLogout(out isAllow, 1);
                if (isAllow)
                {
                    Session.SDWL_RPM_RequestLogout(out isAllow);
                }
                else
                {
                    Log.Info("Deny logout now");
                }
            }
            catch (Exception e)
            {
                this.Log.Info(e.ToString());
            }
        }

        /// <summary>
        /// Here actual execute logout action.
        /// </summary>
        public void ExecuteLogout()
        {
            Log.Info("Enter --> Logout");

            try
            {
                DBProvider.OnUserLogout();
                heartBeater.Stop = true;
                Session.SaveSession(Config.RmSdkFolder);
                Session.User.Logout();
                Session.DeleteSession();
                Session = null;
            }
            catch (Exception e)
            {
                Log.Error("app.Session.User.Logout is failed, msg:" + e.Message, e);
            }
            finally
            {
                // Always make user logout whether above operation execute successsfully or not.
                if (!Config.ClearUserInfo())
                {
                    log.Warn("Clear user inof failed.");
                }
            }

            //this.ManualExit();

            // init tray icon.
            TrayIconManager.IsLogin = false;
            TrayIconManager.RefreshMenuItem();

            try
            {
                // try to close all windows.
                foreach (Window one in Singleton.Windows)
                {
                    if (one != null)
                    {
                        one.Close();
                    }
                }
            }
            catch (Exception e)
            {
                Console.WriteLine(e.ToString());
            }

            // create a new clean session 
            Session = Apis.CreateSession(Config.RmSdkFolder);

            // show chooseServer Window
            UIMediator.OnShowChooseServerWin();

            Log.Info("Leave --> Logout");
        }

        /// <summary>
        /// Easy use func, to show msg in bubble in right-lower corner of windows explorer 
        /// </summary>
        public void ShowBalloonTip(string text, int timeout = 1000)
        {
            if (TrayIconManager != null)
            {
                Log.Info(text);
                TrayIconManager.ni.BalloonTipText = text;
                TrayIconManager.ni.ShowBalloonTip(timeout);
            }
        }

        public bool SignalExternalCommandLineArgs(IList<string> args)
        {
            // todo
            return false;
        }

        /// <summary>
        /// Will be triggered when windows session ends, such as user logout or shutdown the PC.
        /// </summary>
        protected override void OnSessionEnding(SessionEndingCancelEventArgs e)
        {
            base.OnSessionEnding(e);

            Log.Info("OnSessionEnding");
        }

        public void ManualExit()
        {
            this.Shutdown(0);
        }

        private void Application_Exit(object sender, ExitEventArgs e)
        {
            NamedPipeServer.StopServer();

            Log.Info("OnExit");
            // release all used res here
            try
            {
                if (TrayIconManager != null)
                {
                    TrayIconManager.ni.Dispose();
                }
            }
            catch (Exception ee)
            {
                log.Error("error in Application_Exit", ee);
            }

            try
            {
                if (Session != null)
                {
                    Session.SaveSession(Config.RmSdkFolder);
                    Session.DeleteSession();
                }

                Apis.SdkLibCleanup();
            }
            catch (Exception ee)
            {
                log.Error("error in Application_Exit", ee);
            }

            // Can call this only before application exit, or else failed to initialize cef again by call Cef.Initialize when switch user,
            // since it will shutdown whole cef infrastructure. That means CEF can only be Initialized/Shutdown once per process, 
            // this is a limitation of the underlying Chromium framework.
            if (!CefSharp.Cef.IsShutdown)
            {
                CefSharp.Cef.Shutdown();
            }

            // comments by osmond, this is temp work around method to kill self process forcefully
            // the reason is that we may launch namedpipe which may denied process kill it self.
            Process.GetCurrentProcess().Kill();
        }

        private bool IsSaasRouter()
        {
            bool result = false;

            if (Config.PersonRouter.Equals(Config.UserRouter, StringComparison.CurrentCultureIgnoreCase))
            {
                result = true;
            }
            return result;
        }


        private string GetClassPath(string classid)
        {
            log.Info("GetClassPath");
            string result = string.Empty;
            string appclassid = "CLSID\\" + classid + "\\LocalServer32";
            string appclassid32 = "WOW6432Node\\CLSID\\" + classid + "\\LocalServer32";
            RegistryKey registryKey = null;
            RegistryKey registryKey32 = null;

            try
            {
                if (string.IsNullOrEmpty(classid))
                {
                    return string.Empty;
                }

                string appPath = string.Empty;
                registryKey = Registry.ClassesRoot.OpenSubKey(appclassid, false);
                appPath = (string)registryKey?.GetValue("", "");

                if (string.IsNullOrEmpty(appPath))
                {
                    registryKey32 = Registry.ClassesRoot.OpenSubKey(appclassid32, false);
                    appPath = (string)registryKey32?.GetValue("", "");
                }

                if (!string.IsNullOrEmpty(appPath))
                {
                    int index = appPath.IndexOf(".EXE", StringComparison.CurrentCultureIgnoreCase);
                    if (index > 0)
                    {
                        appPath = appPath.Substring(0, index + 4);
                    }

                    appPath = appPath.Trim();

                    if (appPath[0] == '\"' && appPath[appPath.Length - 1] != '\"')
                    {
                        appPath += '\"';
                    }

                    result = appPath;
                }
            }
            catch (Exception ex)
            {
                log.Error(ex);
            }
            finally
            {
                registryKey?.Close();
                registryKey32?.Close();
            }
            return result;
        }

        private string GetApplicationPath(string appName)
        {
            log.Info("GetApplicationPath");
            string result = string.Empty;
            RegistryKey CLSID_registryKey = null;
            try
            {
                string appclassid = appName + "\\CLSID";
                CLSID_registryKey = Registry.ClassesRoot.OpenSubKey(appclassid, false);
                string clsid = (string)CLSID_registryKey?.GetValue("");
                result = GetClassPath(clsid);
            }
            catch (Exception ex)
            {
                log.Error(ex);
            }
            finally
            {
                CLSID_registryKey?.Close();
            }
            return result;
        }

        [DllImport("kernel32.dll", SetLastError = true, CharSet = CharSet.Auto)]
        [return: MarshalAs(UnmanagedType.U4)]
        private static extern int GetLongPathName(
        [MarshalAs(UnmanagedType.LPTStr)]
        string lpszShortPath,
        [MarshalAs(UnmanagedType.LPTStr)]
        System.Text.StringBuilder lpszLongPath,
        [MarshalAs(UnmanagedType.U4)]
        int cchBuffer);

        private string GetLongPath(string shortPath)
        {
            if (String.IsNullOrEmpty(shortPath))
            {
                return shortPath;
            }

            StringBuilder builder = new StringBuilder(255);
            int result = GetLongPathName(shortPath, builder, builder.Capacity);
            if (result > 0 && result < builder.Capacity)
            {
                return builder.ToString(0, result);
            }
            else
            {
                if (result > 0)
                {
                    builder = new StringBuilder(result);
                    result = GetLongPathName(shortPath, builder, builder.Capacity);
                    return builder.ToString(0, result);
                }
                else
                {
                    throw new Exception("Convert short path to long path failed");
                }
            }
        }

        private void RegisterOfficeFileAssociation()
        {
            log.Info("RegisterOfficeFileAssociation");
            try
            {
                string[] arrWord = { ".doc", ".docx", ".dot", ".dotx", ".rtf", ".docm", ".dotm", ".odt" };
                string[] arrExcel = { ".xls", ".xlsx", ".xlt", ".xltx", ".xlsb", ".csv", ".ods", ".xlm", ".xltm", ".xlsm" };
                string[] arrPowerPoint = { ".ppt", ".pptx", ".ppsx", ".potx", ".odp", ".pptm", ".potm", ".pps", ".ppsm", ".pot" };

                string lwordapp = GetLongPath(GetApplicationPath("Word.Application"));
                string lexcelapp = GetLongPath(GetApplicationPath("Excel.Application"));
                string lpowerpointapp = GetLongPath(GetApplicationPath("PowerPoint.Application"));

                if (!string.IsNullOrEmpty(lwordapp))
                {
                    Session.RPM_RegisterApp(lwordapp);
                }

                if (!string.IsNullOrEmpty(lexcelapp))
                {
                    Session.RPM_RegisterApp(lexcelapp);
                }

                if (!string.IsNullOrEmpty(lpowerpointapp))
                {
                    Session.RPM_RegisterApp(lpowerpointapp);
                }

                for (int i = 0; i < arrWord.Length; i++)
                {
                    try
                    {
                        Session.SDWL_RPM_RegisterFileAssociation(arrWord[i], lwordapp);
                    }
                    catch (Exception ex)
                    {
                        log.Error(ex);
                    }
                }

                for (int i = 0; i < arrExcel.Length; i++)
                {
                    try
                    {
                        Session.SDWL_RPM_RegisterFileAssociation(arrExcel[i], lexcelapp);
                    }
                    catch (Exception ex)
                    {
                        log.Error(ex);
                    }

                }

                for (int i = 0; i < arrPowerPoint.Length; i++)
                {
                    try
                    {
                        Session.SDWL_RPM_RegisterFileAssociation(arrPowerPoint[i], lpowerpointapp);
                    }
                    catch (Exception ex)
                    {
                        log.Error(ex);
                    }

                }
            }
            catch (Exception ex)
            {
                log.Error(ex);
            }
            finally
            {
            }
        }

        /// <summary>
        /// Fixed  Bug 67712 - After open dgn.nxl file in normalDir, failed remove temp file under Intermediate folder
        /// </summary>
        private void RemoveTempRPMDir()
        {
            string intermediateFolder = System.Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData) + @"\Nextlabs\SkyDRM\Intermediate";

            if (Directory.Exists(intermediateFolder))
            {
                int option;
                string tags;
                if (Session.RMP_IsSafeFolder(intermediateFolder, out option, out tags))
                {
                    try
                    {
                        DelectDir(intermediateFolder);
                    }
                    catch (Exception ex)
                    {
                        log.Error("Remove intermediate folder failed!", ex);
                    }
                }
            }
        }

        private void DelectDir(string srcPath)
        {
            try
            {
                DirectoryInfo dir = new DirectoryInfo(srcPath);
                FileSystemInfo[] fileinfo = dir.GetFileSystemInfos();
                foreach (FileSystemInfo i in fileinfo)
                {
                    if (i is DirectoryInfo)
                    {
                        DirectoryInfo subdir = new DirectoryInfo(i.FullName);
                        subdir.Delete(true);
                    }
                    else
                    {
                        File.Delete(i.FullName);
                    }
                }
            }
            catch (Exception)
            {
                throw;
            }
        }

    }
}
