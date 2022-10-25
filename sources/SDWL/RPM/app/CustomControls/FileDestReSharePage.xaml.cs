using CustomControls.common.sharedResource;
using CustomControls.components;
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
    /// <summary>
    /// Project data model
    /// </summary>
    public class Project : INotifyPropertyChanged
    {
        private int id;
        private string name;
        private bool isOwner;
        private DateTime createTime;
        private long fileCount;
        private string invitedBy;
        private bool isChecked;

        public int Id { get => id; set { id = value; OnPropertyChanged("Id"); } }
        public string Name { get => name; set { name = value; OnPropertyChanged("Name"); } }
        public bool IsOwner { get => isOwner; set { isOwner = value; OnPropertyChanged("IsOwner"); } }
        public DateTime CreateTime { get => createTime; set { createTime = value; OnPropertyChanged("CreateTime"); } }
        public long FileCount { get => fileCount; set { fileCount = value; OnPropertyChanged("FileCount"); } }
        public string InvitedBy { get => invitedBy; set { invitedBy = value; OnPropertyChanged("InvitedBy"); } }
        public bool IsChecked { get => isChecked; set { isChecked = value; OnPropertyChanged("IsChecked"); } }


        public event PropertyChangedEventHandler PropertyChanged;
        protected void OnPropertyChanged(string propertyName)
        {
            PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(propertyName));
        }
    }

    /// <summary>
    /// FileDestReSharePage.xaml  DataCommands
    /// </summary>
    public class FDReShare_DataCommands
    {
        private static RoutedCommand share;
        private static RoutedCommand cancel;
        static FDReShare_DataCommands()
        {
            // if the IsEnable property of positive button have bindinged,
            // best not binding input gesture in Command. 
            // otherwise user can trigger command by input gesture when button isEnable property is false.
            share = new RoutedCommand(
              "Share", typeof(FDReShare_DataCommands));

            InputGestureCollection input = new InputGestureCollection();
            input.Add(new KeyGesture(Key.Escape));
            cancel = new RoutedCommand(
              "Cancel", typeof(FDReShare_DataCommands), input);
        }

        /// <summary>
        ///  FileDestReSharePage.xaml share button command
        /// </summary>
        public static RoutedCommand Share
        {
            get { return share; }
        }
        /// <summary>
        /// FileDestReSharePage.xaml cancel button command
        /// </summary>
        public static RoutedCommand Cancel
        {
            get { return cancel; }
        }
    }

    #region Convert
    public class ProjectIconConverter : IValueConverter
    {
        public object Convert(object value, Type targetType, object parameter, CultureInfo culture)
        {
            bool IsOwner= (bool)value;
            if (IsOwner)
            {
                return @"/CustomControls;component/resources/icons/projectByMe.png";
            }
            return @"/CustomControls;component/resources/icons/projectByOthers.png";
        }

        public object ConvertBack(object value, Type targetType, object parameter, CultureInfo culture)
        {
            throw new NotImplementedException();
        }

    }
    public class InvitedVisibleConverter : IMultiValueConverter
    {
        public object Convert(object[] value, Type targetType, object parameter, CultureInfo culture)
        {
            bool IsOwner = (bool)value[0];
            string invited = (string)value[1];
            if (IsOwner)
            {
                return Visibility.Collapsed;
            }
            if (string.IsNullOrEmpty(invited))
            {
                return Visibility.Collapsed;
            }
            return Visibility.Visible;
        }

        public object[] ConvertBack(object value, Type[] targetType, object parameter, CultureInfo culture)
        {
            throw new NotImplementedException();
        }
    }
    #endregion

    /// <summary>
    /// ViewModel for FileDestReSharePage.xaml
    /// </summary>
    public class FileDestReShareViewModel : INotifyPropertyChanged
    {
        private FileDestReSharePage host;
        private CaptionDescViewMode captionViewMode;
        private ObservableCollection<Project> projectList =new ObservableCollection<Project>();
        private bool shareBtnIsEnable = true;

        public FileDestReShareViewModel(FileDestReSharePage page)
        {
            host = page;
            host.captionDesc.ViewModel = captionViewMode = new CaptionDescViewMode(host.captionDesc);
        }

        /// <summary>
        /// CaptionDesc component viewModel, can set title, describle, file name, original file tags...
        /// </summary>
        public CaptionDescViewMode CaptionViewMode { get => captionViewMode; }

        /// <summary>
        /// FileDestReSharePage Project data model list, use for init data, also can get checked items by IsChecked property.
        /// </summary>
        public ObservableCollection<Project> ProjectList { get => projectList; }

        /// <summary>
        /// Share button isEnable,defult value is true
        /// </summary>
        public bool ShareBtnIsEnable { get => shareBtnIsEnable; set { shareBtnIsEnable = value; OnPropertyChanged("ShareBtnIsEnable"); } }


        public event PropertyChangedEventHandler PropertyChanged;
        protected void OnPropertyChanged(string propertyName)
        {
            PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(propertyName));
        }
    }
    /// <summary>
    /// Interaction logic for FileDestReSharePage.xaml
    /// </summary>
    public partial class FileDestReSharePage : Page
    {
        private FileDestReShareViewModel viewModel;
        public FileDestReSharePage()
        {
            this.Resources.MergedDictionaries.Add(SharedDictionaryManager.StringResource);
            this.Resources.MergedDictionaries.Add(SharedDictionaryManager.TabItemStyle);
            this.Resources.MergedDictionaries.Add(SharedDictionaryManager.UnifiedBtnStyle);
            InitializeComponent();

            this.DataContext = viewModel = new FileDestReShareViewModel(this);
        }

        /// <summary>
        /// ViewModel for FileDestReSharePage.xaml
        /// </summary>
        public FileDestReShareViewModel ViewModel { get => viewModel; set => this.DataContext = viewModel = value; }
    }
}
