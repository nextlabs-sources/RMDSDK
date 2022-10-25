using CustomControls.common.sharedResource;
using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Linq;
using System.Text;
using System.Text.RegularExpressions;
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
    /// AddRepositoryPage.xaml ComboBox data model
    /// </summary>
    public class ExternalRepoItem
    {
        private BitmapImage icon;
        private BitmapImage icon2;
        private string name;
        private bool isSharePoint;
        private bool isSelected;

        /// <summary>
        /// AddRepositoryPage.xaml ComboBox data model
        /// </summary>
        /// <param name="repoIcon">comboBox item icon</param>
        /// <param name="repoIcon2">connect button icon</param>
        /// <param name="repoName">repository name</param>
        /// <param name="isSharepoint">is it SharePoint</param>
        public ExternalRepoItem(BitmapImage repoIcon, BitmapImage repoIcon2, string repoName, bool isSharepoint=false)
        {
            icon = repoIcon;
            icon2 = repoIcon2;
            name = repoName;
            isSharePoint = isSharepoint;
        }

        public BitmapImage Icon { get => icon; set => icon = value; }
        public BitmapImage Icon2 { get => icon2; set => icon2 = value; }
        public string Name { get => name; set => name = value; }
        public bool IsSharePoint { get => isSharePoint; set => isSharePoint = value; }
        public bool IsSelected { get => isSelected; set => isSelected = value; }
    }

    /// <summary>
    /// AddRepositoryPage.xaml  DataCommands
    /// </summary>
    public class AddRepo_DataCommands
    {
        private static RoutedCommand connect;
        private static RoutedCommand positive;
        private static RoutedCommand cancel;
        static AddRepo_DataCommands()
        {
            connect = new RoutedCommand(
              "Connect", typeof(AddRepo_DataCommands));

            // if the IsEnable property of positive button have bindinged,
            // best not binding input gesture in Command. 
            // otherwise user can trigger command by input gesture when button isEnable property is false.
            positive = new RoutedCommand(
              "Positive", typeof(AddRepo_DataCommands));

            InputGestureCollection input = new InputGestureCollection();
            input.Add(new KeyGesture(Key.Escape));
            cancel = new RoutedCommand(
              "Cancel", typeof(AddRepo_DataCommands), input);
        }
        /// <summary>
        /// AddRepositoryPage.xaml connect button command
        /// </summary>
        public static RoutedCommand Connect
        {
            get { return connect; }
        }
        /// <summary>
        ///  AddRepositoryPage.xaml positive button command
        /// </summary>
        public static RoutedCommand Positive
        {
            get { return positive; }
        }
        /// <summary>
        /// AddRepositoryPage.xaml cancel button command
        /// </summary>
        public static RoutedCommand Cancel
        {
            get { return cancel; }
        }
    }

    /// <summary>
    /// ViewModel for AddRepositoryPage.xaml
    /// </summary>
    public class AddRepositoryViewMode : INotifyPropertyChanged
    {
        private List<ExternalRepoItem> externalRepoList = new List<ExternalRepoItem>();
        // display name
        private string displayName = string.Empty;
        private SolidColorBrush displayNameInfoClor = new SolidColorBrush(Color.FromRgb(0x97, 0x97, 0x97));
        private int displayNameRemainLen = 40;
        private int displayNameAllLen = 40;

        // sharePoint special property, site url
        private Visibility siteUrlVisible = Visibility.Collapsed;
        private string siteURL = string.Empty;
        private Visibility uRLIsNotValidVisible = Visibility.Collapsed;
        private bool showRepoAllUser = true;
        private bool isEnableShowRepoAllUser = false;

        private BitmapImage connectBtnIcon;
        private bool connectBtnIsEnable;
        private bool positiveBtnIsEnable;

        public AddRepositoryViewMode()
        {}

        public event RoutedEventHandler ComboBox_SelectionChanged;
        public event RoutedEventHandler TextBox_TextChanged;

        /// <summary>
        /// ComboBox ItemsSource
        /// </summary>
        public List<ExternalRepoItem> ExternalRepoList { get => externalRepoList; set { externalRepoList = value; OnPropertyChanged("ExternalRepoList"); } }

        /// <summary>
        /// Display Name textBox text
        /// </summary>
        public string DisplayName { get => displayName; set { displayName = value; OnPropertyChanged("DisplayName"); } }
        /// <summary>
        /// 'Display Name' prompt foreground
        /// </summary>
        public SolidColorBrush DisplayNameInfoClor { get => displayNameInfoClor; set { displayNameInfoClor = value; OnPropertyChanged("DisplayNameInfoClor"); } }
        /// <summary>
        /// 'Display name' UI remain length,defult value is 40
        /// </summary>
        public int DisplayNameRemainLen { get => displayNameRemainLen; set { displayNameRemainLen = value; OnPropertyChanged("DisplayNameRemainLen"); } }
        /// <summary>
        /// 'Display name' UI All length,defult value is 40
        /// </summary>
        public int DisplayNameAllLen { get => displayNameAllLen; set { displayNameAllLen = value; OnPropertyChanged("DisplayNameAllLen"); } }

        /// <summary>
        /// When select SharePoint should set this Visible, or else set Collapsed. defult value is Collapsed
        /// </summary>
        public Visibility SiteUrlVisible { get => siteUrlVisible; set { siteUrlVisible = value; OnPropertyChanged("SiteUrlVisible"); } }
        /// <summary>
        /// SharePoint site url textBox text
        /// </summary>
        public string SiteURL { get => siteURL; set { siteURL = value; OnPropertyChanged("SiteURL"); } }
        /// <summary>
        /// SharePoint 'URL is not valid' string visibility,defult value is Collapsed
        /// </summary>
        public Visibility URLIsNotValidVisible { get => uRLIsNotValidVisible; set { uRLIsNotValidVisible = value; OnPropertyChanged("URLIsNotValidVisible"); } }
        /// <summary>
        /// SharePoint show repository to all user, defult value is true.
        /// </summary>
        public bool ShowRepoAllUser { get => showRepoAllUser; set { showRepoAllUser = value; OnPropertyChanged("ShowRepoAllUser"); } }
        /// <summary>
        /// SharePoint show repository to all user checkBox isEnabled, defult value is false.
        /// </summary>
        public bool IsEnableShowRepoAllUser { get => isEnableShowRepoAllUser; set { isEnableShowRepoAllUser = value; OnPropertyChanged("IsEnableShowRepoAllUser"); } }

        public BitmapImage ConnectBtnIcon { get => connectBtnIcon; set { connectBtnIcon = value; OnPropertyChanged("ConnectBtnIcon"); } }
        /// <summary>
        /// Connect button IsEnabled, defult value is false
        /// </summary>
        public bool ConnectBtnIsEnable { get => connectBtnIsEnable; set { connectBtnIsEnable = value; OnPropertyChanged("ConnectBtnIsEnable"); } }
        /// <summary>
        /// Positive button IsEnabled,defult value is false
        /// </summary>
        public bool PositiveBtnIsEnable { get => positiveBtnIsEnable; set { positiveBtnIsEnable = value; OnPropertyChanged("PositiveBtnIsEnable"); } }


        internal void TriggerComboxSelectChanged(object sender, RoutedEventArgs e)
        {
            ComboBox_SelectionChanged?.Invoke(sender, e);
        }
        internal void TriggerTBTextChanged(object sender, RoutedEventArgs e)
        {
            TextBox_TextChanged?.Invoke(sender, e);
        }

        public event PropertyChangedEventHandler PropertyChanged;
        protected void OnPropertyChanged(string propertyName)
        {
            PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(propertyName));
        }
    }

    /// <summary>
    /// Interaction logic for AddRepositoryPage.xaml
    /// </summary>
    public partial class AddRepositoryPage : Page
    {
        private AddRepositoryViewMode viewMode;
        public AddRepositoryPage()
        {
            this.Resources.MergedDictionaries.Add(SharedDictionaryManager.StringResource);
            this.Resources.MergedDictionaries.Add(SharedDictionaryManager.UnifiedBtnStyle);
            this.Resources.MergedDictionaries.Add(SharedDictionaryManager.UnifiedCheckBoxStyle);

            InitializeComponent();

            this.DataContext = viewMode = new AddRepositoryViewMode();
        }

        /// <summary>
        ///  ViewModel for AddRepositoryPage.xaml
        /// </summary>
        public AddRepositoryViewMode ViewMode { get => viewMode; set => this.DataContext = viewMode = value; }

        private bool IsSpecialChar(string str)
        {
            Regex regExp = new Regex("[^0-9a-zA-Z\u4e00-\u9fa5]");
            if (regExp.IsMatch(str))
            {
                return true;
            }
            return false;
        }

        private bool CheckUrl(string url)
        {
            bool checkUrl = false;

            Uri uriResult;
            bool result = Uri.TryCreate(url, UriKind.Absolute, out uriResult)
                && (uriResult.Scheme == Uri.UriSchemeHttp || uriResult.Scheme == Uri.UriSchemeHttps);

            if (result)
            {
                checkUrl = true;
            }
            return checkUrl;
        }

        private bool displayNameInvalid = true;
        private bool siteURLInvalid = true;
        private void TextBox_TextChanged(object sender, TextChangedEventArgs e)
        {
            TextBox textBox = e.Source as TextBox;
            string sourceText = textBox.Text;

            if (textBox.Tag.Equals("DisplayName"))
            {
                viewMode.DisplayNameRemainLen = viewMode.DisplayNameAllLen - sourceText.Length;
                if (viewMode.DisplayNameRemainLen < 0)
                {
                    displayNameInvalid = true;
                    viewMode.DisplayNameInfoClor = new SolidColorBrush(Colors.Red);
                    viewMode.ConnectBtnIsEnable = false;
                    viewMode.PositiveBtnIsEnable = false;
                }
                else
                {
                    if (!string.IsNullOrEmpty(sourceText) && !IsSpecialChar(sourceText))
                    {
                        displayNameInvalid = false;
                        viewMode.DisplayNameInfoClor = new SolidColorBrush(Color.FromRgb(0x97, 0x97, 0x97));
                        if (viewMode.SiteUrlVisible == Visibility.Visible)
                        {
                            if (!siteURLInvalid)
                            {
                                viewMode.ConnectBtnIsEnable = true;
                                viewMode.PositiveBtnIsEnable = true;
                            }
                        }
                        else
                        {
                            viewMode.ConnectBtnIsEnable = true;
                            viewMode.PositiveBtnIsEnable = true;
                        }
                    }
                    else
                    {
                        displayNameInvalid = true;
                        viewMode.DisplayNameInfoClor = new SolidColorBrush(Colors.Red);
                        viewMode.ConnectBtnIsEnable = false;
                        viewMode.PositiveBtnIsEnable = false;
                    }
                }
            }
            else // SiteURL
            {
                if (CheckUrl(sourceText))
                {
                    siteURLInvalid = false;
                    viewMode.URLIsNotValidVisible = Visibility.Collapsed;
                    if (!displayNameInvalid)
                    {
                        viewMode.ConnectBtnIsEnable = true;
                        viewMode.PositiveBtnIsEnable = true;
                    }
                }
                else
                {
                    siteURLInvalid = true;
                    viewMode.URLIsNotValidVisible = Visibility.Visible;
                    viewMode.ConnectBtnIsEnable = false;
                    viewMode.PositiveBtnIsEnable = false;
                }
            }

            viewMode.TriggerTBTextChanged(sender, e);
        }

        private void ComboBox_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            ExternalRepoItem addItem = e.AddedItems[0] as ExternalRepoItem;
            addItem.IsSelected = true;

            if (e.RemovedItems.Count > 0)
            {
                ExternalRepoItem removeItem = e.RemovedItems[0] as ExternalRepoItem;
                removeItem.IsSelected = false;
            }

            if (addItem.IsSharePoint)
            {
                viewMode.SiteUrlVisible = Visibility.Visible;
            }
            else
            {
                viewMode.SiteUrlVisible = Visibility.Collapsed;
            }
            viewMode.ConnectBtnIcon = addItem.Icon2;
            viewMode.DisplayName = "";

            viewMode.TriggerComboxSelectChanged(sender, e);
        }
    }
}
