using CustomControls.componentPages.Preference;
using CustomControls.components.CentralPolicy.model;
using CustomControls.components.ValiditySpecify.model;
using CustomControls.pages.Preference;
using CustomControls.windows;
using Microsoft.Win32;
using Newtonsoft.Json;
using ServiceManager.resources.languages;
using ServiceManager.rmservmgr.common.helper;
using ServiceManager.rmservmgr.sdk;
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Shapes;

namespace ServiceManager.rmservmgr.ui.windows
{
    /// <summary>
    /// Interaction logic for PreferenceWindow.xaml
    /// </summary>
    public partial class PreferenceWindow : Window
    {
        private ServiceManagerApp app = ServiceManagerApp.Singleton;

        private bool isValidWaterMark { get; set; }
        private string warterMark { get; set; }
        private IExpiry expiry { get; set; }

        public PreferenceWindow()
        {
            InitializeComponent();
            InitData();
            InitCommand();
        }

        private void InitData()
        {
            this.preferenceUC.ViewModel.SystemPViewModel.IsShowNotify = app.User.ShowNotifyWindow;
            this.preferenceUC.ViewModel.SystemPViewModel.IsLeaveCopy = app.User.LeaveCopy;

            // get latest warterMark and expiration
            app.User.GetDocumentPreference();

            isValidWaterMark = true;
            warterMark = app.User.Watermark.text;
            if (string.IsNullOrEmpty(warterMark))
            {
                warterMark = "$(User)$(Date)$(Break)$(Time)";
            }
            this.preferenceUC.ViewModel.DocumentPViewModel.WarterMark = warterMark;

            //get expire date from database
            sdk.Expiration expiration = app.User.Expiration;

            //get ExpireDateValue
            IExpiry expiryT = null;

            CommonUtils.SdkExpiration2ValiditySpecifyModel(expiration, out expiryT);

            expiry = expiryT;
            this.preferenceUC.ViewModel.DocumentPViewModel.Expiry = expiry;

            // set rpmPage viewmodel
            this.preferenceUC.ViewModel.RpmFolderPViewModel.RPMpath = GetUserRPMFolder();
            this.preferenceUC.ViewModel.RpmFolderPViewModel.FolderList = CommonUtils.SdkRpmPaths2CusCtrFolderItem(GetApiRPMFolder());
            if(0 == this.preferenceUC.ViewModel.RpmFolderPViewModel.MyFolderList.Count)
            {
                try
                {
                    List<string> myFolderList = app.Session.GetRPMdir(2);
                    if (0 != myFolderList.Count)
                    {
                        foreach (string myFolderPath in myFolderList)
                        {
                            try
                            {
                                int option;
                                string tags = "";
                                if (app.Session.RMP_IsSafeFolder(myFolderPath, out option, out tags))
                                {
                                    bool isAutoProtect = false;
                                    if ((option & (int)SDRmRPMFolderOption.RPMFOLDER_AUTOPROTECT) == (int)SDRmRPMFolderOption.RPMFOLDER_AUTOPROTECT)
                                    {
                                        isAutoProtect = true;
                                    }

                                    Dictionary<string, List<string>> keyValuePairs = JsonConvert.DeserializeObject<Dictionary<string, List<string>>>(tags);
                                    MyFolderItem myFolderItem = new MyFolderItem
                                    {
                                        // fix Bug 70052 - Folder path should not empty when set disk root folder as RPM folder 
                                        FolderName = string.IsNullOrEmpty(System.IO.Path.GetFileName(myFolderPath))? myFolderPath : System.IO.Path.GetFileName(myFolderPath),
                                        FolderPath = myFolderPath,
                                        AutoProtection = isAutoProtect,
                                        SelectedClassification = keyValuePairs
                                    };
                                    this.preferenceUC.ViewModel.RpmFolderPViewModel.MyFolderList.Add(myFolderItem);
                                }
                            }
                            catch (Exception ex)
                            {
                                app.Log.Error(ex);
                            }
                        }
                    }
                }
                catch (Exception ex)
                {
                    app.Log.Error(ex);
                }
            }
        }

        private string GetUserRPMFolder()
        {
            string path = "";
            try
            {
                path = app.Session.GetUserRPMFolder();
            }
            catch (Exception e)
            {
                app.Log.Error(e);
                //app.ServiceManagerWin?.ShowNotifyWindow(new app.recentNotification.MessagePara()
                //{
                //    Application = CultureStringInfo.Tray_SkyDRM,
                //    Target = "",
                //    Message = CultureStringInfo.PreferenceWin_GetRPMFailed
                //});
            }
            return path;
        }

        private List<string> GetApiRPMFolder()
        {
            List<string> rpmPaths = new List<string>();
            try
            {
                rpmPaths = app.Session.GetRPMdir();
            }
            catch (Exception e)
            {
                app.Log.Error(e);
            }

            return rpmPaths;
        }

        private void InitCommand()
        {
            CommandBinding binding;

            binding = new CommandBinding(Sys_DataCommands.Save);
            binding.Executed += Sys_Save_Executed;
            this.CommandBindings.Add(binding);

            binding = new CommandBinding(Sys_DataCommands.Apply);
            binding.Executed += Sys_Apply_Executed;
            this.CommandBindings.Add(binding);

            binding = new CommandBinding(Sys_DataCommands.Cancel);
            binding.Executed += Cancel_Executed;
            this.CommandBindings.Add(binding);

            binding = new CommandBinding(Dcm_DataCommands.Save);
            binding.Executed += Dcm_Save_Executed;
            this.CommandBindings.Add(binding);

            binding = new CommandBinding(Dcm_DataCommands.Apply);
            binding.Executed += Dcm_Apply_Executed;
            this.CommandBindings.Add(binding);

            binding = new CommandBinding(Dcm_DataCommands.Cancel);
            binding.Executed += Cancel_Executed;
            this.CommandBindings.Add(binding);

            binding = new CommandBinding(RpmP_DataCommands.Browse);
            binding.Executed += Rpm_Browse_Executed;
            this.CommandBindings.Add(binding);

            binding = new CommandBinding(RpmP_DataCommands.Apply);
            binding.Executed += Rpm_Apply_Executed;
            this.CommandBindings.Add(binding);

            binding = new CommandBinding(RpmP_DataCommands.Reset);
            binding.CanExecute += Rpm_Reset_CanExecute;
            binding.Executed += Rpm_Reset_Executed;
            this.CommandBindings.Add(binding);

            binding = new CommandBinding(RpmP_DataCommands.Cancel);
            binding.Executed += Cancel_Executed;
            this.CommandBindings.Add(binding);

            binding = new CommandBinding(RpmP_DataCommands.EditMyFolderItem);
            binding.Executed += Rpm_Edit_MyFolder_Item;
            this.CommandBindings.Add(binding);

            binding = new CommandBinding(RpmP_DataCommands.RemoveMyFolderItem);
            binding.Executed += Rpm_remove_MyFolder_Item;
            this.CommandBindings.Add(binding);

        }

        public void Rpm_remove_MyFolder_Item(object sender, ExecutedRoutedEventArgs e)
        {
            ServiceManagerApp app = ServiceManagerApp.Singleton;
            try
            {
                MyFolderItem myFolderItem = this.preferenceUC.ViewModel.RpmFolderPViewModel.MyFolderList[Convert.ToInt32(e.Parameter)];
                //Todo 
                //1 call sdk api removeRPMDir

                int option;
                string tags;
                if (app.Session.RMP_IsSafeFolder(myFolderItem.FolderPath, out option, out tags))
                {
                    string errorStr;
                    app.Session.RPM_RemoveDir(myFolderItem.FolderPath, out errorStr);
                }
               
                //3 remove this item from memory list
                this.preferenceUC.ViewModel.RpmFolderPViewModel.MyFolderList.RemoveAt((int)e.Parameter);
            }
            catch (ResourceIsInUseException ex)
            {
                app.Log.Error(ex);
                app.ServiceManagerWin?.ShowNotifyWindow(new app.recentNotification.MessagePara()
                {
                    Application = CultureStringInfo.Tray_SkyDRM,
                    Target = "",
                    Message = ex.Message
                });
               
            }
            catch (Exception ex)
            {
                app.Log.Error(ex);
                app.ServiceManagerWin?.ShowNotifyWindow(new app.recentNotification.MessagePara()
                {
                    Application = CultureStringInfo.Tray_SkyDRM,
                    Target = "",
                    Message = CultureStringInfo.PreferenceWin_Remove_RPMFailed
                });

            }
        }

        public void Rpm_Edit_MyFolder_Item(object sender, ExecutedRoutedEventArgs e)
        {
            MyFolderItem myFolderItem = this.preferenceUC.ViewModel.RpmFolderPViewModel.MyFolderList[Convert.ToInt32(e.Parameter)];
            CustomControls.components.CentralPolicy.model.Classification[] classifications = new CustomControls.components.CentralPolicy.model.Classification[0];
            ServiceManagerApp app = ServiceManagerApp.Singleton;
            int sbId = app.Session.User.GetSystemProjectId();
            string sbTenant = app.Session.User.GetSystemProjectTenantId();
            bool sbIsEnableAdhoc = app.Session.User.IsEnabledAdhocForSystemBucket();
            ProjectClassification[] classifications_raw = app.Session.User.GetProjectClassification(sbTenant);
            classifications = SdkTag2CustomControlTag(classifications_raw);
            MyFolderConfigWindow myFolderConfigWindow = new MyFolderConfigWindow(myFolderItem, classifications);
            myFolderConfigWindow.OnClickApply += (MyFolderItem old, MyFolderItem newOne) =>
            {
                try
                {
                    //if (IsSystemFolderPath(newOne.FolderPath) || IsSpecialFolderPath(newOne.FolderPath) || IsDefaultRPMFolder(newOne.FolderPath))
                    //{
                    //    app.ServiceManagerWin?.ShowNotifyWindow(new app.recentNotification.MessagePara()
                    //    {
                    //        Application = CultureStringInfo.Tray_SkyDRM,
                    //        Target = "",
                    //        Message = CultureStringInfo.PreferenceWin_SetRPMFailed
                    //    });
                    //    return false;
                    //}

                    bool fixedDriverFolder = IsFixedDriverFolder(newOne.FolderPath);
                    if (!fixedDriverFolder)
                    {
                        app.ServiceManagerWin?.ShowNotifyWindow(new app.recentNotification.MessagePara()
                        {
                            Application = CultureStringInfo.Tray_SkyDRM,
                            Target = "",
                            Message = CultureStringInfo.PreferenceWin_SetRPMFailed
                        });
                        return false;
                    }

                    int oldOption;
                    string oldTags;
                    if (app.Session.RMP_IsSafeFolder(old.FolderPath, out oldOption, out oldTags))
                    {
                        //call SDK API removeRPMdir to remove old one My folder RPM
                        string errorStr;
                        app.Session.RPM_RemoveDir(old.FolderPath, out errorStr);
                    }

                    try
                    {
                        string strTags = JsonConvert.SerializeObject(newOne.SelectedClassification);
                        if (newOne.AutoProtection)
                        {
                            //call SDK API AddRPMdir
                            app.Session.RPM_AddDir(newOne.FolderPath, (int)(SDRmRPMFolderOption.RPMFOLDER_NORMAL |
                                                            SDRmRPMFolderOption.RPMFOLDER_AUTOPROTECT |
                                                            SDRmRPMFolderOption.RPMFOLDER_EXT |
                                                            SDRmRPMFolderOption.RPMFOLDER_MYFOLDER|
                                                            SDRmRPMFolderOption.RPMFOLDER_OVERWRITE
                                                            ), strTags);
                        }
                        else
                        {
                            // reset RPM folder , remove auto protection flag
                            app.Session.RPM_AddDir(newOne.FolderPath, (int)(SDRmRPMFolderOption.RPMFOLDER_NORMAL |
                                                         SDRmRPMFolderOption.RPMFOLDER_EXT |
                                                         SDRmRPMFolderOption.RPMFOLDER_MYFOLDER|
                                                         SDRmRPMFolderOption.RPMFOLDER_OVERWRITE
                                                         ), strTags);

                        }
                    }
                    catch (Exception eex)
                    {
                        //fix bug if add newone folder as myfolder, reset oldone folder as a myfolder
                        app.Session.RPM_AddDir(old.FolderPath, oldOption, oldTags);
                        throw eex;
                    }

                    //update MyFolderItem in memory
                    old.FolderName = string.IsNullOrWhiteSpace(newOne.FolderName) ? newOne.FolderPath : newOne.FolderName;
                    old.FolderPath = newOne.FolderPath;
                    old.AutoProtection = newOne.AutoProtection;
                    old.SelectedClassification = newOne.SelectedClassification;

                    return true;
                }
                catch (ResourceIsInUseException ex)
                {
                    app.Log.Error(ex);
                    app.ServiceManagerWin?.ShowNotifyWindow(new app.recentNotification.MessagePara()
                    {
                        Application = CultureStringInfo.Tray_SkyDRM,
                        Target = "",
                        Message = ex.Message
                    });
                    return false;
                }
                catch (Exception ex)
                {
                    app.Log.Error(ex);
                    //app.ShowBalloonTip(CultureStringInfo.PreferenceWin_SetRPMFailed);
                    app.ServiceManagerWin?.ShowNotifyWindow(new app.recentNotification.MessagePara()
                    {
                        Application = CultureStringInfo.Tray_SkyDRM,
                        Target = "",
                        Message = CultureStringInfo.PreferenceWin_SetRPMFailed
                    });
                    return false;
                }
            };
            myFolderConfigWindow.ShowDialog();
        }


        #region Command Excute
        private void Sys_Save_Executed(object sender, ExecutedRoutedEventArgs e)
        {
            if (this.preferenceUC.ViewModel.SystemPViewModel.BtnApplyIsEnable)
            {
                SaveSystemPre();
            }
            this.Close();
        }
        private void Sys_Apply_Executed(object sender, ExecutedRoutedEventArgs e)
        {
            SaveSystemPre();
            this.preferenceUC.ViewModel.SystemPViewModel.BtnApplyIsEnable = false;
            // if checkbox changed, in SystemPViewModel will reset BtnApplyIsEnable is true.
        }

        private void Cancel_Executed(object sender, ExecutedRoutedEventArgs e)
        {
            this.Close();
        }

        private void Dcm_Save_Executed(object sender, ExecutedRoutedEventArgs e)
        {
            if (this.preferenceUC.ViewModel.DocumentPViewModel.BtnApplyIsEnable)
            {
                SaveDocumentPre();
            }
            this.Close();
        }
        private void Dcm_Apply_Executed(object sender, ExecutedRoutedEventArgs e)
        {
            SaveDocumentPre();
            this.preferenceUC.ViewModel.DocumentPViewModel.BtnApplyIsEnable = false;
        }

        public Classification[] SdkTag2CustomControlTag(ProjectClassification[] sdkTag)
        {
            if (sdkTag == null || sdkTag.Length == 0)
            {
                return new Classification[0];
            }

            Classification[] tags = new Classification[sdkTag.Length];
            for (int i = 0; i < sdkTag.Length; i++)
            {
                tags[i].name = sdkTag[i].name;
                tags[i].isMultiSelect = sdkTag[i].isMultiSelect;
                tags[i].isMandatory = sdkTag[i].isMandatory;
                tags[i].labels = sdkTag[i].labels;
            }
            return tags;
        }

        public static bool IsSystemFolderPath(string path)
        {
            try
            {
                bool result = false;
                foreach (var item in Enum.GetValues(typeof(Environment.SpecialFolder)))
                {
                    bool isSucceed = Enum.TryParse(item.ToString(), out Environment.SpecialFolder value);
                    if (isSucceed)
                    {
                        result = path.Equals(Environment.GetFolderPath(value), StringComparison.OrdinalIgnoreCase);
                        if (result)
                        {
                            break;
                        }
                    }
                }
                if (!result)
                {
                    result = path.Equals(@"c:\", StringComparison.OrdinalIgnoreCase);
                }
                return result;
            }
            catch (Exception ex)
            {
                throw ex;
            }
        }

        public static bool IsSpecialFolderPath(string path)
        {
            try
            {
                bool result = false;
                if (path.StartsWith(Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData) + @"\Nextlabs\SkyDRM", StringComparison.OrdinalIgnoreCase)
                    || path.StartsWith(Environment.GetFolderPath(Environment.SpecialFolder.Windows), StringComparison.OrdinalIgnoreCase)
                    || path.StartsWith(Environment.GetFolderPath(Environment.SpecialFolder.SystemX86), StringComparison.OrdinalIgnoreCase))
                {
                    result = true;
                }
                return result;
            }
            catch (Exception ex)
            {
                throw ex;
            }
        }

        private bool IsDefaultRPMFolder(string folderPath)
        {
            try
            {
                //fix Bug 69768 - Should can't set the default RPM folder "Intermediate" to MySkyDRM and give a friendly message
                string mWorkingDir = string.Empty;
                RegistryKey amazon = Registry.LocalMachine.OpenSubKey(@"Software\Amazon\MachineImage");
                var AMI = amazon?.GetValue("AMIName");
                bool IsAppStream = false;
                if (AMI != null)
                {
                    if (!string.IsNullOrWhiteSpace(AMI.ToString()))
                    {
                        IsAppStream = true;
                    }
                }
                amazon?.Close();

                if (IsAppStream)
                {
                    // Init basic frame, write all files into C:\ProgramData\Nextlabs\SkyDRM
                    mWorkingDir = @"C:\ProgramData\Nextlabs\SkyDRM";
                }
                else
                {
                    // Init basic frame, write all files into User/LocalApp/SkyDRM/
                    mWorkingDir = System.Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData) + @"\Nextlabs\SkyDRM";
                }

                string mDef_RPM_Dir = string.Empty;
                mDef_RPM_Dir = mWorkingDir + "\\" + "Intermediate";

                if (folderPath.StartsWith(mDef_RPM_Dir, StringComparison.CurrentCultureIgnoreCase))
                {
                    return true;
                }
                return false;
            }
            catch (Exception ex)
            {
                throw ex;
            }
        }

        private void Rpm_Browse_Executed(object sender, ExecutedRoutedEventArgs e)
        {
            CustomControls.components.CentralPolicy.model.Classification[] classifications = new CustomControls.components.CentralPolicy.model.Classification[0];
            ServiceManagerApp app = ServiceManagerApp.Singleton;
            int sbId = app.Session.User.GetSystemProjectId();
            string sbTenant = app.Session.User.GetSystemProjectTenantId();
            bool sbIsEnableAdhoc = app.Session.User.IsEnabledAdhocForSystemBucket();
            ProjectClassification[] classifications_raw = app.Session.User.GetProjectClassification(sbTenant);
            classifications = SdkTag2CustomControlTag(classifications_raw);

            MyFolderConfigWindow myFolderConfigWindow = new MyFolderConfigWindow(classifications);
            myFolderConfigWindow.OnClickApply += (MyFolderItem old, MyFolderItem newOne) =>
            {
                try
                {
                    //if (IsSystemFolderPath(newOne.FolderPath) || IsSpecialFolderPath(newOne.FolderPath) || IsDefaultRPMFolder(newOne.FolderPath))
                    //{
                    //    app.ServiceManagerWin?.ShowNotifyWindow(new app.recentNotification.MessagePara()
                    //    {
                    //        Application = CultureStringInfo.Tray_SkyDRM,
                    //        Target = "",
                    //        Message = CultureStringInfo.PreferenceWin_SetRPMFailed
                    //    });
                    //    return false;
                    //}

                    bool fixedDriverFolder = IsFixedDriverFolder(newOne.FolderPath);
                    if (!fixedDriverFolder)
                    {
                        app.ServiceManagerWin?.ShowNotifyWindow(new app.recentNotification.MessagePara()
                        {
                            Application = CultureStringInfo.Tray_SkyDRM,
                            Target = "",
                            Message = CultureStringInfo.PreferenceWin_SetRPMFailed
                        });
                        return false;
                    }

                    int option;
                    string tags;
                    if (app.Session.RMP_IsSafeFolder(newOne.FolderPath, out option, out tags))
                    {
                        //call SDK API removeRPMdir to remove old one My folder RPM
                        string errorStr;
                        app.Session.RPM_RemoveDir(newOne.FolderPath, out errorStr);
                    }

                    string strTags = JsonConvert.SerializeObject(newOne.SelectedClassification);
                    if (newOne.AutoProtection)
                    {
                        //call SDK API AddRPMdir
                        app.Session.RPM_AddDir(newOne.FolderPath, (int)(SDRmRPMFolderOption.RPMFOLDER_NORMAL |
                                                        SDRmRPMFolderOption.RPMFOLDER_AUTOPROTECT |
                                                        SDRmRPMFolderOption.RPMFOLDER_EXT|
                                                        SDRmRPMFolderOption.RPMFOLDER_MYFOLDER|
                                                        SDRmRPMFolderOption.RPMFOLDER_OVERWRITE
                                                        ), strTags);
                    }
                    else
                    {
                        // don't need auto protection flag
                        app.Session.RPM_AddDir(newOne.FolderPath, (int)(SDRmRPMFolderOption.RPMFOLDER_NORMAL |
                                                        SDRmRPMFolderOption.RPMFOLDER_EXT|
                                                        SDRmRPMFolderOption.RPMFOLDER_MYFOLDER|
                                                        SDRmRPMFolderOption.RPMFOLDER_OVERWRITE
                                                        ), strTags);
                    }

                    //add this item into memory list
                    if(string.IsNullOrWhiteSpace(newOne.FolderName))
                    {
                        newOne.FolderName = newOne.FolderPath;
                    }
                    this.preferenceUC.ViewModel.RpmFolderPViewModel.MyFolderList.Add(newOne);
                    return true;
                }
                catch (Exception ex)
                {
                    app.Log.Error(ex);
                    //app.ShowBalloonTip(CultureStringInfo.PreferenceWin_SetRPMFailed);
                    app.ServiceManagerWin?.ShowNotifyWindow(new app.recentNotification.MessagePara()
                    {
                        Application = CultureStringInfo.Tray_SkyDRM,
                        Target = "",
                        Message = CultureStringInfo.PreferenceWin_SetRPMFailed
                    });
                    return false;
                }
            };
            myFolderConfigWindow.ShowDialog();

            ////init Dir.
            //string dftRootFolder = this.preferenceUC.ViewModel.RpmFolderPViewModel.RPMpath;
            //System.Windows.Forms.FolderBrowserDialog dlg = new System.Windows.Forms.FolderBrowserDialog();
            //dlg.Description = "Select a Folder";
            //if (Directory.Exists(dftRootFolder))
            //{
            //    dlg.SelectedPath = dftRootFolder;
            //}
            //var result = dlg.ShowDialog();
            //if (result == System.Windows.Forms.DialogResult.OK || result == System.Windows.Forms.DialogResult.Yes)
            //{
            //    // Will occur PathTooLong exception when dlg.SelectedPath is > 260
            //    try
            //    {
            //        this.preferenceUC.ViewModel.RpmFolderPViewModel.RPMpath = dlg.SelectedPath;
            //    }
            //    catch (Exception ex)
            //    {
            //        app.Log.Error("Error in SaveSystemPre:", ex);
            //        app.ServiceManagerWin?.ShowNotifyWindow(new app.recentNotification.MessagePara()
            //        {
            //            Application = CultureStringInfo.Tray_SkyDRM,
            //            Target = "",
            //            Message = CultureStringInfo.PreferenceWin_SetRPMFailed
            //        });
            //    }
            //}
        }
        private void Rpm_Apply_Executed(object sender, ExecutedRoutedEventArgs e)
        {
            ApplyUserRpmFolder();
            this.preferenceUC.ViewModel.RpmFolderPViewModel.BtnApplyIsEnable = false;
            // if textBox text change, in RpmFolderPViewModel will reset BtnApplyIsEnable is true.
        }
        private void Rpm_Reset_Executed(object sender, ExecutedRoutedEventArgs e)
        {
            ResetApiRpmFolder();
            this.preferenceUC.ViewModel.RpmFolderPViewModel.FolderList = CommonUtils.SdkRpmPaths2CusCtrFolderItem(GetApiRPMFolder());
        }
        private void Rpm_Reset_CanExecute(object sender, CanExecuteRoutedEventArgs e)
        {
            if (this.preferenceUC.ViewModel.RpmFolderPViewModel.FolderList.All(x => !x.IsChecked))
            {
                e.CanExecute = false;
            }
            else
            {
                e.CanExecute = true;
            }
        }
        #endregion

        private void SaveSystemPre()
        {
            app.User.ShowNotifyWindow = (bool)this.preferenceUC.ViewModel.SystemPViewModel.IsShowNotify;

            app.User.LeaveCopy = (bool)this.preferenceUC.ViewModel.SystemPViewModel.IsLeaveCopy;          
        }
        
        private void SaveDocumentPre()
        {
            sdk.WaterMarkInfo markInfo = new sdk.WaterMarkInfo();
            markInfo.text = warterMark;
            app.User.Watermark = markInfo;

            sdk.Expiration expiration = new sdk.Expiration();
            CommonUtils.ValiditySpecifyModel2SdkExpiration(out expiration, expiry);
            app.User.Expiration = expiration;

            app.User.UpdateDocumentPreference();
        }

        private void ApplyUserRpmFolder()
        {
            try
            {
                string oldRPM = GetUserRPMFolder();
                string newRPM = this.preferenceUC.ViewModel.RpmFolderPViewModel.RPMpath;

                if (string.IsNullOrEmpty(newRPM))
                {
                    if (!string.IsNullOrEmpty(oldRPM))
                    {
                        // If original user rpm is not empty, remove it
                        app.Session.RemoveUserRPMFolder(oldRPM, false);
                    }
                }
                else
                {
                    if (Directory.Exists(newRPM))
                    {
                        if (!FilterSystemPath.IsSpecialFolderPath(newRPM) && !FilterSystemPath.IsSystemFolderPath(newRPM))
                        {
                            if (!string.IsNullOrEmpty(oldRPM))
                            {
                                // If original user rpm is not empty, remove it
                                app.Session.RemoveUserRPMFolder(oldRPM, false);
                            }
                            // add new user rpm
                            app.Session.AddUserRPMFolder(newRPM);
                        }
                        else
                        {
                            app.ServiceManagerWin?.ShowNotifyWindow(new app.recentNotification.MessagePara()
                            {
                                Application = CultureStringInfo.Tray_SkyDRM,
                                Target = "",
                                Message = CultureStringInfo.PreferenceWin_SetRPMSystemFolder
                            });
                        }
                    }
                    else
                    {
                        app.ServiceManagerWin?.ShowNotifyWindow(new app.recentNotification.MessagePara()
                        {
                            Application = CultureStringInfo.Tray_SkyDRM,
                            Target = "",
                            Message = CultureStringInfo.PreferenceWin_SetRPMPathInvalid
                        });
                    }
                }
            }
            catch (Exception e)
            {
                app.Log.Error("Error in SaveSystemPre:", e);
                app.ServiceManagerWin?.ShowNotifyWindow(new app.recentNotification.MessagePara()
                {
                    Application = CultureStringInfo.Tray_SkyDRM,
                    Target = "",
                    Message = CultureStringInfo.PreferenceWin_SetRPMFailed
                });
            }
        }

        private void ResetApiRpmFolder()
        {
            var oldList = this.preferenceUC.ViewModel.RpmFolderPViewModel.FolderList;
            foreach (var item in oldList)
            {
                string errorMsg = "";
                try
                {
                    if (item.IsChecked)
                    {
                        app.Session.RPM_RemoveDir(item.FolderPath, out errorMsg);
                    }
                }
                catch (Exception e)
                {
                    string msg = string.Format(CultureStringInfo.PreferenceWin_ResetRPMFailed_For_Common, item.FolderPath);
                    msg = msg+" "+ errorMsg;

                    // Insert log into db
                    app.MyRecentNotifications?.UpdateOrInsertMessage(BuildNotifyJson(item.FolderName, msg));
                    // Bubble
                    app.Session.SDWL_RPM_NotifyMessage("SkyDRM", item.FolderName, msg, 1, "Reset RPM Folder", 0);
                }
            }
        }

        private string BuildNotifyJson(string target, string msg)
        {
            string operat = "Remove RPM Folder";
            string json = "{ \"application\" : \"" + CultureStringInfo.Tray_SkyDRM
                            + "\", \"fileStatus\" : " + 0 // Unknown
                            + ", \"message\" : \"" + msg
                            + "\", \"msgtype\" : " + 0    // Log
                            + ", \"operation\" : \"" + operat
                            + "\", \"result\" : " + 0     // Failed
                            + ", \"target\" : \"" + target
                            + "\"}";

            return json;
        }

        private bool IsFixedDriverFolder(string path)
        {
            FileInfo f = new FileInfo(path);
            string drive = System.IO.Path.GetPathRoot(f.FullName);
            var allDrivers = DriveInfo.GetDrives().Where(x => x.DriveType != System.IO.DriveType.Fixed);
            if (allDrivers == null || allDrivers.ToList().Count == 0)
            {
                return true;
            }

            bool result = allDrivers.ToList().Any(x => x.Name.Equals(drive, StringComparison.CurrentCultureIgnoreCase));
            if (result)
            {
                return false;
            }
            else
            {
                return true;
            }
        }

        #region Window EventHandler
        private void WarterMarkChanged(object sender, RoutedPropertyChangedEventArgs<CustomControls.components.WarterMarkChangedEventArgs> e)
        {
            var value = e.NewValue;
            if (value.IsValid)
            {
                isValidWaterMark = true;
                warterMark = value.WarterMarkValue;
                this.preferenceUC.ViewModel.DocumentPViewModel.BtnSaveIsEnable = true;
                this.preferenceUC.ViewModel.DocumentPViewModel.BtnApplyIsEnable = true;
            }
            else
            {
                isValidWaterMark = false;
                this.preferenceUC.ViewModel.DocumentPViewModel.BtnSaveIsEnable = false;
                this.preferenceUC.ViewModel.DocumentPViewModel.BtnApplyIsEnable = false;
            }
        }

        private void ExpiryValueChanged(object sender, RoutedPropertyChangedEventArgs<ExpiryValueChangedEventArgs> e)
        {
            var value = e.NewValue;
            expiry = value.Expiry;
            if (isValidWaterMark)
            {
                this.preferenceUC.ViewModel.DocumentPViewModel.BtnApplyIsEnable = true;
            }
        }

        private void Window_KeyUp(object sender, KeyEventArgs e)
        {
            if (e.Key == Key.Escape)
            {
                this.Close();
            }
        }
        #endregion
    }
}
