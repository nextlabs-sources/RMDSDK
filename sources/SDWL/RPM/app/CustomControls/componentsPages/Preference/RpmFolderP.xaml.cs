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
using System.Globalization;

namespace CustomControls.componentPages.Preference
{

    public class MyFolderCount2EditButtonVisibility : IValueConverter
    {
        public object Convert(object value, Type targetType, object parameter, CultureInfo culture)
        {
            try
            {
                if ((int)value > 0)
                {
                    return Visibility.Visible;
                }
                else
                {
                    return Visibility.Collapsed;
                }
            }
            catch (Exception)
            {
                return Visibility.Collapsed;
            }
        }

        public object ConvertBack(object value, Type targetType, object parameter, CultureInfo culture)
        {
            throw new NotImplementedException();
        }
    }


    public class MyFolderCount2AddButtonVisibility : IValueConverter
    {
        public object Convert(object value, Type targetType, object parameter, CultureInfo culture)
        {
            try
            {
                if ((int)value>0)
                {
                    return Visibility.Collapsed;
                }
                else
                {
                    return Visibility.Visible;
                }
           
            }
            catch (Exception)
            {
                return Visibility.Visible;
            }
        }

        public object ConvertBack(object value, Type targetType, object parameter, CultureInfo culture)
        {
            throw new NotImplementedException();
        }
    }

    public class Bool2TextConverter : IValueConverter
    {
        public object Convert(object value, Type targetType, object parameter, CultureInfo culture)
        {
            try
            {
                if ((bool)value)
                {
                    return "On";
                }
                return "Off";
            }
            catch (Exception)
            {
                return "";
            }
        }

        public object ConvertBack(object value, Type targetType, object parameter, CultureInfo culture)
        {
            throw new NotImplementedException();
        }
    }

    public class Bool2TextForeground : IValueConverter
    {
        public object Convert(object value, Type targetType, object parameter, CultureInfo culture)
        {
            try
            {
                if ((bool)value)
                {
                    return new SolidColorBrush(Color.FromRgb(0x21, 0x96, 0x53));
                }
                return new SolidColorBrush(Color.FromRgb(0xEB, 0x57, 0x57));
            }
            catch (Exception)
            {
                return "";
            }
        }

        public object ConvertBack(object value, Type targetType, object parameter, CultureInfo culture)
        {
            throw new NotImplementedException();
        }
    }

    public class MyFolderItem : INotifyPropertyChanged
    {
        //private BitmapImage icon;
        private string folderName;
        private string folderPath;
        private bool autoProtection;
        private Dictionary<string, List<string>> selectedClassification;

        // public BitmapImage Icon { get=> icon; set { icon = value; OnPropertyChanged("Icon");}}

        public string FolderName { get => folderName; set { folderName = value; OnPropertyChanged("FolderName"); } }

        public string FolderPath { get => folderPath; set { folderPath = value; OnPropertyChanged("FolderPath"); } }

        public bool AutoProtection { get => autoProtection; set { autoProtection = value; OnPropertyChanged("AutoProtection"); } }

        public Dictionary<string, List<string>> SelectedClassification { get => selectedClassification; set { selectedClassification = value; OnPropertyChanged("SelectedClassification"); } }

        public event PropertyChangedEventHandler PropertyChanged;
        protected virtual void OnPropertyChanged(string propertyName)
        {
            PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(propertyName));
        }
    }


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
        private static RoutedCommand editMyFolderItem;
        private static RoutedCommand removeMyFolderItem;
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

            editMyFolderItem = new RoutedCommand(
              "EditMyFolderItem", typeof(RpmP_DataCommands));

            removeMyFolderItem = new RoutedCommand(
              "RemoveMyFolderItem", typeof(RpmP_DataCommands));
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

        public static RoutedCommand EditMyFolderItem
        {
            get { return editMyFolderItem; }
        }

        public static RoutedCommand RemoveMyFolderItem
        {
            get { return removeMyFolderItem; }
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
        private ObservableCollection<MyFolderItem> myFolderList = new ObservableCollection<MyFolderItem>();
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

        public ObservableCollection<MyFolderItem> MyFolderList { get => myFolderList; set { myFolderList = value; OnPropertyChanged("MyFolderList"); } }

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
