using ServiceManager.rmservmgr.app.recentNotification;
using ServiceManager.rmservmgr.ui.windows.serviceManager.viewModel;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Interop;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;

namespace ServiceManager
{
    /// <summary>
    /// Interaction logic for ServiceManagerWin.xaml
    /// </summary>
    public partial class ServiceManagerWin : Window
    {
        private ServiceManagerViewModel viewModel;
        private ServiceManagerApp app = ServiceManagerApp.Singleton;
        private const long BROADCAST_QUERY_DENY = 0x424D5144;

        // For fix Bug 57582 - [PCV]Repeat click on the tray icon: need close the main pane if it already opened 
        public bool IsDeActivedEventInvoke { get; set; }

        public ServiceManagerWin()
        {
            InitializeComponent();

            this.DataContext = viewModel = new ServiceManagerViewModel(this);
        }

        /// <summary>
        /// Show NotifyWindow, will not record in DB file
        /// </summary>
        /// <param name="messagePara"></param>
        public void ShowNotifyWindow(MessagePara messagePara)
        {
            viewModel.ShowNotifyWindow(messagePara);
        }

        private void Window_Loaded(object sender, RoutedEventArgs e)
        {
            (PresentationSource.FromVisual(this) as HwndSource).AddHook(new HwndSourceHook(WndProc));
        }

        private IntPtr WndProc(IntPtr hwnd, int msg, IntPtr wParam, IntPtr lParam, ref bool handled)
        {
            try
            {
               if(msg == app.WM_CHECK_IF_ALLOW_LOGOUT)
                {
                   // If don't allow to logout, do following:
                   // handled = true;
                   // return new IntPtr(BROADCAST_QUERY_DENY);
                } else if (msg == app.WM_START_LOGOUT_ACTION)
                {
                    app.ExecuteLogout();
                }

                return hwnd;
            }
            catch (Exception e)
            {
                Console.WriteLine(e.ToString());
                return IntPtr.Zero;
            }
        }

        private void Window_KeyUp(object sender, KeyEventArgs e)
        {
            if (e.Key == Key.Escape)
            {
                this.Hide();
            }
        }

        private void Window_Activated(object sender, EventArgs e)
        {
            // Control the service manager location
            this.Left = SystemParameters.WorkArea.Right - this.Width;
            this.Top = SystemParameters.WorkArea.Height - this.Height;

            viewModel.OnActivated();
        }

        private void Window_Deactivated(object sender, EventArgs e)
        {
            if (this.Visibility == Visibility.Visible)
            {
                this.Hide();
            }

            viewModel.OnDeactivated();

            IsDeActivedEventInvoke = true;

            Task.Factory.StartNew(()=> {
                Thread.Sleep(500);
                IsDeActivedEventInvoke = false;
            });
        }

    }
}
