using ServiceManager.resources.languages;
using ServiceManager.rmservmgr.common.helper;
using ServiceManager.rmservmgr.ui.windows;
using ServiceManager.rmservmgr.ui.windows.chooseServer;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;

namespace ServiceManager.rmservmgr.app
{
    public class UIMediator
    {
        private ServiceManagerApp app;

        public UIMediator(ServiceManagerApp app)
        {
            this.app = app;
        }

        public void OnShowMainWin(Window window)
        {
        }

        public void OnShowChooseServerWin(Window sender = null)
        {
            try
            {
                //for nxrmshell Judgment display window
                bool isFirstLogin = true;
                foreach (Window one in app.Windows)
                {
                    if (one.GetType() == typeof(ChooseServerWindow))
                    {
                        isFirstLogin = false;
                        one.Show();
                        one.Activate();
                        one.Focus();
                        one.WindowState = WindowState.Normal;
                    }
                    if (one.GetType() == typeof(LoginWindow))
                    {
                        isFirstLogin = false;
                        one.Show();
                        one.Activate();
                        one.Focus();
                        one.WindowState = WindowState.Normal;
                    }
                }

                if (isFirstLogin)
                {
                    ChooseServerWindow chooseWindow = new ChooseServerWindow();
                    chooseWindow.Topmost = true;
                    chooseWindow.Show();
                    chooseWindow.Activate();
                    chooseWindow.Focus();
                    chooseWindow.WindowState = WindowState.Normal;
                }
            }
            catch (Exception e)
            {
                app.Log.Fatal("Error:OnShowLogin", e);
            }

            if (sender != null)
            {
                sender.Close();
            }
        }

        // for chooseServer Window use
        public void OnShowLoginWin(Window sender = null, bool IsPersonal = true, string routerUrl = "", bool IsRemeber = true)
        {
            try
            {
                ui.windows1.LoginWindowEx loginWindow = new ui.windows1.LoginWindowEx(ui.windows1.Intention.REGISTER, IsPersonal, routerUrl, IsRemeber);
                loginWindow.Show();

            }
            catch (Exception e)
            {
                app.Log.Fatal("Error:OnShowLogin", e);
            }

            if (sender != null)
            {
                sender.Close();
            }

        }

        private void OnShowInitializeWin(bool isShow)
        {
            try
            {
                if (isShow)
                {
                    Window win = new InitializeWindow();
                    win.Show();
                    win.Activate();
                    win.Focus();
                }
                else
                {
                    foreach (Window win in app.Windows)
                    {
                        if (win.GetType() == typeof(InitializeWindow))
                        {
                            win.Close();
                            return;
                        }
                    }
                }

            }
            catch (Exception e)
            {
                app.Log.Warn(e.Message, e);
            }
        }

        public void OnLogin(Window loginWindow, string loginString, string webUrl, bool isPersonal)
        {
            try
            {
                // Load InitializeWindow
                OnShowInitializeWin(true);

                app.TrayIconManager.OnGoing_Login = true;
                app.TrayIconManager.RefreshMenuItem();
                app.TrayIconManager.PopupTargetWin = null;

                // bg task
                Func<LoginPara, int> asyncTask = new Func<LoginPara, int>((LoginPara para) => {
                    return app.Login(para);
                });

                // calllback
                Action<int> callback = new Action<int>((int result) => {

                    if (result == 1)
                    {
                        // This will takes a few seconds sometimes, now put it here, and will optimize later.
                        app.InitAfterLogin();

                        loginWindow.Close();
                    }
                    else
                    {
                        loginWindow.Close();

                        app.TrayIconManager.OnGoing_Login = false;
                        app.TrayIconManager.RefreshMenuItem();

                        if (result == 101)
                        {
                            app.ShowBalloonTip("Another user is already login the different RMS server.");
                        }
                        else
                        {
                            app.ShowBalloonTip("Service Manager Login failed");
                        }

                        // Goto chooseServerWindow
                        OnShowChooseServerWin();
                    }
                    OnShowInitializeWin(false);
                });

                // Invoke
                AsyncHelper.RunAsync(asyncTask, new LoginPara(loginString, webUrl, isPersonal), callback);

            }
            catch (Exception e)
            {
                app.Log.Fatal("Error: OnLogin", e);
            }
        }

        public class LoginPara
        {
            public string LoginStr { get; private set; }
            public string WebUrl { get; private set; }
            public bool IsPersonal { get; private set; }
            public LoginPara(string loginStr, string webUrl, bool isPersonal)
            {
                this.LoginStr = loginStr;
                this.WebUrl = webUrl;
                this.IsPersonal = isPersonal;
            }
        }

        public void OnShowAboutWindow()
        {
            try
            {
                //fix bug 49880
                foreach (Window win in app.Windows)
                {
                    if (win.GetType() == typeof(AboutWindow))
                    {
                        win.Show();
                        win.Activate();
                        win.Focus();
                        win.WindowState = WindowState.Normal;

                        return;
                    }
                }
                AboutWindow aboutWindow = new AboutWindow();
                
                aboutWindow.Show();
            }
            catch (Exception e)
            {
                app.Log.Fatal("Error:OnShowAboutWindow", e);
            }
        }

        public void OnShowPreferenceWindow()
        {
            try
            {
                //fix bug 49880
                foreach (Window win in app.Windows)
                {
                    if (win.GetType() == typeof(PreferenceWindow))
                    {
                        win.Show();
                        win.Activate();
                        win.Focus();
                        win.WindowState = WindowState.Normal;

                        return;
                    }
                }
                PreferenceWindow window = new PreferenceWindow();

                window.Show();
            }
            catch (Exception e)
            {
                app.Log.Fatal("Error:OnShowAboutWindow", e);
            }
        }

    }
}
