using CustomControls.common.sharedResource;
using CustomControls.components;
using CustomControls.components.TreeView.viewModel;
using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Globalization;
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

namespace CustomControls
{
    public enum ProtectLocation
    {
        LocalDrive,
        CentralLocation
    }

    #region Convert
    public class ProtectLocationToBoolenConverter : IValueConverter
    {
        //ProtectLocation to radiobutton
        public object Convert(object value, Type targetType, object parameter, CultureInfo culture)
        {
            ProtectLocation type = (ProtectLocation)value;
            return type == (ProtectLocation)int.Parse(parameter.ToString());
        }
        //radiobutton to ProtectLocation
        public object ConvertBack(object value, Type targetType, object parameter, CultureInfo culture)
        {
            bool isChecked = (bool)value;
            if (!isChecked)
            {
                return null;
            }
            return (ProtectLocation)int.Parse(parameter.ToString());
        }
    }

    public class ProtectLocationToLocalDriveVisibilityConvert : IValueConverter
    {
        public object Convert(object value, Type targetType, object parameter, CultureInfo culture)
        {
            ProtectLocation type = (ProtectLocation)value;
            if (type == ProtectLocation.LocalDrive)
            {
                return Visibility.Visible;
            }
            return Visibility.Collapsed;
        }

        public object ConvertBack(object value, Type targetType, object parameter, CultureInfo culture)
        {
            throw new NotImplementedException();
        }
    }
    public class ProtectLocationToCentralVisibilityConvert : IValueConverter
    {
        public object Convert(object value, Type targetType, object parameter, CultureInfo culture)
        {
            ProtectLocation type = (ProtectLocation)value;
            if (type == ProtectLocation.CentralLocation)
            {
                return Visibility.Visible;
            }
            return Visibility.Collapsed;
        }

        public object ConvertBack(object value, Type targetType, object parameter, CultureInfo culture)
        {
            throw new NotImplementedException();
        }
    }
    #endregion

    /// <summary>
    /// FileDestSelectPage.xaml  DataCommands
    /// </summary>
    public class FDSelect_DataCommands
    {
        private static RoutedCommand positive;
        private static RoutedCommand cancel;
        static FDSelect_DataCommands()
        {
            // if the IsEnable property of positive button have bindinged,
            // best not binding input gesture in Command. 
            // otherwise user can trigger command by input gesture when button isEnable property is false.
            positive = new RoutedCommand(
              "Positive", typeof(FDSelect_DataCommands));

            InputGestureCollection input = new InputGestureCollection();
            input.Add(new KeyGesture(Key.Escape));
            cancel = new RoutedCommand(
              "Cancel", typeof(FDSelect_DataCommands), input);
        }

        /// <summary>
        ///  FileDestSelectPage.xaml positive button command
        /// </summary>
        public static RoutedCommand Positive
        {
            get { return positive; }
        }
        /// <summary>
        /// FileDestSelectPage.xaml cancel button command
        /// </summary>
        public static RoutedCommand Cancel
        {
            get { return cancel; }
        }
    }

    /// <summary>
    /// ViewModel for FileDestSelectPage.xaml
    /// </summary>
    public class FileDestSelectViewMode : INotifyPropertyChanged
    {
        private FileDestSelectPage host;
        // CaptionDesc.xaml ViewModel
        private CaptionDesc4ViewMode caption4ViewModel;
        private Visibility savePathSpVisibility = Visibility.Visible;
        private string savePath = "";
        private string radioSpDescribe;
        private Visibility raidoVisibility = Visibility.Visible;
        private ProtectLocation protectLocation = ProtectLocation.CentralLocation;
        private bool localDriveRdIsEnable = true;
        private bool centralLocationRdIsEnable = true;
        private LocalDriveViewModel localDriveVM;
        private TreeViewViewModel treeViewVM;
        private string positiveBtnContent = "";
        private bool positiveBtnIsEnable = true;
        
        public FileDestSelectViewMode(FileDestSelectPage page)
        {
            host = page;
            caption4ViewModel = host.captionCom4.ViewModel;
            localDriveVM = host.locralDrive.ViewModel;
            treeViewVM = host.treeView.ViewModel;
            radioSpDescribe = host.TryFindResource("FileDestSelect_Sp_Lable").ToString();
            positiveBtnContent = host.TryFindResource("FileDestSelect_Select_Btn").ToString();
        }

        /// <summary>
        /// RadioButton_Checked event hander
        /// </summary>
        public event RoutedEventHandler OnRadioButton_Checked;

        /// <summary>
        /// TreeViewItem selected changed event handler
        /// </summary>
        public event RoutedPropertyChangedEventHandler<object> OnTreeViewItemSelectedChanged;

        /// <summary>
        /// CaptionDesc component viewModel, can set title, describle, file name, original file tags...
        /// </summary>
        public CaptionDesc4ViewMode Caption4VM { get => caption4ViewModel; }

        /// <summary>
        /// Save path StackPanel visible, defult value is Visbile
        /// </summary>
        public Visibility SavePathStpVisibility { get => savePathSpVisibility; set { savePathSpVisibility = value; OnPropertyChanged("SavePathStpVisibility"); } }

        /// <summary>
        /// Display save path, defult value is "", it's part of SavePath StackPanel
        /// </summary>
        public string SavePath { get => savePath; set { savePath = value; OnPropertyChanged("SavePath"); } }

        /// <summary>
        /// Display radio stackPanel description, defult value is "Select a location to save the file"
        /// </summary>
        public string RadioSpDescribe { get => radioSpDescribe; set { radioSpDescribe = value; OnPropertyChanged("RadioSpDescribe"); } }

        /// <summary>
        /// Control localDrive and CentralLocation RadioBtn visibility, defulte value is Visible.
        /// </summary>
        public Visibility RaidoVisibility { get => raidoVisibility; set { raidoVisibility = value; OnPropertyChanged("RaidoVisibility"); } }

        /// <summary>
        /// Protect file location, corresponding localDrive and centralLocation radioButton checked. defult value is CentralLocation.
        /// </summary>
        public ProtectLocation ProtectLocation { get => protectLocation; set { protectLocation = value; OnPropertyChanged("ProtectLocation"); } }

        /// <summary>
        /// Local Drive RadioButton IsEnable, defult value is true
        /// </summary>
        public bool LocalDriveRdIsEnable { get => localDriveRdIsEnable; set { localDriveRdIsEnable = value; OnPropertyChanged("LocalDriveRdIsEnable"); } }

        /// <summary>
        ///  Central location RadioButton IsEnable, defult value is true
        /// </summary>
        public bool CentralLocationRdIsEnable { get => centralLocationRdIsEnable; set { centralLocationRdIsEnable = value; OnPropertyChanged("CentralLocationRdIsEnable"); } }

        /// <summary>
        /// Local drive ViewModel
        /// </summary>
        public LocalDriveViewModel LocalDriveVM { get => localDriveVM; }

        /// <summary>
        /// TreeView ViewModel
        /// </summary>
        public TreeViewViewModel TreeViewVM { get => treeViewVM; }

        /// <summary>
        /// Positive button content, defult value is "Select"
        /// </summary>
        public string PositiveBtnContent { get => positiveBtnContent; set { positiveBtnContent = value; OnPropertyChanged("PositiveBtnContent"); } }

        /// <summary>
        /// Positive button IsEnable, defult value is true
        /// </summary>
        public bool PositiveBtnIsEnable { get => positiveBtnIsEnable; set { positiveBtnIsEnable = value; OnPropertyChanged("PositiveBtnIsEnable"); } }

        /// <summary>
        /// Invoke RadioButton_Checked event handler
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        internal void TriggerRadioButton_Checked(object sender, RoutedEventArgs e)
        {
            OnRadioButton_Checked?.Invoke(sender,e);
        }

        internal void TriggerTreeViewItemSelected_Changed(object sender, RoutedPropertyChangedEventArgs<object> e)
        {
            OnTreeViewItemSelectedChanged?.Invoke(sender, e);
        }

        public event PropertyChangedEventHandler PropertyChanged;
        protected void OnPropertyChanged(string propertyName)
        {
            PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(propertyName));
        }
    }

    /// <summary>
    /// Interaction logic for FileDestSelectPage.xaml
    /// </summary>
    public partial class FileDestSelectPage : Page
    {
        private FileDestSelectViewMode viewMode;
        public FileDestSelectPage()
        {
            this.Resources.MergedDictionaries.Add(SharedDictionaryManager.StringResource);
            this.Resources.MergedDictionaries.Add(SharedDictionaryManager.UnifiedBtnStyle);
            InitializeComponent();
            this.DataContext = viewMode = new FileDestSelectViewMode(this);
        }
        /// <summary>
        /// ViewModel for FileDestSelectPage.xaml
        /// </summary>
        public FileDestSelectViewMode ViewMode { get => viewMode; set => this.DataContext = viewMode = value; }

        private void RadioButton_Checked(object sender, RoutedEventArgs e)
        {
            viewMode.TriggerRadioButton_Checked(sender, e);
        }

        private void TreeView_SelectedItemChanged(object sender, RoutedPropertyChangedEventArgs<object> e)
        {
            viewMode.TriggerTreeViewItemSelected_Changed(sender, e);
        }
    }
}
