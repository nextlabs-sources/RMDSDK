using Microsoft.Win32;
using Newtonsoft.Json;
using ServiceManager.resources.languages;
using ServiceManager.rmservmgr.common.components;
using ServiceManager.rmservmgr.common.helper;
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using static ServiceManager.rmservmgr.common.components.NamedPipeClient;

namespace ServiceManager.rmservmgr.common.filemonitor
{
    // This used to monitor file if is edited by trusted application with os_rmx.
    public class FileMonitor
    {
        private const string OPERATE_Edit = "Edit";
        private static FileMonitor instance;
        private static readonly object locker = new object();

        // monitor list.
        private readonly List<IMonitoredItem> monitorList = new List<IMonitoredItem>();

        // monitor interval,
        private int monitorIntervalSec;

        FileMonitor()
        {
            // Should can config this value, read from  registry.
            monitorIntervalSec = 3;
        }

        public static FileMonitor GetInstance()
        {
            if(instance == null)
            {
                lock (locker)
                {
                    if(instance == null)
                    {
                        instance = new FileMonitor();
                    }
                }
            }

            return instance;
        }

        public void StartMonitor(IMonitoredItem item)
        {
            // Fix bug 59378 that will multiple trigger service sending Open file event notification when Edit/Save office file.
            if (IsItemExist(item))
            {
                return;
            }

            AddToMonitorList(item);

            // Even though have submitted to thread pool, don't means the task will must be executed immediately.
            ThreadPool.QueueUserWorkItem(new WaitCallback(InnerMonitor), item);
        }

        private void InnerMonitor(object obj)
        {
            IMonitoredItem item = obj as IMonitoredItem;

            // Maybe the process quit(file close) immediately before execte monitor task.
            if(!IsItemExist(item))
            {
                return;
            }

            try
            {
                SleepBeforeAccessFile();

                DoInit(item);

                // cycle polling
                while (!item.IsStopMonitor)
                {
                    // sleep interval
                    Thread.Sleep(monitorIntervalSec * 1000);

                    if (IsFileModified(item))
                    {
                        if(item.IsAppAllowEdit && item.IsFileAllowEdit)
                        {
                            // Do Edit&Save, excluding pdf file, for pdf file, we only edit&save it when process exit(fix bug 59278).
                            //if (!IsPdfFile(item.FilePath))
                            //{
                            //    DoEditSave(item);
                            //}


                            // Do DoEditSave after process has been closed
                            //if (IsNonRPMFolderFile(item.FilePath))
                            //{
                            //    DoEditSave(item);
                            //} 
                        }
                        else
                        {
                            ServiceManagerApp.Singleton.Log.Info("Deny edit.");

                            // popup bubble in main thread.
                            ServiceManagerApp.Singleton.Dispatcher.Invoke(() =>
                            {   
                                ServiceManagerApp.Singleton.MyRecentNotifications?.UpdateOrInsertMessage(BuildNotifyJson(item));
                            });
                            Console.WriteLine("Deny edit bubble!");

                            // Update lastModified.
                            UpdateLastModified(item);
                        }
                    }
                }

                // means app process exit/ file close
                if (item.IsStopMonitor)
                {
                    // Also should perform Edit&Save if file has eidt rights and has been modified when process exit.
                    if(item.IsAppAllowEdit && item.IsFileAllowEdit)
                    {
                        string nxlFilePath = RPMEdit_FindMap(item.FilePath);
                        if (IsFileModified(item))
                        {
                            if (0 == DoEditSave(item))
                            {
                                NotifyRMDAppToSyncFile(true, nxlFilePath);
                            }
                        }
                        else
                        {
                            NotifyRMDAppToSyncFile(false, nxlFilePath);
                        }
                    }

                    RemoveFromMonitorList(item);

                    DoCleanUp(item);
                }

            }
            catch (Exception e)
            {
                ServiceManagerApp.Singleton.Log.Warn(e.ToString());
            }
        }

        // Set the stop monitor flag when receive the event that application process exit
        public void SetStopMonitor(uint pid)
        {
            lock (this)
            {
                // Stop the monitor of these files opened by the process(pid). 
                foreach(var one in monitorList)
                {
                    if(one.Pid == pid)
                    {
                        one.IsStopMonitor = true;
                    }
                }
            }
        }

        // Judge if the nxl file is opened from RPM folder or not.
        // Now to judge by registry.
        private bool IsNonRPMFolderFile(string filePath)
        {
            bool ret = false;

            Microsoft.Win32.RegistryKey sessionKey = null;
            try
            {
                sessionKey = Registry.CurrentUser.OpenSubKey(@"Software\NextLabs\SkyDRM\Session");
                if(sessionKey != null)
                {
                    string nxlFilePath = (string)sessionKey.GetValue(filePath);
                    if (!string.IsNullOrEmpty(nxlFilePath))
                    {
                        ret = true;
                    }
                }
            }
            catch (Exception e)
            {
                ServiceManagerApp.Singleton.Log.Warn(e.ToString());
            }
            finally
            {
                if(sessionKey != null)
                {
                    sessionKey.Close();
                }
            }

            return ret;
        }

        private uint DoEditSave(IMonitoredItem item)
        {
            ServiceManagerApp.Singleton.Log.Info("Do edit save.");
            var ret = ServiceManagerApp.Singleton.Session.RPM_EditSaveFile(item.FilePath);
            if (ret == 0)
            {
                // Update lastModified time after save successfully.
                UpdateLastModified(item);

                // send edit log
                ServiceManagerApp.Singleton.Log.Info("Send edit save.");
                ServiceManagerApp.Singleton.Session.User.AddLog(item.FilePath, sdk.NxlOpLog.Edit, true);
            }
            else
            {
                ServiceManagerApp.Singleton.Log.Info("Edit save failed.");
            }
            return ret;
        }

        public class EditCallBack
        {
            public bool IsEdit { get; set; }
            public string LocalPath { get; set; }
            public EditCallBack(bool ie, string lp)
            {
                this.IsEdit = ie;
                this.LocalPath = lp;
            }
        }

        private void NotifyRMDAppToSyncFile(bool b,string nxlFilePath)
        {
            // Notify RMD to update file status and do sync.
            try
            {
                if (string.IsNullOrEmpty(nxlFilePath))
                {
                    return;
                }

                Bundle<EditCallBack> bundle = new Bundle<EditCallBack>()
                {
                    Intent = Intent.SyncFileAfterEdit,
                    obj = new EditCallBack(b, nxlFilePath)

                };
                string json = JsonConvert.SerializeObject(bundle);
                NamedPipeClient.Start(json);
            }
            catch (Exception ex)
            {

            }
        }

        private string RPMEdit_FindMap(string filepath)
        {
            string result = string.Empty;
            try
            {
                string subKey = @"Software\Nextlabs\SkyDRM\Session";
                RegistryKey registryKey = Registry.CurrentUser.OpenSubKey(subKey, RegistryKeyPermissionCheck.ReadSubTree);
                result = (string)registryKey.GetValue(filepath);
            }
            catch (Exception ex)
            {

            }
            return result;
        }

        //
        // Sleeping for 3s before access the file, For fix bug 59334;
        // Since the trusted target application maybe is accessing the file and the drive is decrypting at this time,
        // if nxrmtray(trusted process) also access it immediately, which may result in data disorder of the drive decryption.
        //
        private void SleepBeforeAccessFile()
        {
            Thread.Sleep(monitorIntervalSec * 1000);
        }

        private void DoInit(IMonitoredItem item)
        {
            if(string.IsNullOrEmpty(item.FilePath))
            {
                return;
            }
            // Init lastModified time
            System.IO.FileInfo fileInfo = new System.IO.FileInfo(item.FilePath);
            item.LastModified = fileInfo.LastWriteTime;

            // send view/open log
            ServiceManagerApp.Singleton.Session.User.AddLog(item.FilePath, sdk.NxlOpLog.View, true);
        }

        private void DoCleanUp(IMonitoredItem item)
        {
            // Delete the temp file(Fix bug 59460) when process exit.
            // Note: para "bDeleteSource" is true and "exitmode" is 2 (exit and not save).
            if (ServiceManagerApp.Singleton.Session.RPM_EditSaveFile(item.FilePath, "", true, 2) != 0)
            {
                ServiceManagerApp.Singleton.Log.Info("Delete temp file failed, the file is:" + item.FilePath);
            } else
            {
                ServiceManagerApp.Singleton.Log.Info("Delete temp file succeed, the file is:" + item.FilePath);
            }

            // Clean up the application document history for os_rmx.
            ClearRegistryFileRecord.ClearAppHistoryDocs(ExtractAppName(item.AppName));
        }

        private bool IsFileModified(IMonitoredItem item)
        {
            if (string.IsNullOrEmpty(item.FilePath))
                return false;

            bool bModified;

            DateTime old_lastModified = item.LastModified;
            System.IO.FileInfo fileInfo = new System.IO.FileInfo(item.FilePath);

            // The file if exist
            if (!fileInfo.Exists)
            {
                Console.WriteLine("++++++++++++++++++ file not exist!");
                return false;
            }

            DateTime new_lastModified = fileInfo.LastWriteTime;

            bModified = !DateTime.Equals(old_lastModified, new_lastModified);

            return bModified;
        }


        private void UpdateLastModified(IMonitoredItem item)
        {
            System.IO.FileInfo fileInfo = new System.IO.FileInfo(item.FilePath);
            DateTime new_lastModified = fileInfo.LastWriteTime;
            item.LastModified = new_lastModified;
        }

        #region monitro list operation
        public void AddToMonitorList(IMonitoredItem item)
        {
            lock (this)
            {
                monitorList.Add(item);
            }
        }

        public void RemoveFromMonitorList(IMonitoredItem item)
        {
            lock (this)
            {
                IMonitoredItem find = null;
                foreach(var one in monitorList)
                {
                    if(one.Pid == item.Pid && one.FilePath == item.FilePath)
                    {
                        find = one;
                        break;
                    }
                }

                if(find != null)
                {
                    ServiceManagerApp.Singleton.Log.Info("Remove the file from monitor list, the file is:" + item.FilePath);
                    monitorList.Remove(find);
                }
            }
        }

        public void ClearMonitorQueue()
        {
            lock (this)
            {
                monitorList.Clear();
            }
        }

        public bool IsItemExist(IMonitoredItem item)
        {
            lock (this)
            {
                foreach(var one in monitorList)
                {
                    if(one.Pid == item.Pid && one.FilePath == item.FilePath)
                    {
                        return true;
                    }
                }

                return false;
            }
        }

        #endregion // monitor list operation

        private string BuildNotifyJson(IMonitoredItem item)
        {
            string json = "{ \"application\" : \"" + ExtractAppName(item.AppName)
                            + "\", \"fileStatus\" : " + 0 // Unknown
                            + ", \"message\" : \"" + CultureStringInfo.FileMonitor_Deny_Edit
                            + "\", \"msgtype\" : " + 1    // Popup bubble
                            + ", \"operation\" : \"" + OPERATE_Edit
                            + "\", \"result\" : " + 0     // Failed
                            + ", \"target\" : \"" + Path.GetFileName(item.FilePath)
                            + "\"}";

            return json;
        }

        private string ExtractAppName(string appPath)
        {
            if (!string.IsNullOrEmpty(appPath) && appPath.Contains("\\") && !appPath.EndsWith("\\"))
            {
                int index = appPath.LastIndexOf("\\");

                return appPath.Substring(index + 1);
            }

            return appPath;
        }

        private bool IsPdfFile(string filePath)
        {
            if (string.IsNullOrEmpty(filePath))
            {
                return false;
            }

            var ext = Path.GetExtension(filePath);
            if (!string.IsNullOrEmpty(ext) && ext.ToLower().Equals(".pdf"))
            {
                return true;
            }

            return false;
        }

    }
}
