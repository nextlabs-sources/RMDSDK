using Microsoft.Win32;
using ServiceManager.resources.languages;
using ServiceManager.rmservmgr.common.components;
using ServiceManager.rmservmgr.common.helper;
using ServiceManager.rmservmgr.ui.windows.notifyWindow.view;
using ServiceManager.rmservmgr.ui.windows.notifyWindow.viewModel;
using ServiceManager.rmservmgr.ui.windows.serviceManager.helper;
using ServiceManager.rmservmgr.ui.windows.serviceManager.model;
using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.ComponentModel;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Controls.Primitives;
using System.Windows.Input;
using static ServiceManager.rmservmgr.ui.components.CustomSearchBox;

namespace ServiceManager.rmservmgr.ui.windows.serviceManager.viewModel
{
    public class ServiceManagerViewModel : INotifyPropertyChanged
    {
        private readonly ServiceManagerApp app = ServiceManagerApp.Singleton;
        private readonly ServiceManagerWin parentWin;
        
        // for Notify Event
        private bool isTriggerNotifyEvent = false;
        // for show Notify window
        private readonly List<NotifyWin> notifyWins = new List<NotifyWin>();
        // for open RMD
        private string rmdExcutePath;

        #region DataBinding
        private string userName;
        private string avatarText;
        private string avatarTextColor;
        private string avatarBackground;
        private bool isNetworkAvailable;
        private Visibility openRmdVisibility;
        private string isCheckedAppItem;
        private readonly ObservableCollection<Notifications> notifyList = new ObservableCollection<Notifications>();
        private readonly ObservableCollection<Applications> applicationList = new ObservableCollection<Applications>();
        // for search
        private readonly ObservableCollection<Applications> copyAppList = new ObservableCollection<Applications>();

        private bool? isAllChecked;
        private string isAllCheckedText;

        public string UserName { get => userName; set { userName = value; OnPropertyChanged("UserName"); } }
        public string AvatarText { get => avatarText; set { avatarText = value; OnPropertyChanged("AvatarText"); } }
        public string AvatarTextColor { get => avatarTextColor; set { avatarTextColor = value; OnPropertyChanged("AvatarTextColor"); } }
        public string AvatarBackground { get => avatarBackground; set { avatarBackground = value; OnPropertyChanged("AvatarBackground"); } }
        public bool IsNetworkAvailable { get => isNetworkAvailable; set { isNetworkAvailable = value; OnPropertyChanged("IsNetworkAvailable"); } }
        public Visibility OpenRmdVisibility { get => openRmdVisibility; set { openRmdVisibility = value; OnPropertyChanged("OpenRmdVisibility"); } }
        public string IsCheckedAppItem { get => isCheckedAppItem; set { isCheckedAppItem = value; OnPropertyChanged("IsCheckedAppItem"); } }
        public ObservableCollection<Notifications> NotifyList { get => notifyList; }
        public ObservableCollection<Applications> ApplicationList { get => applicationList; }
        public ObservableCollection<Applications> CopyAppList { get => copyAppList; }
        public bool? IsAllChecked { get => isAllChecked; set { isAllChecked = value; OnPropertyChanged("IsAllChecked"); } }
        public string IsAllCheckedText { get => isAllCheckedText; set { isAllCheckedText = value; OnPropertyChanged("IsAllCheckedText"); } }

        // Command bind
        public DelegateCommand<object> WindowCommand { get; set; }
        public DelegateCommand<SearchEventArgs> SearchCommand { get; set; }

        public event PropertyChangedEventHandler PropertyChanged;
        protected void OnPropertyChanged(string propertyName)
        {
            PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(propertyName));
        }
        #endregion

        public ServiceManagerViewModel(ServiceManagerWin parentWindow)
        {
            parentWin = parentWindow;

            GetUserName();
            app.HeartBeatEvent += () => {
                GetUserName();
            };

            // Regsiter network status event listener
            NetworkStatus.AvailabilityChanged += new NetworkStatus.NetworkStatusChangedHandler((ss,ee)=> {
                IsNetworkAvailable = ee.IsAvailable;
            });
            // init network status
            IsNetworkAvailable = NetworkStatus.IsAvailable;
            
            InitData();

            // init Command
            WindowCommand = new DelegateCommand<object>(WinCmdDispatcher);
            SearchCommand = new DelegateCommand<SearchEventArgs>(ApplicationListDoSearch);

            // init App_ListBox Event
            parentWin.app_ListBox.PreviewMouseWheel += ListBox_PreviewMouseWheel;

            // binding Notify event
            app.MyRecentNotifications.NotifyEvent += MyRecentNotifications_NotifyEvent;
        }

        private void GetUserName()
        {
            UserName = app.User.Name;
            AvatarText = CommonUtils.ConvertNameToAvatarText(UserName, " ");
            AvatarTextColor = CommonUtils.SelectionTextColor(UserName);
            AvatarBackground = CommonUtils.SelectionBackgroundColor(UserName);
        }

        #region Init Lists and notify Title, Rmd registry
        /// <summary>
        /// Init ApplicationList, NotifyList, NotifyTitle; read registry localMachine RMD Excutable
        /// </summary>
        private void InitData()
        {
            InitListsAndTitle();
            ReadRegistryLocal();
        }
        /// <summary>
        /// Init ApplicationList, NotifyList, NotifyTitle
        /// </summary>
        private void InitListsAndTitle()
        {
            InitAppList();
            InitNotifyListByAppList();
            InitNotifyTitle();
        }
        private void InitAppList()
        {
            ApplicationList.Clear();
            CopyAppList.Clear();

            var listApp = app.MyRecentNotifications.ListApp();

            var ascendList = listApp.OrderBy(a => a.Application).ToList();
            foreach (var item in ascendList)
            {
                ApplicationList.Add(new Applications()
                {
                    Id = item.Id,
                    Application = item.Application,
                    IsChecked = item.IsDisplay
                });

                CopyAppList.Add(new Applications()
                {
                    Id = item.Id,
                    Application = item.Application,
                    IsChecked = item.IsDisplay
                });
            }
            
            CheckedAppItem();
        }
        private void InitNotifyListByAppList()
        {
            NotifyList.Clear();

            var list = app.MyRecentNotifications.List();

            var descedList = list.OrderByDescending(a => a.Id).ToList();
            foreach (var item in descedList)
            {
                var appitem = ApplicationList.FirstOrDefault(a => a.Application.Equals(item.Application,StringComparison.OrdinalIgnoreCase));
                if (appitem != null && appitem.IsChecked)
                {
                    NotifyList.Add(new Notifications()
                    {
                        Id = item.Id,
                        Application = item.Application,
                        Target = item.Target,
                        Message = item.Message,
                        MessageType = item.MessageType,
                        Operation = item.Operation,
                        Result = item.Result,
                        FileStatus = item.FileStatus,
                        DateTime = item.DateTime
                    });
                }
            }
        }
        private void InitNotifyTitle()
        {
            if (IsAllChecked == null)
            {
                StringBuilder builder = new StringBuilder();
                for (int i = 0; i < ApplicationList.Count; i++)
                {
                    if (!ApplicationList[i].IsChecked)
                    {
                        continue;
                    }
                    builder.Append(ApplicationList[i].Application);
                    builder.Append(", ");
                }
                builder.Remove(builder.Length-2, 2);//remove last ", "
                IsCheckedAppItem = builder.ToString();
            }
            else if (IsAllChecked == true)
            {
                IsCheckedAppItem = CultureStringInfo.ServiceManageWin_All_Title;
            }
        }
        private void ReadRegistryLocal()
        {
            rmdExcutePath = "";
            try
            {
                RegistryKey localAppKey = Registry.LocalMachine.OpenSubKey(@"Software\Nextlabs\SkyDRM\LocalApp", false);
                if (localAppKey != null)
                {
                    rmdExcutePath = (string)localAppKey.GetValue("Executable", "");
                    localAppKey.Close();
                }
            }
            catch (Exception e)
            {
                app.Log.Error(e);
            }
            finally
            {
                if (string.IsNullOrEmpty(rmdExcutePath))
                {
                    OpenRmdVisibility = Visibility.Collapsed;
                }
                else
                {
                    // fix not clear regedit bug
                    if (File.Exists(rmdExcutePath))
                    {
                        OpenRmdVisibility = Visibility.Visible;
                    }
                    else
                    {
                        OpenRmdVisibility = Visibility.Collapsed;
                    }
                }
            }
        }
        #endregion

        #region Window Command Dispatcher
        private void WinCmdDispatcher(object args)
        {
            try
            {
                app.Log.Info("*****CMD******:" + args.ToString());
                switch (args.ToString())
                {
                    case "Cmd_OpenSkyDrmDesktop":
                        DoOpenSkydrmDesktop();
                        break;
                    case "Cmd_OpenSkyDrmWeb":
                        DoOpenSkydrmWeb();
                        break;
                    case "Cmd_OpenMenu":
                        DoOpenMenu();
                        break;
                    case "Cmd_MenuItemAbout":
                        MenuItemAbout();
                        break;
                    case "Cmd_MenuItemHelp":
                        MenuItemHelp();
                        break;
                    case "Cmd_MenuItemPreference":
                        MenuItemPreference();
                        break;
                    case "Cmd_MenuItemLogout":
                        MenuItemLogout();
                        break;
                    case "Cmd_OpenFilter":
                        DoOpenFilter();
                        break;
                    case "Cmd_ClearNotifyList":
                        ClearNotifyList();
                        break;
                    case "Cmd_CheckedAll":
                        CheckedAllAppItem();
                        break;
                    case "Cmd_CheckedItem":
                        CheckedAppItem();
                        break;
                    case "Cmd_FilterOk":
                        FilterOk();
                        break;
                    case "Cmd_FilterCancel":
                        FilterCancel();
                        break;
                    case "Cmd_DeleteNotifyItem":
                        DeleteNotifyItem();
                        break;
                    default:
                        break;
                }
            }
            catch (Exception e)
            {
                app.Log.Error("WinCmdDispatcher", e);
            }
           
        }
        private void DoOpenSkydrmDesktop()
        {
            if (!string.IsNullOrEmpty(rmdExcutePath))
            {
                ProcessStartInfo startInfo = new ProcessStartInfo();
                startInfo.FileName = rmdExcutePath;
                startInfo.Arguments = "-showmain";

                Process.Start(startInfo);
            }
        }
        private void DoOpenSkydrmWeb()
        {
            try
            {
                string url = app.DBProvider.GetUrl();
                if (string.IsNullOrEmpty(url))
                {
                    url = "http://www.skydrm.com/";
                }
                System.Diagnostics.Process.Start(url);
            }
            catch (Exception e)
            {
                app.Log.Error("Error happend in Get Url:", e);
            }
        }
        private void DoOpenMenu()
        {
            parentWin.contextMenu.PlacementTarget = parentWin.btnMenu;

            parentWin.contextMenu.Placement = PlacementMode.MousePoint;

            parentWin.contextMenu.IsOpen = true;
        }
        private void MenuItemAbout()
        {
            app.UIMediator.OnShowAboutWindow();
        }
        private void MenuItemHelp()
        {
            try
            {
                if (string.IsNullOrEmpty(rmdExcutePath))
                {
                    return;
                }
                string executable = rmdExcutePath;
                int index = executable.LastIndexOf("\\");
                string binPath = executable.Substring(0, index);

                int index2 = binPath.LastIndexOf("\\");
                string skydrmPath = binPath.Substring(0, index2);

                string helpPath = string.Format(@"{0}\help\index.html", skydrmPath);

                if (!File.Exists(helpPath))
                {
                    helpPath = "https://help.skydrm.com/docs/windows/help/1.0/en-us/home.htm#t=skydrmintro.htm";
                }
                System.Diagnostics.Process.Start(helpPath);
            }
            catch (Exception e)
            {
                app.Log.Error("Open help:",e);
            }
        }
        private void MenuItemPreference()
        {
            app.UIMediator.OnShowPreferenceWindow();
        }
        private void MenuItemLogout()
        {
            app.RequestLogout();
        }
        private void DoOpenFilter()
        {
            InitAppList();
            parentWin.searchBox.TbxInput.Text = "";// will trigger search cmd
            IsAllCheckedText = CultureStringInfo.ServiceManageWin_All_CheckBox;
        }
        private void ClearNotifyList()
        {
            app.MyRecentNotifications.DeleteAll();
            NotifyList.Clear();
        }
        private void CheckedAllAppItem()
        {
            if (IsAllChecked == true)
            {
                foreach (var item in ApplicationList)
                {
                    item.IsChecked = true;
                }
            }
            else
            {
                foreach (var item in ApplicationList)
                {
                    item.IsChecked = false;
                }
            }
        }
        private void CheckedAppItem()
        {
            if (ApplicationList.All(x => x.IsChecked))
                IsAllChecked = true;
            else if (ApplicationList.All(x => !x.IsChecked))
                IsAllChecked = false;
            else
                IsAllChecked = null;
        }
        private void FilterOk()
        {
            if (IsAllChecked == false)
            {
                return;
            }

            var listApp = app.MyRecentNotifications.ListApp();
            var ascendList = listApp.OrderBy(a => a.Application).ToList();

            // update db
            foreach (var item in ascendList)
            {
                var appItem = ApplicationList.FirstOrDefault(a => a.Id == item.Id);
                if (appItem != null)
                {
                    item.IsDisplay = appItem.IsChecked;
                }
                else
                {
                    item.IsDisplay = false;
                }
            }

            // update ApplicationList, notifyList and notify Title
            InitListsAndTitle();

            // close filter popup
            parentWin.filter_btn.IsChecked = false;
        }
        private void FilterCancel()
        {
            parentWin.filter_btn.IsChecked = false;
        }
        private void DeleteNotifyItem()
        {
            var item = parentWin.nt_LsBx.SelectedItem;
            if (item != null)
            {
                if (item is Notifications)
                {
                    app.MyRecentNotifications.DeleteItem(((Notifications)item).Id);
                    NotifyList.Remove((Notifications)item);
                }
            }
        }

        #endregion

        #region Search Command Dispatcher   
        private void ApplicationListDoSearch(SearchEventArgs args)
        {
            ApplicationList.Clear();

            string search = args.SearchText;
            if (string.IsNullOrEmpty(search))
            {
                foreach (var item in CopyAppList)
                {
                    ApplicationList.Add(item);
                }
                CheckedAppItem();
                IsAllCheckedText = CultureStringInfo.ServiceManageWin_All_CheckBox;
                return;
            }

            var searchList = CopyAppList.Where(a => a.Application.ToLower().Contains(search.ToLower())).ToList();
            foreach (var item in searchList)
            {
                ApplicationList.Add(new Applications()
                {
                    Id = item.Id,
                    Application = item.Application,
                    IsChecked = item.IsChecked
                });
            }
            CheckedAppItem();
            IsAllCheckedText = CultureStringInfo.ServiceManageWin_All_Search_CheckBox;
        }
        #endregion

        #region Window Event
        public void OnActivated()
        {
            // Mainly used to refresh data when popup service manager window.
            InitData();
            // binding notify event
            isTriggerNotifyEvent = true;
        }
        public void OnDeactivated()
        {
            // remove notify event
            isTriggerNotifyEvent = false;
        }
        private void ListBox_PreviewMouseWheel(object sender, MouseWheelEventArgs e)
        {
            var eventArg = new MouseWheelEventArgs(e.MouseDevice, e.Timestamp, e.Delta);
            //set routedEvent Type
            eventArg.RoutedEvent = UIElement.MouseWheelEvent;
            eventArg.Source = sender;
            (sender as ListBox).RaiseEvent(eventArg);
        }
        private void MyRecentNotifications_NotifyEvent(object sender, app.recentNotification.NotifyEventArgs e)
        {
            app.Log.Info("NotifyEvent is invoke");

            bool isUpdate = e.IsUpdate;
            rmservmgr.app.recentNotification.IRecentNotification recentNotification = e.Notification;

            int mesType = recentNotification.MessageType;
            if (mesType == 1&& app.User.ShowNotifyWindow)
            {
                // show notify window
                //this.parentWin.Dispatcher.BeginInvoke(new Action( delegate() { ShowNotifyWindow(recentNotification); }));
                ShowNotifyWindow(recentNotification);
            }

            if (!isTriggerNotifyEvent)
            {
                return;
            }
            
            if (isUpdate)
            {
                UpdateNotifyList(recentNotification);
            }
            else
            {
                InsertNotifyList(recentNotification);
            }
        }
        private void ShowNotifyWindow(app.recentNotification.IRecentNotification recentNotification)
        {
            try
            {
                NotifyWinViewModel notifyWinViewModel = new NotifyWinViewModel();
                notifyWinViewModel.Application = recentNotification.Application;
                notifyWinViewModel.Target = recentNotification.Target;
                notifyWinViewModel.Message = recentNotification.Message;
                notifyWinViewModel.Result = recentNotification.Result;
                notifyWinViewModel.FileStatus = recentNotification.FileStatus;

                NotifyWin window = new NotifyWin()
                {
                    ViewModel = notifyWinViewModel
                };

                window.Closed += (ss, ee) =>
                {
                    var win = ss as NotifyWin;
                    notifyWins.Remove(win);
                };
                window.TopFrom = GetTopFrom();
                notifyWins.Add(window);
                window.Show();
                window.Activate();
                // fix bug 57690
                window.Topmost = true;
                window.Topmost = false;
            }
            catch (Exception e)
            {
                app.Log.Error("Error in show Notify window:", e);
            }
           
        }
        /// <summary>
        /// Show NotifyWindow,will not record in DB file
        /// </summary>
        /// <param name="messagePara"></param>
        public void ShowNotifyWindow(app.recentNotification.MessagePara messagePara)
        {
            try
            {
                NotifyWinViewModel notifyWinViewModel = new NotifyWinViewModel();
                notifyWinViewModel.Application = messagePara.Application;
                notifyWinViewModel.Target = messagePara.Target;
                notifyWinViewModel.Message = messagePara.Message;
                notifyWinViewModel.Result = messagePara.Result;
                notifyWinViewModel.FileStatus = messagePara.FileStatus;

                NotifyWin window = new NotifyWin()
                {
                    ViewModel = notifyWinViewModel
                };

                window.Closed += (ss, ee) =>
                {
                    var win = ss as NotifyWin;
                    notifyWins.Remove(win);
                };
                window.TopFrom = GetTopFrom();
                notifyWins.Add(window);
                window.Show();
                window.Activate();
                // fix bug 57690
                window.Topmost = true;
                window.Topmost = false;
            }
            catch (Exception e)
            {
                app.Log.Error("Error in show Notify window:", e);
            }
        }

        private double GetTopFrom()
        {
            // 10 is Windows taskBar height
            double topFrom = System.Windows.SystemParameters.WorkArea.Bottom;
            bool isContinueFind = notifyWins.Any(o => o.TopFrom == topFrom);

            while (isContinueFind)
            {
                topFrom = topFrom - 100;//100 is NotifyWin height, 10 is interval
                isContinueFind = notifyWins.Any(o => o.TopFrom == topFrom);
            }

            if (topFrom <= 100)
                topFrom = System.Windows.SystemParameters.WorkArea.Bottom;

            return topFrom;
        }

        private void UpdateNotifyList(app.recentNotification.IRecentNotification recentNotification)
        {
            foreach (var item in NotifyList)
            {
                if (item.Id == recentNotification.Id)
                {
                    item.Message = recentNotification.Message;
                    item.MessageType = recentNotification.MessageType;
                    item.Operation = recentNotification.Operation;
                    item.Result = recentNotification.Result;
                    item.FileStatus = recentNotification.FileStatus;
                    item.DateTime = recentNotification.DateTime;
                }
            }
        }
        private void InsertNotifyList(app.recentNotification.IRecentNotification recentNotification)
        {
            // when filter popup not open, re-init AppList.
            if (parentWin.filter_btn.IsChecked != true)
            {
                InitAppList();
                InitNotifyTitle();
            }
            
            var appitem = ApplicationList.FirstOrDefault(a => a.Application.Equals(recentNotification.Application,StringComparison.OrdinalIgnoreCase));
            if (appitem != null && appitem.IsChecked)
            {
                NotifyList.Insert(0, new Notifications()
                {
                    Id = recentNotification.Id,
                    Application = recentNotification.Application,
                    Target = recentNotification.Target,
                    Message = recentNotification.Message,
                    MessageType = recentNotification.MessageType,
                    Operation = recentNotification.Operation,
                    Result = recentNotification.Result,
                    FileStatus = recentNotification.FileStatus,
                    DateTime = recentNotification.DateTime
                });
            }
        }
        #endregion


    }
}
