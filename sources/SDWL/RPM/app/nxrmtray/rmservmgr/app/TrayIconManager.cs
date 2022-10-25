using ServiceManager.resources.languages;
using System;
using System.Collections.Generic;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Input;

namespace ServiceManager.rmservmgr.app
{
    public class TrayIconManager
    {
        public System.Windows.Forms.NotifyIcon ni;

        private ServiceManagerApp app;

        public System.Windows.Forms.ContextMenu ContextMenu { get; set; }

        public bool IsLogin { get; set; }

        public bool IsChooseServerWindowLoaded { get; set; }

        public bool IsLoginWindowLoaded { get; set; }

        public bool OnGoing_Login{ get; set; }

        public Window PopupTargetWin { get; set; }

        public TrayIconManager(ServiceManagerApp app)
        {
            ni = new System.Windows.Forms.NotifyIcon();
            this.app = app;
            ContextMenu = new System.Windows.Forms.ContextMenu();

            IsLogin = false;

            InitIcon();

            // init bubble.
            InitText();
            // fix bug 58026
            app.HeartBeatEvent += () => { InitText(); };

            InitMenuItem();

            InitIconClick();
            // display
            ni.Visible = true;
        }

        private void InitIcon()
        {
            // init icon
            var stream = new Uri(@"resources/icons/TrayIcon2.png", UriKind.Relative);
            if (IsWindows7)
            {
                stream = new Uri(@"resources/icons/TrayIcon.png", UriKind.Relative);
            }
            else
            {
                stream = new Uri(@"resources/icons/TrayIcon2.png", UriKind.Relative);
            }
            var bitmap = new Bitmap(Application.GetResourceStream(stream).Stream);
            ni.Icon = System.Drawing.Icon.FromHandle(bitmap.GetHicon());
        }
        private bool IsWindows7
        {
            get
            {
                return (Environment.OSVersion.Platform == PlatformID.Win32NT)
                    && (Environment.OSVersion.Version.Major == 6)
                    && (Environment.OSVersion.Version.Minor == 1);
            }
        }

        private void InitText()
        {
            if (IsLogin)
            {
                string text= CultureStringInfo.Tray_Text + "\n" + app.User.Name;
               
                ni.Text = HandleNotifyIconText(text); 
            }
            else
            {
                string text= CultureStringInfo.Tray_Text + "\n" + CultureStringInfo.Tray_Text_Not_Sign_in;
               
                ni.Text = HandleNotifyIconText(text);
            }
        }

        //NotifyIcon text must less than 63 characters long. fix bug 57658
        private string HandleNotifyIconText(string text)
        {
            string value = text;
            if (value.Length > 63)
            {
                value = value.Substring(0, 60) + "...";
            }
            return value;
        }

        private void InitIconClick()
        {
            ni.MouseClick += (ss, ee) =>
            {
                try
                {
                    // not login, such as Splash window, login window ...
                    if (!IsLogin)
                    {
                        if (PopupTargetWin == null && !OnGoing_Login)
                        {
                            app.UIMediator.OnShowChooseServerWin();
                            return;
                        }

                        if (PopupTargetWin.Visibility != Visibility.Visible)
                        {
                            PopupTargetWin.Show();
                            PopupTargetWin.Activate();
                            PopupTargetWin.WindowState = WindowState.Normal;
                        }
                        else
                        {
                            PopupTargetWin.Hide();
                        }
                    }
                    else // login
                    {
                        if (Keyboard.IsKeyDown(Key.LeftCtrl))
                        {
                            app.UIMediator.OnShowMainWin(null);
                        }
                        else
                        {
                            PopupServiceManager(ss, ee);
                        }
                    }
                }
                catch (Exception e)
                {
                    app.Log.Error(e.Message, e);
                }
            };
        }

        /// <summary>
        /// Will add Logout item after login.
        /// </summary>
        public void RefreshMenuItem()
        {
            InitMenuItem();
            InitText();
        }

        public void MenuItemDisable(string itemName, bool isDisabled)
        {
            if (itemName == CultureStringInfo.MenuItem_Logout)
            {
                itemLog.Enabled = !isDisabled;
            }
            else if (itemName == CultureStringInfo.MenuItem_Exit)
            {
                //itemExit.Enabled = !isDisabled;
            }
        }

        private System.Windows.Forms.MenuItem itemLog;
        //private System.Windows.Forms.MenuItem itemExit;

        private void InitMenuItem()
        {
            ContextMenu.MenuItems.Clear();

            if (OnGoing_Login)
            {
                return;
            }
            // About item
            System.Windows.Forms.MenuItem itemAbout = new System.Windows.Forms.MenuItem();
            itemAbout.Name = CultureStringInfo.MenuItem_About;
            itemAbout.Text = CultureStringInfo.MenuItem_About;
            itemAbout.Click += (ss, ee) =>
            {  
                app.UIMediator.OnShowAboutWindow();
            };
            ContextMenu.MenuItems.Add(itemAbout);

            // logout item
            if (IsLogin)
            {
                itemLog = new System.Windows.Forms.MenuItem();
                itemLog.Name = CultureStringInfo.MenuItem_Logout;
                itemLog.Text = CultureStringInfo.MenuItem_Logout;
                itemLog.Click += (ss, ee) =>
                {
                    app.RequestLogout();
                };

                ContextMenu.MenuItems.Add(itemLog);
            }
            else
            {
                itemLog = new System.Windows.Forms.MenuItem();
                itemLog.Name = CultureStringInfo.MenuItem_Login;
                itemLog.Text = CultureStringInfo.MenuItem_Login;
                itemLog.Click += (ss, ee) =>
                {
                    app.UIMediator.OnShowChooseServerWin();
                };

                ContextMenu.MenuItems.Add(itemLog);
            }

            // Exit item
            //itemExit = new System.Windows.Forms.MenuItem();
            //itemExit.Name = CultureStringInfo.MenuItem_Exit;
            //itemExit.Text = CultureStringInfo.MenuItem_Exit;
            //itemExit.Click += (ss, ee) =>
            //{
            //    app.ManualExit();
            //};

            //ContextMenu.MenuItems.Add(itemExit);

            ni.ContextMenu = ContextMenu;
        }

        private void PopupServiceManager(object ss, System.Windows.Forms.MouseEventArgs ee)
        {
            try
            {
                if (ee.Button != System.Windows.Forms.MouseButtons.Left)
                    return;

                // Show or Hide Service Manager
                if (app.ServiceManagerWin == null)
                    return;

                if (app.ServiceManagerWin.IsDeActivedEventInvoke)
                {
                    // reset IsDeActivedEventInvoke
                    app.ServiceManagerWin.IsDeActivedEventInvoke = false;
                    return;
                }

                if (app.ServiceManagerWin.Visibility != Visibility.Visible)
                {
                    app.ServiceManagerWin.Show();
                    app.ServiceManagerWin.Activate();
                    app.ServiceManagerWin.WindowState = WindowState.Normal;
                }
                else
                {
                    app.ServiceManagerWin.Hide();
                }

            }
            catch (Exception ex)
            {
                Console.WriteLine(ex.ToString());
            }
        }
    }
}
