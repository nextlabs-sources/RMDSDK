using Microsoft.Win32;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using ServiceManager.rmservmgr.common.filemonitor;
using ServiceManager.rmservmgr.common.helper;
using ServiceManager.rmservmgr.sdk;
using System;
using System.Collections.Generic;
using System.IO;
using System.IO.Pipes;
using System.Linq;
using System.Security.Principal;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using System.Windows.Threading;

namespace ServiceManager.rmservmgr.common.components
{
    public class NamedPipeServer
    {
        private Thread readThread;
        private const int BUF_SIZE = 4096;
        private bool exitThreadForced = false;
        private static readonly int remoteUserSessionId = System.Diagnostics.Process.GetCurrentProcess().SessionId;
        private static readonly string PIPE_NAME = "nxrmtray_" + remoteUserSessionId.ToString();

        public void StartServer()
        {
            readThread = new Thread(new ThreadStart(ListenForClient));
            readThread.Start();
        }

        private void ListenForClient()
        {
            // Set DACL
            SecurityIdentifier si = new SecurityIdentifier(WellKnownSidType.AuthenticatedUserSid, null);
            PipeSecurity psa = new PipeSecurity();
            psa.SetAccessRule(new PipeAccessRule(
                si, PipeAccessRights.ReadWrite,
                System.Security.AccessControl.AccessControlType.Allow));
            using (NamedPipeServerStream ss = new NamedPipeServerStream(
                   PIPE_NAME,
                   PipeDirection.InOut,
                   NamedPipeServerStream.MaxAllowedServerInstances,
                   PipeTransmissionMode.Message,
                   PipeOptions.None,
                   BUF_SIZE, BUF_SIZE,
                   psa))
            {
                while (true)
                {
                    if (exitThreadForced)
                    {
                        break;
                    }
                    ss.WaitForConnection();
                    try
                    {
                        // read first
                        string data = ReadDataBy(ss);

                        // Receive data from service
                        if (StringHelper.IsValidJsonStr_Fast(data))
                        {
                            Excute(data);
                        }

                        // Receive data from explorer.
                        else
                        {
                            string localPath = data.Substring(0, data.IndexOf('\0'));

                            ServiceManagerApp.Singleton.Log.Info("GetNxlFileFingerPrint:"+ localPath);

                            NxlFileFingerPrint fp = ServiceManagerApp.Singleton.Session.User.GetNxlFileFingerPrint(localPath);

                            // response 
                            var buf = Encoding.UTF8.GetBytes(ParseRights(fp));
                            ss.Write(buf, 0, buf.Length);
                            ss.Flush();
                        }
                    }
                    catch (Exception e)
                    {
                        Console.WriteLine(e.ToString());
                    }
                    ss.Disconnect();
                }
            }
        }

        private string ReadDataBy(Stream sr)
        {
            var buffer = new byte[BUF_SIZE];
            int bytesRead = sr.Read(buffer, 0, BUF_SIZE);
            string data = Encoding.UTF8.GetString(buffer, 0, bytesRead);

            ServiceManagerApp.Singleton.Log.Info("ReadData: " + data);
            return data;
        }

        private bool IsLoginWindowShowed()
        {
            return ServiceManagerApp.Singleton.TrayIconManager.IsChooseServerWindowLoaded
                || ServiceManagerApp.Singleton.TrayIconManager.IsLoginWindowLoaded;
        }

        private void Excute(string value)
        {
            try
            {
                string data = value;
                // login or log out
                JObject jo = (JObject)JsonConvert.DeserializeObject(data);
                if (jo.ContainsKey("code"))
                {
                    string code = jo["code"].ToString();
                    switch (code)
                    {
                        case "100":
                            //notify
                            if (jo.ContainsKey("message"))
                            {
                                var msg = jo.GetValue("message");
                                if (msg != null)
                                {
                                    ServiceManagerApp.Singleton.Dispatcher.BeginInvoke(DispatcherPriority.Background, new Action(delegate () 
                                    {
                                        try
                                        {
                                            if (ServiceManagerApp.Singleton.TrayIconManager.IsLogin)
                                            {
                                                ServiceManagerApp.Singleton.MyRecentNotifications?.UpdateOrInsertMessage(msg.ToString());
                                            }
                                            else
                                            {
                                                JObject msgJo = (JObject)JsonConvert.DeserializeObject(msg.ToString());
                                                if (msgJo.ContainsKey("message"))
                                                {
                                                    ServiceManagerApp.Singleton.ShowBalloonTip(msgJo["message"].ToString());
                                                }
                                            }
                                        }
                                        catch (Exception e)
                                        {
                                            ServiceManagerApp.Singleton.Log.Error("Notify message error: ", e);            
                                        }
                                    }));
                                }
                            }
                            break;
                        case "101":
                            //login
                            ServiceManagerApp.Singleton.Dispatcher.BeginInvoke(new Action(delegate ()
                            {
                                if (!ServiceManagerApp.Singleton.TrayIconManager.IsLogin 
                                    && !IsLoginWindowShowed())
                                {
                                    ServiceManagerApp.Singleton.UIMediator.OnShowChooseServerWin();
                                }
                            }));
                            break;
                        // monitor notification
                        case "104":
                            HandleMonitorNotification(jo);
                            break;
                        default:
                            break;
                    }

                    return;
                }
            }
            catch (Exception e)
            {
                ServiceManagerApp.Singleton.Log.Error("Error In NamedPipeServer Excute", e);
            }
            
        }

        public void StopServer()
        {
            exitThreadForced = true;
        }

        private void HandleMonitorNotification(JObject jo)
        {
            try
            {
                if (jo.ContainsKey("action"))
                {
                    if (jo.ContainsKey("data"))
                    {
                        string dataJson = jo.GetValue("data").ToString();
                        JObject dataJo = (JObject)JsonConvert.DeserializeObject(dataJson);

                        uint pid = 0;
                        if (uint.TryParse(dataJo["pid"].ToString(), out uint result))
                        {
                            pid = result;
                        }
                        string appName = dataJo["application"]?.ToString();

                        // file is opened
                        if (jo["action"]?.ToString() == "Open file")
                        {
                            string file = dataJo["file"]?.ToString();
                            bool isAppEditAllow = (bool)dataJo["isAppAllowEdit"];
                            bool isFileAllowEdit = (bool)dataJo["isFileAllowEdit"];
                            IMonitoredItem item = new MonitoredItem(pid, appName, file, isAppEditAllow, isFileAllowEdit);

                            // start monitor
                            FileMonitor.GetInstance().StartMonitor(item);
                        }
                        // process exit
                        else
                        {
                            if (jo["action"]?.ToString() == "Process exit")
                            {
                                JToken dataJk = jo.GetValue("data");
                                string application = dataJk["application"]?.ToString();

                                if (string.Equals(Path.GetFileName(application),"POWERPNT.EXE",StringComparison.CurrentCultureIgnoreCase)
                                    ||
                                    string.Equals(Path.GetFileName(application), "EXCEL.EXE", StringComparison.CurrentCultureIgnoreCase)
                                    ||
                                    string.Equals(Path.GetFileName(application), "WINWORD.EXE", StringComparison.CurrentCultureIgnoreCase)
                                    )
                                {
                                    ClearRecordOfDocumentRecovery(InvoluedRegistry(Path.GetFileName(application)));
                                }
                            }
                            // stop monitor
                            FileMonitor.GetInstance().SetStopMonitor(pid);
                        }
                    }
                }
            }
            catch (Exception e)
            {
                ServiceManagerApp.Singleton.Log.Warn(e.ToString());
            }
        }


        public static bool DetectOffice2013(RegistryView registryView)
        {
            bool result = false;
            try
            {
                RegistryKey baseKey = RegistryKey.OpenBaseKey(RegistryHive.LocalMachine, registryView);
                RegistryKey subKey = baseKey.OpenSubKey(@"SOFTWARE\Microsoft\Office\15.0\Common\InstallRoot", false); // Office 2013
                if (subKey != null)
                {
                    object oValue = subKey.GetValue("Path");
                    if (null != oValue)
                    {
                        string strValue = oValue.ToString();
                        if (!string.IsNullOrWhiteSpace(strValue))
                        {
                            result = true;
                        }
                    }
                }
            }
            catch (Exception ex)
            {

            }
            return result;
        }



        public static bool DetectOffice2016(RegistryView registryView)
        {
            bool result = false;
            try
            {
                RegistryKey baseKey = RegistryKey.OpenBaseKey(RegistryHive.LocalMachine, registryView);
                RegistryKey subKey = baseKey.OpenSubKey(@"SOFTWARE\Microsoft\Office\16.0\Word\InstallRoot", false); // Office 2016
                if (subKey != null)
                {
                    object oValue = subKey.GetValue("Path");
                    if (null != oValue)
                    {
                        string strValue = oValue.ToString();
                        if (!string.IsNullOrWhiteSpace(strValue))
                        {
                            result = true;
                        }
                    }
                }
            }
            catch (Exception ex)
            {

            }
            return result;
        }


        public static bool DetectOffice2019(RegistryView registryView)
        {
            bool result = false;
            try
            {
                RegistryKey baseKey = RegistryKey.OpenBaseKey(RegistryHive.LocalMachine, registryView);
                RegistryKey subKey = baseKey.OpenSubKey(@"SOFTWARE\Microsoft\Office\ClickToRun\Configuration", false);
                if (null != subKey)
                {
                    object value = subKey.GetValue("ProductReleaseIDs");
                    if (null != value)
                    {
                        string strValue = value.ToString();
                        if (string.Equals(strValue, "ProPlus2019Retail", StringComparison.CurrentCultureIgnoreCase))
                        {
                            result = true;
                        }
                        else if (string.Equals(strValue, "ProPlusRetail", StringComparison.CurrentCultureIgnoreCase))
                        {
                            result = true;
                        }
                    }
                }
            }
            catch (Exception ex)
            {

            }
            return result;
        }


        public static bool DetectOffice365(RegistryView registryView)
        {
            bool result = false;
            try
            {
                RegistryKey baseKey = RegistryKey.OpenBaseKey(RegistryHive.LocalMachine, registryView);
                RegistryKey subKey = baseKey.OpenSubKey(@"SOFTWARE\Microsoft\Office\ClickToRun\Configuration", false);
                if (null != subKey)
                {
                    object value = subKey.GetValue("ProductReleaseIDs");
                    if (null != value)
                    {
                        string strValue = value.ToString();
                        if (string.Equals(strValue, "O36​​5ProPlusRetail", StringComparison.CurrentCultureIgnoreCase))
                        {
                            result = true;
                        }
                    }
                }
            }
            catch (Exception ex)
            {

            }
            return result;
        }


        private string InvoluedRegistry(string application)
        {
            ServiceManagerApp.Singleton.Log.Info("Enter: InvoluedRegistry");
            ServiceManagerApp.Singleton.Log.Info("application: "+ application);

            string EXCEL = "Excel";
            string WORD = "Word";
            string POWERPOINT = "PowerPoint";
            string result = @"Software\Microsoft\Office\{0}\{1}\Resiliency\DocumentRecovery";

            if (DetectOffice2013(RegistryView.Registry64) || DetectOffice2013(RegistryView.Registry32))
            {
                ServiceManagerApp.Singleton.Log.Info("office 2013");
                result = result.Replace("{0}","15.0");
            }
            else if (DetectOffice2016(RegistryView.Registry64) || DetectOffice2016(RegistryView.Registry32))
            {
                ServiceManagerApp.Singleton.Log.Info("office 2016");
                result = result.Replace("{0}", "16.0");
            }
            else if (DetectOffice2019(RegistryView.Registry64) || DetectOffice2019(RegistryView.Registry32))
            {
                ServiceManagerApp.Singleton.Log.Info("office 2019");
            }
            else if (DetectOffice365(RegistryView.Registry64) || DetectOffice365(RegistryView.Registry32))
            {
                ServiceManagerApp.Singleton.Log.Info("office 365");
            }

            if (string.Equals(application, "POWERPNT.EXE", StringComparison.CurrentCultureIgnoreCase))
            {
                ServiceManagerApp.Singleton.Log.Info("POWERPNT.EXE");
                result = result.Replace("{1}", POWERPOINT);
            }
            else if (string.Equals(application, "EXCEL.EXE", StringComparison.CurrentCultureIgnoreCase))
            {
                ServiceManagerApp.Singleton.Log.Info("EXCEL.EXE");
                result = result.Replace("{1}", EXCEL);
            }
            else if (string.Equals(application, "WINWORD.EXE", StringComparison.CurrentCultureIgnoreCase))
            {
                ServiceManagerApp.Singleton.Log.Info("WINWORD.EXE");
                result = result.Replace("{1}", WORD);
            }

            ServiceManagerApp.Singleton.Log.Info("Result :"+ result);
            ServiceManagerApp.Singleton.Log.Info("Leave: InvoluedRegistry");
            return result;
        }


        private void ClearRecordOfDocumentRecovery(string involuedRegistry)
        {
            if (string.IsNullOrEmpty(involuedRegistry))
            {
                return;
            }

            ServiceManagerApp.Singleton.Log.Info("Enter: ClearRecordOfDocumentRecovery");

            RegistryKey documentRecovery = null;
            try
            {
                ServiceManagerApp.Singleton.Log.Info("OpenSubKey :" + involuedRegistry);
                documentRecovery = Registry.CurrentUser.OpenSubKey(involuedRegistry, true);

                if (null == documentRecovery)
                {
                    ServiceManagerApp.Singleton.Log.Info("documentRecovery == null");
                    return;
                }

                ServiceManagerApp.Singleton.Log.Info("GetSubKeyNames");
                string[] keyNames = documentRecovery.GetSubKeyNames();
                ServiceManagerApp.Singleton.Log.Info("keyNames: " + keyNames.ToString());

                for (int i = 0; i < keyNames.Length; i++)
                {
                    ServiceManagerApp.Singleton.Log.Info("OpenSubKey :" + keyNames[i]);
                    RegistryKey subKey = documentRecovery.OpenSubKey(keyNames[i]);

                    ServiceManagerApp.Singleton.Log.Info("GetValue :" + keyNames[i]);
                    byte[] array = (byte[])subKey.GetValue(keyNames[i]);

                    ServiceManagerApp.Singleton.Log.Info("UTF8 GetString");
                    string decoded = System.Text.Encoding.UTF8.GetString(array, 12, array.Length - 12);
                    decoded = decoded.Replace("\0", string.Empty);
                    ServiceManagerApp.Singleton.Log.Info("Decoded : " + decoded);

                    int startIndex = decoded.IndexOf(":");
                    if (-1 == startIndex)
                    {
                        ServiceManagerApp.Singleton.Log.Info("startIndex == -1");
                        continue;
                    }
                    startIndex -= 1;
                    ServiceManagerApp.Singleton.Log.Info("startIndex :" + startIndex);

                    int endIndex = -1;
                    int offset = 0;
                    for (; ; )
                    {
                        int tempIndex = decoded.IndexOf(@"\", offset + 1);
                        ServiceManagerApp.Singleton.Log.Info("tempIndex :" + tempIndex);
                        if (tempIndex != -1)
                        {
                            offset = tempIndex;
                        }
                        else
                        {
                            break;
                        }
                    }

                    endIndex = decoded.IndexOf(".", offset);
                    ServiceManagerApp.Singleton.Log.Info("endIndex :" + endIndex);
                    if (-1 == endIndex)
                    {
                        ServiceManagerApp.Singleton.Log.Info("endIndex == -1");
                        continue;
                    }

                    string filePath = decoded.Substring(startIndex, endIndex - startIndex);
                    ServiceManagerApp.Singleton.Log.Info("File Path :" + filePath);

                    string extention = TryGetFileExtention(decoded.Substring(endIndex, 5).ToLower());
                    if (string.IsNullOrEmpty(extention))
                    {
                        extention = TryGetFileExtention(decoded.Substring(endIndex, 4).ToLower());
                    }
                    ServiceManagerApp.Singleton.Log.Info("File Extention :" + extention);

                    filePath += extention;
                    ServiceManagerApp.Singleton.Log.Info("File path :" + filePath);

                    if (!File.Exists(filePath))
                    {
                        ServiceManagerApp.Singleton.Log.WarnFormat("The file {0} not exists", filePath);
                        continue;
                    }

                    int dirstatus;
                    bool filestatus;
                    if (!ServiceManagerApp.Singleton.Session.SDWL_RPM_GetFileStatus(filePath, out dirstatus, out filestatus))
                    {
                        ServiceManagerApp.Singleton.Log.Warn("Error happend in function SDWL_RPM_GetFileStatus");
                        continue;
                    }

                    ServiceManagerApp.Singleton.Log.InfoFormat("Dirstatus: {0}, Filestatus :{1}", dirstatus, filestatus);
                    if (filestatus)
                    {
                        ServiceManagerApp.Singleton.Log.Info("DeleteSubKey :" + keyNames[i]);
                        documentRecovery.DeleteSubKey(keyNames[i]);
                    }
                }

            }
            catch (Exception ex)
            {
                ServiceManagerApp.Singleton.Log.Warn(ex.Message);
            }
            finally
            {
                ServiceManagerApp.Singleton.Log.Info("Leave: ClearRecordOfDocumentRecovery");
            }
        }


        private string TryGetFileExtention(string rawExtention)
        {
            string result = string.Empty;
            switch (rawExtention)
            {
                case ".pptx":
                    result = ".pptx";
                    break;
                case ".ppsx":
                    result = ".ppsx";
                    break;
                case ".potx":
                    result = ".potx";
                    break;
                case ".ppt":
                    result = ".ppt";
                    break;

                case ".xls":
                    result = ".xls";
                    break;
                case ".xlsx":
                    result = ".xlsx";
                    break;
                case ".xlt":
                    result = ".xlt";
                    break;
                case ".xltx":
                    result = ".xltx";
                    break;
                case ".xlsb":
                    result = ".xlsb";
                    break;

                case ".doc":
                    result = ".doc";
                    break;
                case ".docx":
                    result = ".docx";
                    break;
                case ".dot":
                    result = ".dot";
                    break;
                case ".dotx":
                    result = ".dotx";
                    break;
                case ".rtf":
                    result = ".rtf";
                    break;
                default:
                    break;
            }

            return result;
        }

        /// <summary>
        /// The content that wrote into named pipe like following:
        ///  "RIGHT_VIEW=true|RIGHT_EDIT=true|RIGHT_SAVEAS=false;isByAdHoc=true|isByCentrolPolicy=fale"
        /// </summary>
        private string ParseRights(NxlFileFingerPrint fp)
        {
            Dictionary<string, bool> rights = new Dictionary<string, bool>();

            rights.Add("RIGHT_VIEW", fp.HasRight(FileRights.RIGHT_VIEW));
            rights.Add("RIGHT_EDIT", fp.HasRight(FileRights.RIGHT_EDIT));
            rights.Add("RIGHT_PRINT", fp.HasRight(FileRights.RIGHT_PRINT));
            rights.Add("RIGHT_CLIPBOARD", fp.HasRight(FileRights.RIGHT_CLIPBOARD));
            rights.Add("RIGHT_SAVEAS", fp.HasRight(FileRights.RIGHT_SAVEAS));
            rights.Add("RIGHT_DECRYPT", fp.HasRight(FileRights.RIGHT_DECRYPT));
            rights.Add("RIGHT_SCREENCAPTURE", fp.HasRight(FileRights.RIGHT_SCREENCAPTURE));
            rights.Add("RIGHT_SEND", fp.HasRight(FileRights.RIGHT_SEND));
            rights.Add("RIGHT_CLASSIFY", fp.HasRight(FileRights.RIGHT_CLASSIFY));
            rights.Add("RIGHT_SHARE", fp.HasRight(FileRights.RIGHT_SHARE));
            rights.Add("RIGHT_DOWNLOAD", fp.HasRight(FileRights.RIGHT_DOWNLOAD));
            rights.Add("RIGHT_WATERMARK", fp.HasRight(FileRights.RIGHT_WATERMARK));

            StringBuilder sb = new StringBuilder();
            // Append file rights
            foreach (var one in rights)
            {
                sb.Append(one.Key);
                sb.Append("=");
                sb.Append(one.Value);
                sb.Append("|");
            }

            // Append is adhoc or is policy
            sb.Append(";");
            sb.Append("isByAdHoc=");
            sb.Append(fp.isByAdHoc);

            sb.Append("|");
            sb.Append("isByCentrolPolicy=");
            sb.Append(fp.isByCentrolPolicy);

            sb.Append("|");
            sb.Append("hasAdminRights=");
            sb.Append(fp.hasAdminRights);

            sb.Append("|");
            sb.Append("isFromMyVault=");
            sb.Append(fp.isFromMyVault);

            sb.Append("|");
            sb.Append("isFromPorject=");
            sb.Append(fp.isFromPorject);

            sb.Append("|");
            sb.Append("isFromSystemBucket=");
            sb.Append(fp.isFromSystemBucket);

            sb.Append("|");
            sb.Append("isSaaSRouter=");
            sb.Append(ServiceManagerApp.Singleton.IsPersonRouter);

            ServiceManagerApp.Singleton.Log.Info("ParseRights: " + sb.ToString());

            return sb.ToString();
        }

    }

}
