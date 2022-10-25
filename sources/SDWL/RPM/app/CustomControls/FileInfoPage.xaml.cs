using CustomControls.common.helper;
using CustomControls.common.sharedResource;
using CustomControls.components;
using CustomControls.components.RightsDisplay.model;
using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
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
    #region Convert
    public class EmailListVisibleConverter : IValueConverter
    {
        public object Convert(object value, Type targetType, object parameter, CultureInfo culture)
        {
            return (int)value > 0 ? Visibility.Visible : Visibility.Collapsed;
        }

        public object ConvertBack(object value, Type targetType, object parameter, CultureInfo culture)
        {
            throw new NotImplementedException();
        }
    }

    public class NameToBackground : IValueConverter
    {
        public object Convert(object value, Type targetType, object parameter, CultureInfo culture)
        {
            return NameColorHelper.SelectionBackgroundColor(value.ToString());

        }

        public object ConvertBack(object value, Type targetType, object parameter, CultureInfo culture)
        {
            throw new NotImplementedException();
        }
    }

    public class NameToForeground : IValueConverter
    {
        public object Convert(object value, Type targetType, object parameter, CultureInfo culture)
        {
            return NameColorHelper.SelectionTextColor(value.ToString());
        }

        public object ConvertBack(object value, Type targetType, object parameter, CultureInfo culture)
        {
            throw new NotImplementedException();
        }
    }

    public class CheckoutFirstChar : IValueConverter
    {
        public object Convert(object value, Type targetType, object parameter, CultureInfo culture)
        {
            if (string.IsNullOrEmpty(value.ToString()))
            {
                return "";
            }
            else
            {
                return value.ToString().Substring(0, 1).ToUpper();
            }
        }

        public object ConvertBack(object value, Type targetType, object parameter, CultureInfo culture)
        {
            throw new NotImplementedException();
        }
    }

    public class AdhocRightsVisibleConverter : IValueConverter
    {
        public object Convert(object value, Type targetType, object parameter, CultureInfo culture)
        {
            Visibility accessDeny = (Visibility)value;
            if (accessDeny == Visibility.Visible)
            {
                return Visibility.Collapsed;
            }
            return Visibility.Visible;
        }

        public object ConvertBack(object value, Type targetType, object parameter, CultureInfo culture)
        {
            throw new NotImplementedException();
        }
    }
    #endregion

    /// <summary>
    /// FileInfoPage.xaml  DataCommands
    /// </summary>
    public class FileInfo_DataCommands
    {
        private static RoutedCommand close;
        static FileInfo_DataCommands()
        {
            InputGestureCollection input = new InputGestureCollection();

            input = new InputGestureCollection();
            input.Add(new KeyGesture(Key.Enter));
            close = new RoutedCommand(
              "Close", typeof(FileInfo_DataCommands), input);
        }

        /// <summary>
        /// FileInfoPage.xaml close button command
        /// </summary>
        public static RoutedCommand Close
        {
            get { return close; }
        }
    }

    /// <summary>
    /// ViewModel for FileInfoPage.xaml
    /// </summary>
    public class FileInfoViewModel : INotifyPropertyChanged
    {
        private FileInfoPage host;

        private string fileName;
        private string fileSize;
        private string lastModifiedTime;
        private Thickness sharedDescMargin = new Thickness(200,0,0,0);
        private string sharedDesc;
        private Thickness emailListMargin = new Thickness(200, 9, 0, 0);
        private ObservableCollection<string> emails = new ObservableCollection<string>();

        private FileType fileType = FileType.CentralPolicy;

        // RightsStackPanle.xaml
        private RightsStPanViewModel rightsDisplayViewModel;
        private string adhocAccessDenyText;
        private Visibility adhocAccessDenyVisibility = Visibility.Collapsed;
        // ClassifiedRightsView.xaml
        private ClassifiedRightsViewModel classifiedRightsViewModel;

        public FileInfoViewModel(FileInfoPage host)
        {
            this.host = host;
            rightsDisplayViewModel = this.host.rightSP.ViewModel;
            adhocAccessDenyText = this.host.TryFindResource("ClassifiedRight_No_Permission").ToString();
            classifiedRightsViewModel = this.host.classifiedRights.ViewModel;
        }

        /// <summary>
        /// FileName, defult value is null
        /// </summary>
        public string FileName { get => fileName; set { fileName = value; OnPropertyChanged("FileName"); } }

        /// <summary>
        /// File size, defult value is null
        /// </summary>
        public string FileSize { get => fileSize; set { fileSize = value; OnPropertyChanged("FileSize"); } }

        /// <summary>
        /// File last modified time, defult value is null
        /// </summary>
        public string LastModifiedTime { get => lastModifiedTime; set { lastModifiedTime = value; OnPropertyChanged("LastModifiedTime"); } }

        /// <summary>
        /// Shared describe margin, defult value is '200,0,0,0'
        /// </summary>
        public Thickness SharedDescMargin { get => sharedDescMargin; set { sharedDescMargin = value; OnPropertyChanged("SharedDescMargin"); } }

        /// <summary>
        /// Shared describe, defult value is null
        /// </summary>
        public string SharedDesc { get => sharedDesc; set { sharedDesc = value; OnPropertyChanged("SharedDesc"); } }

        /// <summary>
        /// Email list margin, defult value is '200,9,0,0'
        /// </summary>
        public Thickness EmailListMargin { get => emailListMargin; set { emailListMargin = value; OnPropertyChanged("EmailListMargin"); } }

        /// <summary>
        /// Email list
        /// </summary>
        public ObservableCollection<string> Emails { get => emails; set { emails = value; OnPropertyChanged("Emails"); } }

        /// <summary>
        /// It's control adhocRights and centralTag UI, defult value is FileType.CentralPolicy and display centralTag UI
        /// </summary>
        public FileType FileType { get => fileType; set { fileType = value; OnPropertyChanged("FileType"); } }

        /// <summary>
        /// Adhoc rights UI(contain rights, warterMark, expiry date) ViewModel
        /// </summary>
        public RightsStPanViewModel AdhocRightsVM { get => rightsDisplayViewModel; }

        /// <summary>
        /// Adhoc AccessDeniedView UI display text, defult value is "You do not have any right on this file."
        /// </summary>
        public string AdhocAccessDenyText { get => adhocAccessDenyText; set { adhocAccessDenyText = value; OnPropertyChanged("AdhocAccessDenyText"); } }

        /// <summary>
        /// Adhoc AccessDeniedView UI visibility,defult vallue is Collapsed. if this value is Visibility.Visible, the RightsStackPanle UI will Collapsed.
        /// </summary>
        public Visibility AdhocAccessDenyVisibility { get => adhocAccessDenyVisibility; set { adhocAccessDenyVisibility = value; OnPropertyChanged("AdhocAccessDenyVisibility"); } }

        /// <summary>
        /// Classified(Central policy) rights UI(contain tag, rights, warterMark, expiry date) ViewModel
        /// </summary>
        public ClassifiedRightsViewModel ClassifiedRightsVM { get => classifiedRightsViewModel; }


        public event PropertyChangedEventHandler PropertyChanged;
        protected void OnPropertyChanged(string propertyName)
        {
            PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(propertyName));
        }
    }

    /// <summary>
    /// Interaction logic for FileInfoPage.xaml
    /// </summary>
    public partial class FileInfoPage : Page
    {
        private FileInfoViewModel viewModel;
        public FileInfoPage()
        {
            this.Resources.MergedDictionaries.Add(SharedDictionaryManager.UnifiedBtnStyle);
            this.Resources.MergedDictionaries.Add(SharedDictionaryManager.StringResource);
            InitializeComponent();
            this.DataContext = viewModel = new FileInfoViewModel(this);
        }

        public FileInfoPage(bool isOnlyDisplayRights)
        {
            this.Resources.MergedDictionaries.Add(SharedDictionaryManager.UnifiedBtnStyle);
            this.Resources.MergedDictionaries.Add(SharedDictionaryManager.StringResource);
            InitializeComponent();

            if (isOnlyDisplayRights)
            {
                this.sp.Visibility = Visibility.Collapsed;
            }
            this.DataContext = viewModel = new FileInfoViewModel(this);

        }

        /// <summary>
        /// FileInfoPage.xaml ViewModel
        /// </summary>
        public FileInfoViewModel ViewModel { get => viewModel; }
    }
}
