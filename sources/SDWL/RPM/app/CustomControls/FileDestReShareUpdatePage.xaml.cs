using CustomControls.common.sharedResource;
using CustomControls.components;
using CustomControls.components.RightsDisplay.model;
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

namespace CustomControls
{
    /// <summary>
    /// FileDestReShareUpdatePage.xaml  DataCommands
    /// </summary>
    public class FDReShareUpdate_DataCommands
    {
        private static RoutedCommand update;
        private static RoutedCommand close;
        private static RoutedCommand revoke;
        static FDReShareUpdate_DataCommands()
        {
            // if the IsEnable property of positive button have bindinged,
            // best not binding input gesture in Command. 
            // otherwise user can trigger command by input gesture when button isEnable property is false.
            InputGestureCollection input = new InputGestureCollection();
            input.Add(new KeyGesture(Key.Enter));
            update = new RoutedCommand(
              "Update", typeof(FDReShareUpdate_DataCommands), input);

            input = new InputGestureCollection();
            input.Add(new KeyGesture(Key.Escape));
            close = new RoutedCommand(
              "Close", typeof(FDReShareUpdate_DataCommands), input);

            revoke = new RoutedCommand("Revoke",typeof(FDReShareUpdate_DataCommands));
        }

        /// <summary>
        ///  FileDestReShareUpdatePage.xaml update button command
        /// </summary>
        public static RoutedCommand Update
        {
            get { return update; }
        }
        /// <summary>
        /// FileDestReShareUpdatePage.xaml close button command
        /// </summary>
        public static RoutedCommand Close
        {
            get { return close; }
        }
        /// <summary>
        /// FileDestReShareUpdatePage.xaml revoke button command
        /// </summary>
        public static RoutedCommand Revoke
        {
            get { return revoke; }
        }
    }

    /// <summary>
    /// ViewModel for FileDestReShareUpdatePage.xaml
    /// </summary>
    public class FileDestReShareUpdateViewModel : INotifyPropertyChanged
    {
        private FileDestReShareUpdatePage host;
        private CaptionDescViewMode captionViewMode;
        private string notify = "";

        private AdhocAndClassifiedRightsViewModel adhocAndClassifiedRights;

        private ObservableCollection<Project> projectList = new ObservableCollection<Project>();

        private Visibility revokeVisibility = Visibility.Collapsed;
        private string positiveContent;

        public FileDestReShareUpdateViewModel(FileDestReShareUpdatePage page)
        {
            host = page;
            captionViewMode = host.captionDesc.ViewModel;
            adhocAndClassifiedRights = host.adhocAndclassifiedRights.ViewModel;
            positiveContent = host.TryFindResource("Windows_Btn_OK").ToString();
        }

        /// <summary>
        /// CaptionDesc component viewModel, can set title, describle, file name, original file tags...
        /// </summary>
        public CaptionDescViewMode CaptionViewMode { get => captionViewMode; }

        /// <summary>
        /// Notify string,defult value is "".
        /// </summary>
        public string Notify { get => notify; set { notify = value; OnPropertyChanged("Notify"); } }

        /// <summary>
        /// Adhoc and Classified(Central policy) Rights UI ViewModel
        /// </summary>
        public AdhocAndClassifiedRightsViewModel AdhocAndClassifiedRightsVM { get => adhocAndClassifiedRights; }

        /// <summary>
        /// FileDestinationSharePage Project data model list, use for init data, also can get checked items by IsChecked property.
        /// </summary>
        public ObservableCollection<Project> ProjectList { get => projectList; }

        /// <summary>
        /// revoke button visibility,defult value is 'Collapsed'
        /// </summary>
        public Visibility RevokeVisibility { get => revokeVisibility; set { revokeVisibility = value; OnPropertyChanged("RevokeVisibility"); } }

        /// <summary>
        /// Positive button content, defult value is 'OK'.
        /// </summary>
        public string PositiveContent { get => positiveContent; set { positiveContent = value; OnPropertyChanged("PositiveContent"); } }


        protected void OnPropertyChanged(string propertyName)
        {
            PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(propertyName));
        }
        public event PropertyChangedEventHandler PropertyChanged;
    }

    /// <summary>
    /// Interaction logic for FileDestReShareUpdatePage.xaml
    /// </summary>
    public partial class FileDestReShareUpdatePage : Page
    {
        private FileDestReShareUpdateViewModel viewModel;

        public FileDestReShareUpdatePage()
        {
            this.Resources.MergedDictionaries.Add(SharedDictionaryManager.StringResource);
            this.Resources.MergedDictionaries.Add(SharedDictionaryManager.TabItemStyle);
            this.Resources.MergedDictionaries.Add(SharedDictionaryManager.UnifiedBtnStyle);

            InitializeComponent();

            this.DataContext = viewModel = new FileDestReShareUpdateViewModel(this);
        }

        public FileDestReShareUpdateViewModel ViewModel { get => viewModel; set => this.DataContext = viewModel = value; }
    }
}
