using ServiceManager.resources.languages;
using ServiceManager.rmservmgr.common.components;
using ServiceManager.rmservmgr.common.helper;
using ServiceManager.rmservmgr.ui.windows.chooseServer.model;
using System;
using System.Collections.Generic;
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
using static ServiceManager.rmservmgr.common.components.NetworkStatus;

namespace ServiceManager.rmservmgr.ui.windows.chooseServer
{
    /// <summary>
    /// Interaction logic for ChooseServerWindow.xaml
    /// </summary>
    public partial class ChooseServerWindow : Window
    {
        private ServiceManagerApp app = ServiceManagerApp.Singleton;

        private ChooseServerModel chooseServerModel;
        private bool isPersonal = true;
        private bool isNetworkAvailable;
        private string Message = null;

        public ChooseServerWindow()
        {
            InitializeComponent();

            chooseServerModel = new ChooseServerModel();
            this.DataContext = chooseServerModel;

            // Set defult radio checked
            this.RadioCompany.IsChecked = true;

            this.Loaded += delegate
            {
                // register trayIcon click popup window.
                app.TrayIconManager.PopupTargetWin = this;
                app.TrayIconManager.IsChooseServerWindowLoaded = true;
                this.Topmost = false;
            };

            this.Closed += delegate 
            {
                if (app.TrayIconManager.PopupTargetWin is ChooseServerWindow)
                {
                    app.TrayIconManager.PopupTargetWin = null;
                }
                app.TrayIconManager.IsChooseServerWindowLoaded = false;
            };

            // Regsiter network status event listener
            AvailabilityChanged += new NetworkStatusChangedHandler(OnNetworkStatusChanged);

            // init network status
            isNetworkAvailable = NetworkStatus.IsAvailable;
        }


        private void OnNetworkStatusChanged(object sender, NetworkStatusChangedArgs e)
        {
            isNetworkAvailable = e.IsAvailable;
        }

        private void Next_Click(object sender, RoutedEventArgs e)
        {
            // Judge whether isPersonal from RadioButton isChecked 
            if (this.RadioCompany.IsChecked == true)
            {
                isPersonal = false;
                chooseServerModel.URL = this.mycombox.Text.Trim();
            }
            else
            {
                isPersonal = true;
                chooseServerModel.URL = app.Config.PersonRouter.Trim();  
            }

            // First check: judge Url whether is null or empty
            if (string.IsNullOrEmpty(chooseServerModel.URL))
            {
                app.ShowBalloonTip(CultureStringInfo.CheckUrl_Notify_UrlEmpty);
                return;
            }

            // Add http head if url not contain
            if (!chooseServerModel.URL.StartsWith("http", StringComparison.CurrentCultureIgnoreCase))
            {
                chooseServerModel.URL = "https://" + chooseServerModel.URL;
            }

            // Second check:
            if (!CheckUrl(chooseServerModel.URL))
            {
                app.ShowBalloonTip(CultureStringInfo.CheckUrl_Notify_UrlError);
                return;
            }

            // Third check: judge Network
            if (isNetworkAvailable)
            {
                // If network connected, start async task and check URL from http request.
                this.GridProBar.Visibility = Visibility.Visible;

                AsyncCheckUrl(chooseServerModel.URL);
            }
            else
            {
                app.ShowBalloonTip(CultureStringInfo.CheckUrl_Notify_NetDisconnect);
            }

        }

        private void AsyncCheckUrl(string chosedUrl)
        {
            // bg task
            Func<string, bool> asyncTask = new Func<string, bool>((string url) => {
                return InvokeSdkSessionInitialize(chooseServerModel.URL);
            });

            // calllback
            Action<bool> callback = new Action<bool>((bool result) => {

                // Modify ui status
                this.GridProBar.Visibility = Visibility.Collapsed;

                if (result)
                {
                    bool IsRember = false;

                    if (this.RadioCompany.IsChecked == true)
                    {
                        if (this.CheckRememberURL.IsChecked == true)
                        {
                            //do insert url in .db file
                            // ChooseServerModel.InsertUrl();
                            IsRember = true;
                        }
                    }

                    app.UIMediator.OnShowLoginWin(this, isPersonal, chooseServerModel.URL, IsRember); 
                }
                else
                {
                    string msg = CultureStringInfo.CheckUrl_Notify_NetworkOrUrlError;
                    app.ShowBalloonTip(msg);
                    app.Log.Error(msg + Message);
                }

            });

            // Execute
            AsyncHelper.RunAsync(asyncTask, chosedUrl, callback);
        }

        // Invoke SDK Session.Initialize
        private bool InvokeSdkSessionInitialize(string strUrl)
        {
            try
            {
                app.Session.Initialize(app.Config.RmSdkFolder, strUrl, "");
                return true;
            }
            catch (Exception we)
            {
                Message = we.Message;
                return false;
            }
        }

        private bool CheckUrl(string url)
        {
            bool checkUrl = false;

            Uri uriResult;
            bool result = Uri.TryCreate(url, UriKind.Absolute, out uriResult)
                && (uriResult.Scheme == Uri.UriSchemeHttp || uriResult.Scheme == Uri.UriSchemeHttps);

            if (result)
            {
                checkUrl = true;
            }
            return checkUrl;
        }

        private void RadioBtn(object sender, RoutedEventArgs e)
        {
            RadioButton radioButton = sender as RadioButton;
            if (radioButton != null && radioButton.Content != null)
            {
                switch (radioButton.Name.ToString())
                {
                    case "RadioPerson":
                        chooseServerModel.ServerModel = ServerModel.Personal;
                        break;
                    case "RadioCompany":
                        chooseServerModel.ServerModel = ServerModel.Company;
                        break;
                }

            }
        }


        private void Combox_TextChanged(object sender, TextChangedEventArgs e)
        {
            Console.WriteLine("--text changed!!!----" + this.mycombox.Text);
            string sourceText = (e.OriginalSource as TextBox).Text;
            //fix bug 49936
            var targetComboBox = sender as ComboBox;
            var targetTextBox = targetComboBox?.Template.FindName("PART_EditableTextBox", targetComboBox) as TextBox;

            bool isDropDown;
            if (this.RadioCompany.IsChecked == true)
            {
                this.chooseServerModel.Serach(sourceText, out isDropDown);

                //Records the position of the currently selected cursor
                int careIndex = targetTextBox.CaretIndex;
                //the text value is selected
                this.mycombox.IsDropDownOpen = isDropDown;
                //Set cursor position,and the text value is not selected 
                targetTextBox.CaretIndex = careIndex;

            }
            else
            {
                this.mycombox.IsDropDownOpen = false;
                //when combox text is "",it should  again add item from copylist to UrlList 
                this.chooseServerModel.Serach(sourceText, out isDropDown);
            }
        }

    }
}
