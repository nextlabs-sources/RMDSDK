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

namespace ServiceManager.rmservmgr.ui.windows
{
    /// <summary>
    /// Interaction logic for InitializeWindow.xaml
    /// </summary>
    public partial class InitializeWindow : Window
    {
        public InitializeWindow()
        {
            InitializeComponent();
        }

        private void Window_Loaded(object sender, RoutedEventArgs e)
        {
            this.Left = SystemParameters.WorkArea.Right - this.Width;
            this.Top = SystemParameters.WorkArea.Height - this.Height;
        }
    }
}
