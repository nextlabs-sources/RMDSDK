using ServiceManager.rmservmgr.ui.windows.notifyWindow.viewModel;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Media;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Animation;
using System.Windows.Media.Imaging;
using System.Windows.Shapes;

namespace ServiceManager.rmservmgr.ui.windows.notifyWindow.view
{
    /// <summary>
    /// Interaction logic for NotifyWin.xaml
    /// </summary>
    public partial class NotifyWin : Window
    {
        private NotifyWinViewModel viewModel;

        internal NotifyWinViewModel ViewModel { get => viewModel; set => this.DataContext = viewModel = value; }

        public NotifyWin()
        {
            InitializeComponent();
            this.Loaded += NotifyWin_Loaded;
        }

        public double TopFrom { get; set; }

        private void NotifyWin_Loaded(object sender, RoutedEventArgs e)
        {
            this.UpdateLayout();
            SystemSounds.Asterisk.Play();

            AnimationForShowWin();

            Task.Factory.StartNew(delegate
            {
                int seconds = 5;
                System.Threading.Thread.Sleep(TimeSpan.FromSeconds(seconds));
                this.Dispatcher.Invoke(delegate
                {
                    AnimationCloseWindow();
                });
            });
        }

        private void CancelImg_MouseLeftBtnDown(object sender, MouseButtonEventArgs e)
        {
            AnimationCloseWindow();
        }

        private void ServiceManager_MouseLeftBtnDown(object sender, MouseButtonEventArgs e)
        {
            AnimationCloseWindow();
            ServiceManagerApp.Singleton.ServiceManagerWin?.Show();
        }

        private void AnimationForShowWin()
        {
            double right = System.Windows.SystemParameters.WorkArea.Right;
            this.Top = this.TopFrom - this.ActualHeight;
            DoubleAnimation animation = new DoubleAnimation();
            animation.Duration = new Duration(TimeSpan.FromMilliseconds(500));
            animation.From = right;
            animation.To = right - this.ActualWidth;
            this.BeginAnimation(Window.LeftProperty, animation);
        }

        private void AnimationCloseWindow()
        {
            double right = System.Windows.SystemParameters.WorkArea.Right;
            DoubleAnimation animation = new DoubleAnimation();
            animation.Duration = new Duration(TimeSpan.FromMilliseconds(500));

            animation.Completed += (s, a) => { this.Close(); };
            animation.From = right - this.ActualWidth;
            animation.To = right;
            this.BeginAnimation(Window.LeftProperty, animation);
        }

    }
}
