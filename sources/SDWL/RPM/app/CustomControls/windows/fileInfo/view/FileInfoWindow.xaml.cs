using CustomControls.common.sharedResource;
using CustomControls.windows.fileInfo.viewModel;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Shapes;

namespace CustomControls.windows.fileInfo.view
{
    /// <summary>
    /// Interaction logic for FileInfoWindow.xaml
    /// </summary>
    public partial class FileInfoWindow : Window
    {
        private FileInfoWindowViewModel mVewModel;

        public FileInfoWindowViewModel ViewModel
        {
            get
            {
                return mVewModel;
            }

            set
            {
                this.DataContext = mVewModel = value;
            }
        }
        

        public FileInfoWindow()
        {
            this.Resources.MergedDictionaries.Add(SharedDictionaryManager.UnifiedBtnStyle);
            this.Resources.MergedDictionaries.Add(SharedDictionaryManager.StringResource);
            InitializeComponent();
        }

        private void Close_CanExecute(object sender, CanExecuteRoutedEventArgs e)
        {
            e.CanExecute = true;
            e.Handled = true;
        }

        private void Close_Executed(object sender, ExecutedRoutedEventArgs e)
        {
            this.Close();
            e.Handled = true;
        }
    }
}
