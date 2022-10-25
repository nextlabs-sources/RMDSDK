using CefSharp;
using CefSharp.Wpf;
using Newtonsoft.Json.Linq;
using ServiceManager.rmservmgr.common.helper;
using System;
using System.Collections.Generic;
using System.Diagnostics;
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

namespace ServiceManager.rmservmgr.ui.windows1
{
    /// <summary>
    /// Interaction logic for LoginWindowEx.xaml
    ///  
    ///   Upgrade the embeded browser, and now using Chromium to replace the default IE browser, to resolve using google
    ///   account cannot login issue: fix bug 69720
    /// </summary>
    public partial class LoginWindowEx : Window
    {
        ServiceManagerApp app = ServiceManagerApp.Singleton;
        private string weburl;

        public Intention Intention { get; set; }
        private string Router { get; set; }
        private bool IsPersonal { get; set; }
        private bool IsRemeber { get; set; }

        private ChromiumWebBrowser LoginWeb;

        public LoginWindowEx(Intention intention, bool IsPersonal = true, string routerUrl = "", bool IsRemeber = true)
        {
            this.Intention = intention;
            this.Router = routerUrl;
            this.IsPersonal = IsPersonal;
            this.IsRemeber = IsRemeber;

            InitializeComponent();

            InitBrowser();

            LoadLoginPage();

            this.Loaded += delegate
            {
                // Register trayIcon click popup window.
                app.TrayIconManager.PopupTargetWin = this;
                app.TrayIconManager.IsLoginWindowLoaded = true;
            };

            this.Closed += delegate
            {
                if (app.TrayIconManager.PopupTargetWin is LoginWindowEx)
                {
                    app.TrayIconManager.PopupTargetWin = null;
                }
                app.TrayIconManager.IsLoginWindowLoaded = false;
            };
        }


        private void InitBrowser()
        {
            try
            {
                var setting = new CefSettings();

                // Must config the following setting for using google account login.
                setting.IgnoreCertificateErrors = true;
                setting.UserAgent = "Chrome/72.0.3626.121 Safari/537.36";
                setting.LogSeverity = LogSeverity.Disable;

                if (!CefSharp.Cef.IsInitialized)
                {
                    var ret = CefSharp.Cef.Initialize(setting, true);
                }
            }
            catch (Exception e)
            {
                Trace.WriteLine("Failed to init chromium browser.");
                throw e;
            }
        }

        private void LoadLoginPage()
        {

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

                var cookieMgr = Cef.GetGlobalCookieManager();
                // Delete the cookies first, to fix bug 70102
                cookieMgr.DeleteCookies();
                foreach (var item in cookies)
                {
                   bool rt = cookieMgr.SetCookie(weburl, new Cookie() { Name = item.Key, Value = item.Value });
                    if (!rt)
                    {
                        Trace.WriteLine("Failed to set cookie.");
                    }

                    //
                    // Note: cannot using the below system api to set cookie for Chromium
                    //
                    // Win32Common.InternetSetCookie(weburl, item.Key, item.Value);
                }

                LoginWeb = new ChromiumWebBrowser();
                LoginWeb.RenderProcessMessageHandler = new MyRenderHandler()
                {
                    LoginWeb = this.LoginWeb,
                    LoginWindowEx = this,
                    IsPersonal = this.IsPersonal
                };
                this.topGrid.Children.Add(LoginWeb);
                LoginWeb.Address = weburl;
            }
            catch (Exception e)
            {
                app.Log.Warn("error in LoadLoginPage," + e.Message, e);
                this.Close();
            }
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
                this.LoginWeb.Load("about:blank");
            }
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

        LoginWindowEx mExternalWPF;
        public ObjectForScriptingHelper(LoginWindowEx w, string webUrl, bool isPersonal)
        {
            this.mExternalWPF = w;
            this.webUrl = webUrl;
            this.isPersonal = isPersonal;
        }
        public void hook(string jsscript) // Function name must be lowercase.
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

                // Need to switch to main thread.
                app.Dispatcher.Invoke(() =>
                {
                    mExternalWPF.ToDefaultWebPage();
                    mExternalWPF.Hide();
                    isLogined = true;

                    app.UIMediator.OnLogin(mExternalWPF, jsscript, webUrl, isPersonal);
                });

            }
            catch (Exception e)
            {
                app.Log.Warn("error occrued in injected js that will take user login," + e.Message, e);
                return;
            }


        }

    }


    class MyRenderHandler : IRenderProcessMessageHandler
    {
        public ChromiumWebBrowser LoginWeb { get; set; }
        public bool IsPersonal { get; set; }
        public LoginWindowEx LoginWindowEx { get; set; }

        private ObjectForScriptingHelper jsInterface = null;

        public void OnContextCreated(IWebBrowser chromiumWebBrowser, IBrowser browser, IFrame frame)
        {
            string url = frame.Url;
            Console.WriteLine("OnContextCreated");

            //string url = ee.Browser.FocusedFrame.Url;
            if (!url.ToLower().Contains("login") && !url.ToLower().Contains("main"))
            {
                // hide load progress
                return;
            }

            if (url.ToLower().Contains("servicelogin"))
            {
                return;
            }

            if (jsInterface == null)
            {
                // Regsiter the listener callback that js call C#
                jsInterface = new ObjectForScriptingHelper(LoginWindowEx, url, IsPersonal);

                // Not need to set this
                // LoginWeb.JavascriptObjectRepository.Settings.LegacyBindingEnabled = true;

                LoginWeb.JavascriptObjectRepository.NameConverter = new CefSharp.JavascriptBinding.CamelCaseJavascriptNameConverter();

                LoginWeb.JavascriptObjectRepository.Register("jsInterface",
                    jsInterface,
                    true,
                    CefSharp.BindingOptions.DefaultBinder);

            }

            try
            {
                string InjectedCode =
                    @"
                    (function (open) { 
                            XMLHttpRequest.prototype.open = function () { 
                                this.addEventListener('readystatechange', function () { 
                                      CefSharp.BindObjectAsync('jsInterface');
                                      jsInterface.hook(this.responseText);  
                                }, false); 
                                open.apply(this, arguments); 
                            }; 
                        } 
                    ) 
                    (XMLHttpRequest.prototype.open);
                 ";

                frame.ExecuteJavaScriptAsync(InjectedCode);
            }
            catch (Exception e)
            {
                Trace.WriteLine(e.ToString());
            }

        }

        public void OnContextReleased(IWebBrowser chromiumWebBrowser, IBrowser browser, IFrame frame) { }

        public void OnFocusedNodeChanged(IWebBrowser chromiumWebBrowser, IBrowser browser, IFrame frame, IDomNode node) { }

        public void OnUncaughtException(IWebBrowser chromiumWebBrowser, IBrowser browser, IFrame frame, JavascriptException exception) { }

    }
}
