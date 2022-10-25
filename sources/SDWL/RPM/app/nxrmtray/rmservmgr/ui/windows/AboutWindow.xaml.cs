using System;
using System.Collections.Generic;
using System.ComponentModel;
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
    public class AboutWinViewModel : INotifyPropertyChanged
    {
        public string Version
        {
            get { return "Version " + "2022.07 "+"("+System.Reflection.Assembly.GetExecutingAssembly().GetName().Version.ToString()+")"; }
        }

        public event PropertyChangedEventHandler PropertyChanged;

        protected void OnPropertyChanged(string propertyName)
        {
            PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(propertyName));
        }
    }

    /// <summary>
    /// Interaction logic for AboutWindow.xaml
    /// </summary>
    public partial class AboutWindow : Window
    {
        private AboutWinViewModel viewModel = new AboutWinViewModel();

        public AboutWindow()
        {
            InitializeComponent();

            this.DataContext = viewModel;
        }

        private void Window_KeyUp(object sender, KeyEventArgs e)
        {
            if (e.Key == Key.Escape || e.Key == Key.Enter)
            {
                this.Close();
            }
        }
    }
}
