using CustomControls.common.sharedResource;
using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.ComponentModel;
using System.Linq;
using System.Text;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;

namespace CustomControls.componentPages.Preference
{
    /// <summary>
    /// RpmFolderP.xaml RpmFolder List item data model
    /// </summary>
    public class FolderItem : INotifyPropertyChanged
    {
        public BitmapImage Icon { get; set; }

        public string FolderName { get; set; }

        public string FolderPath { get; set; }

        private bool isChecked;
        public bool IsChecked { get => isChecked; set { isChecked = value; OnPropertyChanged("IsChecked"); } }

        public event PropertyChangedEventHandler PropertyChanged;
        protected virtual void OnPropertyChanged(string propertyName)
        {
            PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(propertyName));
        }
    }

    /// <summary>
    /// RpmFolderP.xaml  DataCommands
    /// </summary>
    public class RpmP_DataCommands
    {
        private static RoutedCommand browse;
        private static RoutedCommand apply;
        private static RoutedCommand reset;
        private static RoutedCommand cancel;
        static RpmP_DataCommands()
        {
            browse = new RoutedCommand(
             "Browse", typeof(RpmP_DataCommands));

            apply = new RoutedCommand(
              "Apply", typeof(RpmP_DataCommands));

            reset = new RoutedCommand(
             "Reset", typeof(RpmP_DataCommands));

            cancel = new RoutedCommand(
              "Cancel", typeof(RpmP_DataCommands));
        }
        /// <summary>
        /// RpmFolderP page browse button command
        /// </summary>
        public static RoutedCommand Browse
        {
            get { return browse; }
        }
        /// <summary>
        /// RpmFolderP page apply button command
        /// </summary>
        public static RoutedCommand Apply
        {
            get { return apply; }
        }
        /// <summary>
        /// RpmFolderP page Reset button command
        /// </summary>
        public static RoutedCommand Reset
        {
            get { return reset; }
        }
        /// <summary>
        /// RpmFolderP page cancel button command
        /// </summary>
        public static RoutedCommand Cancel
        {
            get { return cancel; }
        }
    }

    /// <summary>
    /// ViewModel for RpmFolderP.xaml
    /// </summary>
    public class RpmFolderPViewModel : INotifyPropertyChanged
    {
        private string rPMpath;
        private bool btnApplyIsEnable;

        private bool? isCheckedAll = false;
        private ObservableCollection<FolderItem> folderList = new ObservableCollection<FolderItem>();
        private bool btnResetIsEnable = true;

        /// <summary>
        /// RPM folder path
        /// </summary>
        public string RPMpath { get => rPMpath; set { rPMpath = value; OnPropertyChanged("RPMpath"); } }

        /// <summary>
        /// Apply button isEnable, use for save 'Apply button' IsEnable status
        /// </summary>
        public bool BtnApplyIsEnable { get => btnApplyIsEnable; set { btnApplyIsEnable = value; OnPropertyChanged("BtnApplyIsEnable"); } }

        /// <summary>
        /// Control IsCheckedAll checkBox isChecked
        /// </summary>
        public bool? IsCheckedAll { get => isCheckedAll; set { isCheckedAll = value; OnPropertyChanged("IsCheckedAll"); } }

        /// <summary>
        /// Folder ListView itemSource
        /// </summary>
        public ObservableCollection<FolderItem> FolderList { get => folderList; set { folderList = value; OnPropertyChanged("FolderList"); } }

        /// <summary>
        /// Reset button isEnable, use for save 'Reset button' IsEnable status
        /// </summary>
        public bool BtnResetIsEnable { get => btnResetIsEnable; set { btnResetIsEnable = value; OnPropertyChanged("BtnResetIsEnable"); } }


        public event PropertyChangedEventHandler PropertyChanged;
        protected void OnPropertyChanged(string propertyName)
        {
            PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(propertyName));
        }
    }

    /// <summary>
    /// Interaction logic for RpmFolderP.xaml
    /// </summary>
    public partial class RpmFolderP : Page
    {
        private RpmFolderPViewModel viewModel;
        public RpmFolderP()
        {
            this.Resources.MergedDictionaries.Add(SharedDictionaryManager.StringResource);
            this.Resources.MergedDictionaries.Add(SharedDictionaryManager.UnifiedBtnStyle);

            InitializeComponent();

            this.DataContext = viewModel = new RpmFolderPViewModel();
        }

        /// <summary>
        /// ViewModel for RpmFolderP.xaml
        /// </summary>
        public RpmFolderPViewModel ViewModel { get => viewModel; set { this.DataContext = viewModel = value; } }

        private void ClearBtn_Click(object sender, RoutedEventArgs e)
        {
            viewModel.RPMpath = "";
        }

        private void Tbx_content_TextChanged(object sender, TextChangedEventArgs e)
        {
            Console.WriteLine("Tbx_content_TextChanged");
            viewModel.BtnApplyIsEnable = true;
        }

        private void AllCheckBox_Checked_UnChecked(object sender, RoutedEventArgs e)
        {
            if (viewModel.IsCheckedAll == true)
            {
                foreach (var item in viewModel.FolderList)
                {
                    item.IsChecked = true;
                }
            }
            else
            {
                foreach (var item in viewModel.FolderList)
                {
                    item.IsChecked = false;
                }
            }
        }

        private void CheckBox_Checked_UnChecked(object sender, RoutedEventArgs e)
        {
            if (viewModel.FolderList.All(x => x.IsChecked))
                viewModel.IsCheckedAll = true;
            else if (viewModel.FolderList.All(x => !x.IsChecked))
                viewModel.IsCheckedAll = false;
            else
                viewModel.IsCheckedAll = null;
        }
    }
}
