using Newtonsoft.Json.Linq;
using ServiceManager.rmservmgr.common.helper;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Reflection;
using System.Runtime.InteropServices;
using System.Security.Permissions;
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
    /// Interaction logic for LoginWindow.xaml
    /// </summary>
    public partial class LoginWindow : Window
    {
        ServiceManagerApp app = ServiceManagerApp.Singleton;
        private string weburl;

        public Intention Intention { get; set; }
        private string Router { get; set; }
        private bool IsPersonal { get; set; }
        private bool IsRemeber { get; set; }

        public LoginWindow(Intention intention, bool IsPersonal = true, string routerUrl = "", bool IsRemeber = true)
        {
            this.Intention = intention;
            this.Router = routerUrl;
            this.IsPersonal = IsPersonal;
            this.IsRemeber = IsRemeber;

            InitializeComponent();

            // Load progress
            DisplayLoadProgress();

            LoadLoginPage();

            this.Loaded += delegate
            {
                // Register trayIcon click popup window.
                app.TrayIconManager.PopupTargetWin = this;
            };

            this.Closed += delegate
            {
                if (app.TrayIconManager.PopupTargetWin is LoginWindow)
                {
                    app.TrayIconManager.PopupTargetWin = null;
                }
            };

            LoginWeb.Navigating += delegate
            {
                SuppressScriptErrors(true);
            };

        }

        private void LoadLoginPage()
        {
            //
            // clear all the cookies at this page
            //
            /*
             +// #define CLEAR_COOKIES         0x0002 // Clears cookies
             // #define CLEAR_SHOW_NO_GUI     0x0100 // Do not show a GUI when running the cache clearing
            */
            RunCmd("RunDll32.exe", "InetCpl.cpl,ClearMyTracksByProcess 258");

            try
            {
                // Will do SaaS login session initialization
                if (Intention == Intention.SIGN_UP)
                {
                    string strUrl = "https://r.skydrm.com";
                    app.Session.Initialize(app.Config.RmSdkFolder, strUrl, ""); 
                }

                Dictionary<string, string> cookies = null;
                app.Session.GetLogingParams(out weburl, out cookies); 

                // Will directly jump into SaaS account register page when user click "SignUp" button from splash.
                if (Intention == Intention.SIGN_UP)
                {
                    weburl = "https://skydrm.com/rms/register";
                }

                foreach (var item in cookies)
                {
                    Win32Common.InternetSetCookie(weburl, item.Key, item.Value);
                }

                this.LoginWeb.Source = new Uri(weburl);
            }
            catch (Exception e)
            {
                app.Log.Warn("error in LoadLoginPage," + e.Message, e);
                this.Close();
            }

            // We got a requried to hide the main page of web rms
            LoginWeb.Navigating += (ss, ee) =>
            {

                if (ee.Uri.AbsolutePath.ToLower().Contains("/rms/main"))
                {
                    try
                    {
                        DisplayLoadProgress();
                    }
                    catch (Exception e)
                    {
                        var a = e.Message;
                    }
                }

            };

            // we have to inject js code to set the listener for user login message
            LoginWeb.LoadCompleted += (ss, ee) =>
            {
                HideLoadProgress();

                if (!ee.Uri.AbsolutePath.ToLower().Contains("login") && !ee.Uri.AbsolutePath.ToLower().Contains("main"))
                {
                    // hide load progress
                    return;
                }
         
                var doc = LoginWeb.Document;
                // prepare a method let javascript to call back
                LoginWeb.ObjectForScripting = new ObjectForScriptingHelper(this, weburl, IsPersonal);

                try
                {
                    // write ajac intercept code     
                    string InjectedCode =
                        @"
                        {
                            $(document).ajaxSuccess(function( event, xhr, settings ) {
                                window.external.InvokeMeFromJavascript(xhr.responseText);
                            });
                        }
                    ";

                    LoginWeb.InvokeScript("eval", new object[] { InjectedCode });

                    // special for domain account login
                    InjectedCode =
                    @"
                    (function (open) { 
                            XMLHttpRequest.prototype.open = function () { 
                                this.addEventListener('readystatechange', function () { 
                                    window.external.InvokeMeFromJavascript(this.responseText); 
                                }, false); 
                                open.apply(this, arguments); 
                            }; 
                        } 
                    ) 
                    (XMLHttpRequest.prototype.open);
                ";
                    LoginWeb.InvokeScript("eval", new object[] { InjectedCode });
                }
                catch (Exception e)
                {
                    app.Log.Error(e);
                }


            };

        }

        private void RunCmd(string cmd, string param)
        {
            System.Diagnostics.Process p = new System.Diagnostics.Process();
            p.StartInfo.FileName = cmd;
            p.StartInfo.Arguments = param;
            p.StartInfo.UseShellExecute = false;
            p.StartInfo.RedirectStandardInput = true;
            p.StartInfo.RedirectStandardOutput = true;
            p.StartInfo.RedirectStandardError = true;
            p.StartInfo.CreateNoWindow = true;
            p.StartInfo.WindowStyle = System.Diagnostics.ProcessWindowStyle.Hidden;
            p.Start();
            p.WaitForExit();
        }

        private void Window_Closed(object sender, EventArgs e)
        {
            ToDefaultWebPage();
        }

        public void ToDefaultWebPage()
        {
            if (this.LoginWeb != null)
            {
                // we intercept XMLHTTPRequest
                this.LoginWeb.Navigate("about:blank");
            }
        }

        private void DisplayLoadProgress()
        {
            this.LoadingBar.Visibility = Visibility.Visible;
            this.LoginWeb.Visibility = Visibility.Collapsed;
        }
        private void HideLoadProgress()
        {
            this.LoadingBar.Visibility = Visibility.Collapsed;
            this.LoginWeb.Visibility = Visibility.Visible;
        }

        /// <summary>
        /// Forbid popup script errors dialog when login
        /// </summary>
        private void SuppressScriptErrors(bool isHide)
        {
            FieldInfo fiComWebBrowser = typeof(WebBrowser).GetField("_axIWebBrowser2", BindingFlags.Instance | BindingFlags.NonPublic);
            if (fiComWebBrowser == null) return;

            object objComWebBrowser = fiComWebBrowser.GetValue(LoginWeb);
            if (objComWebBrowser == null) return;

            objComWebBrowser.GetType().InvokeMember("Silent", BindingFlags.SetProperty, null, objComWebBrowser, new object[] { isHide });
        }

    }

    public enum Intention
    {
        REGISTER,
        SIGN_UP,
    }

    [PermissionSet(SecurityAction.Demand, Name = "FullTrust")]
    [ComVisible(true)]
    public class ObjectForScriptingHelper
    {
        bool isLogined = false;
        bool isPersonal;
        string webUrl;

        LoginWindow mExternalWPF;
        public ObjectForScriptingHelper(LoginWindow w, string webUrl, bool isPersonal)
        {
            this.mExternalWPF = w;
            this.webUrl = webUrl;
            this.isPersonal = isPersonal;
        }
        public void InvokeMeFromJavascript(string jsscript)
        {
            ServiceManagerApp app = ServiceManagerApp.Singleton;
            /*
             		jsscript	"{\"statusCode\":200,\"message\":\"Authorized\",\"serverTime\":1526563486530,\"extra\":{\"userId\":25,\"ticket\":\"1A609738A8D9BF01B5CD5609209FB7FF\",\"tenantId\":\"21b06c79-baab-419d-8197-bad3ce3f4476\",\"lt\":\"skydrm.com\",\"ltId\":\"21b06c79-baab-419d-8197-bad3ce3f4476\",\"ttl\":1526649886528,\"name\":\"osmond.ye\",\"email\":\"osmond.ye@nextlabs.com\",\"preferences\":{\"homeTour\":true,\"profile_picture\":\"data:image/jpeg;base64,/9j/4AAQSkZJRgABAQAAAQABAAD/2wBDAFA3PEY8MlBGQUZaVVBfeMiCeG5uePWvuZHI////////\KKKKACiiigAooooAKKKKACiiigAoo\\nooAKKKKACpYPvGiigCaiiigAooooAKKKKACiiigAooooAKKKKACiiigAooooAZN/q6r0UUAFFFFA\\nBRRRQAUUUUAFFFFABRRRQAUUUUAFFFFABTo/viiigCzRRRQAUUUUAf/Z\\n\"},\"idpType\":0,\"memberships\":[{\"id\":\"m39@skydrm.com\",\"type\":0,\"tenantId\":\"21b06c79-baab-419d-8197-bad3ce3f4476\",\"projectId\":1},{\"id\":\"m332@t-f3d5a9dad6c54454a896a98fcae10c5f\",\"type\":0,\"tenantId\":\"4ead5485-80b0-4e6d-bea0-a9b16119b674\",\"projectId\":163},{\"id\":\"m929@t-36d9566d8eed4495a3608f4ff80064a8\",\"type\":0,\"tenantId\":\"43360a40-a4b2-4061-97e1-c8912f115828\",\"projectId\":503},{\"id\":\"m1138@t-0fb33c1c7f3746f293e0228f94fbed23\",\"type\":0,\"tenantId\":\"2096293b-4f78-460b-9350-7628fe06cbcc\",\"projectId\":608},{\"id\":\"m915@t-cd7ec64c03a349ce940730248b828bba\",\"type\":0,\"tenantId\":\"c689dbf5-b7a1-4345-a630-2765b60de278\",\"projectId\":498},{\"id\":\"m691@t-066f8cd594164c569bf8322b58681ad0\",\"type\":0,\"tenantId\":\"0dde2d00-a3b7-453a-b705-427876045e36\",\"projectId\":367},{\"id\":\"m331@t-dc62646efbee49ceb4c184864d28816a\",\"type\":0,\"tenantId\":\"7929e62c-e4ac-4a81-8043-9c2c345bea8d\",\"projectId\":162},{\"id\":\"m333@t-b719fa508bae411d9f4470221819bd9a\",\"type\":0,\"tenantId\":\"2f69c9e1-d310-4ea1-9b1d-15420453b773\",\"projectId\":17}],\"defaultTenant\":\"skydrm.com\",\"defaultTenantUrl\":\"https://rmtest.nextlabs.solutions/rms\",\"attributes\":{\"displayName\":[\"osmond.ye\"],\"email\":[\"osmond.ye@nextlabs.com\"]}}}"	string

            */
            // try to parse the result
            if (isLogined)
            {
                return;
            }

            if (!StringHelper.IsValidJsonStr_Fast(jsscript))
            {
                return;
            }

            try
            {
                JObject jo = JObject.Parse(jsscript);
                int statuscode = (int)jo["statusCode"];
                string message = (string)jo["message"];
                if (statuscode != 200)
                {
                    return;
                }
                if (!"Authorized".ToLower().Equals(message.ToLower()))
                {
                    return;
                }

                // we consider user logined 
                mExternalWPF.ToDefaultWebPage();
                mExternalWPF.Hide();
                isLogined = true;

                app.UIMediator.OnLogin(mExternalWPF, jsscript, webUrl, isPersonal); 
            }
            catch (Exception e)
            {
                app.Log.Warn("error occrued in injected js that will take user login," + e.Message, e);
                return;
            }


        }

    }

}
