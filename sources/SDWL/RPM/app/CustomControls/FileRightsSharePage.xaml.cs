using CustomControls.common.sharedResource;
using CustomControls.components;
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
    /// FileRightsSharePage.xaml  DataCommands
    /// </summary>
    public class FRShare_DataCommands
    {
        private static RoutedCommand addEmail;
        private static RoutedCommand positive;
        private static RoutedCommand back;
        private static RoutedCommand cancel;
        static FRShare_DataCommands()
        {
            // if the IsEnable property of positive button have bindinged,
            // best not binding input gesture in Command. 
            // otherwise user can trigger command by input gesture when button isEnable property is false.

            addEmail = new RoutedCommand(
              "AddEmail", typeof(FRShare_DataCommands));

            positive = new RoutedCommand(
              "Positive", typeof(FRShare_DataCommands));

            back = new RoutedCommand(
              "Back", typeof(FRShare_DataCommands));

            InputGestureCollection input = new InputGestureCollection();
            input.Add(new KeyGesture(Key.Escape));
            cancel = new RoutedCommand(
              "Cancel", typeof(FRShare_DataCommands), input);
        }
        /// <summary>
        /// FileRightsSharePage.xaml addEmail button command
        /// </summary>
        public static RoutedCommand AddEmail
        {
            get { return addEmail; }
        }
        /// <summary>
        /// FileRightsSharePage.xaml positive button command
        /// </summary>
        public static RoutedCommand Positive
        {
            get { return positive; }
        }
        /// <summary>
        ///  FileRightsSharePage.xaml back button command
        /// </summary>
        public static RoutedCommand Back
        {
            get { return back; }
        }
        /// <summary>
        /// FileRightsSharePage.xaml cancel button command
        /// </summary>
        public static RoutedCommand Cancel
        {
            get { return cancel; }
        }
    }

    /// <summary>
    /// ViewModel for FileRightsSharePage.xaml
    /// </summary>
    public class FileRightsShareViewMode : INotifyPropertyChanged
    {
        private FileRightsSharePage host;
        // CaptionDesc4.xaml ViewModel
        private CaptionDesc4ViewMode caption4ViewModel;
        private Thickness savePathMargin = new Thickness(222, 5, 0, 15);
        private SolidColorBrush savePathLabelForegrd = new SolidColorBrush(Color.FromRgb(0X82, 0X82, 0X82));
        private string savePathLabel;
        private string savePath = "";

        // AdhocAndClassifiedRights.xaml
        private AdhocAndClassifiedRightsViewModel adhocAndClassifiedRightsVM;

        private Visibility emailInputVisibility = Visibility.Visible;
        private Thickness emailListMargin = new Thickness(140, 12, 140, 0);
        private ObservableCollection<string> emailList = new ObservableCollection<string>();

        private Visibility msgStpVisibility = Visibility.Visible;
        private Visibility msgExceedInfoVisibility = Visibility.Collapsed;
        private string remainCharacters = "250";
        private string maxCharacters = "/250";

        private string positiveBtnContent = "";
        private bool positiveBtnIsEnable = true;
        private Visibility positiveBtnVisibility = Visibility.Visible;
        private Visibility backBtnVisibility = Visibility.Visible;

        public FileRightsShareViewMode(FileRightsSharePage page)
        {
            host = page;
            caption4ViewModel = host.captionDesc4.ViewModel;
            savePathLabel = host.TryFindResource("FileRightsShare_SavePath_Lable").ToString();
            adhocAndClassifiedRightsVM = host.adhocAndclassifiedRights.ViewModel;
        }

        public event RoutedEventHandler EmailTB_PreviewKeyDown;
        public event RoutedEventHandler EmailTB_TextChanged;
        public event RoutedEventHandler DeleteEmailItem_MouseLeftButtonUp;
        public event RoutedEventHandler MessageTB_TextChanged;


        /// <summary>
        /// CaptionDesc2 component viewModel, can set title and file name
        /// </summary>
        public CaptionDesc4ViewMode Caption4VM { get => caption4ViewModel; }

        /// <summary>
        /// Save path TextBlock margin,defult value is (222, 5, 0, 15)
        /// </summary>
        public Thickness SavePathMargin { get => savePathMargin; set { savePathMargin = value; OnPropertyChanged("SavePathMargin"); } }

        /// <summary>
        /// Save path label foreground, defult value is "#828282"
        /// </summary>
        public SolidColorBrush SavePathLabelForegrd { get => savePathLabelForegrd; set { savePathLabelForegrd = value; OnPropertyChanged("SavePathLabelForegrd"); } }

        /// <summary>
        /// Save path label, it's part of SavePath TextBlock, defult value is "Above file will be protected into"
        /// </summary>
        public string SavePathLabel { get => savePathLabel; set { savePathLabel = value; OnPropertyChanged("SavePathLabel"); } }

        /// <summary>
        /// Save path, it's part of SavePath TextBlock, defult value is ""
        /// </summary>
        public string SavePath { get => savePath; set { savePath = value; OnPropertyChanged("SavePath"); } }

        /// <summary>
        /// Adhoc and Classified(Central policy) Rights UI ViewModel
        /// </summary>
        public AdhocAndClassifiedRightsViewModel AdhocAndClassifiedRightsVM { get => adhocAndClassifiedRightsVM; }

        /// <summary>
        /// Email input textBox and add email button visibility, defult value is Visible
        /// </summary>
        public Visibility EmailInputVisibility { get => emailInputVisibility; set { emailInputVisibility = value; OnPropertyChanged("EmailInputVisibility"); } }

        /// <summary>
        /// Email list ItemsControl margin,defult value is (140, 12, 140, 0)
        /// </summary>
        public Thickness EmailListMargin { get => emailListMargin; set { emailListMargin = value; OnPropertyChanged("EmailListMargin"); } }

        /// <summary>
        /// Email List
        /// </summary>
        public ObservableCollection<string> EmailList { get => emailList; set { emailList = value; OnPropertyChanged("EmailList"); } }

        /// <summary>
        /// Message stackPanel visibility, defult value is Visible
        /// </summary>
        public Visibility MsgStpVisibility { get => msgStpVisibility; set { msgStpVisibility = value; OnPropertyChanged("MsgStpVisibility"); } }

        /// <summary>
        /// Message exceeds the maximum number of characters info visibility, defult value is Collapsed
        /// </summary>
        public Visibility MsgExceedInfoVisibility { get => msgExceedInfoVisibility; set { msgExceedInfoVisibility = value; OnPropertyChanged("MsgExceedInfoVisibility"); } }

        /// <summary>
        /// Number of characters remaining in the message, defult value is '250'
        /// </summary>
        public string RemainCharacters { get => remainCharacters; set { remainCharacters = value; OnPropertyChanged("RemainCharacters"); } }

        /// <summary>
        ///  Message the maximum number of characters, defult value is '/250'
        /// </summary>
        public string MaxCharacters { get => maxCharacters; set { maxCharacters = value; OnPropertyChanged("MaxCharacters"); } }

        /// <summary>
        /// Positive button content, defult value is ""
        /// </summary>
        public string PositiveBtnContent { get => positiveBtnContent; set { positiveBtnContent = value; OnPropertyChanged("PositiveBtnContent"); } }

        /// <summary>
        /// Positive button IsEnable, defult value is true
        /// </summary>
        public bool PositiveBtnIsEnable { get => positiveBtnIsEnable; set { positiveBtnIsEnable = value; OnPropertyChanged("PositiveBtnIsEnable"); } }

        /// <summary>
        /// Positive button Visibility, defult value is Visible
        /// </summary>
        public Visibility PositiveBtnVisibility { get => positiveBtnVisibility; set { positiveBtnVisibility = value; OnPropertyChanged("PositiveBtnVisibility"); } }

        /// <summary>
        /// Back button Visibility, defult value is Visible
        /// </summary>
        public Visibility BackBtnVisibility { get => backBtnVisibility; set { backBtnVisibility = value; OnPropertyChanged("BackBtnVisibility"); } }

     

        internal void Trigger_EmailTB_PreviewKeyDown(object sender, RoutedEventArgs e)
        {
            EmailTB_PreviewKeyDown?.Invoke(sender, e);
        }

        internal void Trigger_EmailTB_TextChanged(object sender, RoutedEventArgs e)
        {
            EmailTB_TextChanged?.Invoke(sender, e);
        }

        internal void Trigger_DeleteEmailItem_MouseLeftButtonUp(object sender, RoutedEventArgs e)
        {
            DeleteEmailItem_MouseLeftButtonUp?.Invoke(sender, e);
        }

        internal void Trigger_MessageTB_TextChanged(object sender, RoutedEventArgs e)
        {
            MessageTB_TextChanged?.Invoke(sender, e);
        }

        public event PropertyChangedEventHandler PropertyChanged;
        protected void OnPropertyChanged(string propertyName)
        {
            PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(propertyName));
        }
    }

    /// <summary>
    /// Interaction logic for FileRightsSharePage.xaml
    /// </summary>
    public partial class FileRightsSharePage : Page
    {
        private FileRightsShareViewMode viewModel;
        public FileRightsSharePage()
        {
            this.Resources.MergedDictionaries.Add(SharedDictionaryManager.StringResource);
            this.Resources.MergedDictionaries.Add(SharedDictionaryManager.UnifiedBtnStyle);

            InitializeComponent();

            this.DataContext = viewModel = new FileRightsShareViewMode(this);
        }
        /// <summary>
        /// ViewModel for FileRightsSharePage.xaml 
        /// </summary>
        public FileRightsShareViewMode ViewMode { get => viewModel; set => this.DataContext = viewModel = value; }

        private void EmailTB_PreviewKeyDown(object sender, KeyEventArgs e)
        {
            ViewMode.Trigger_EmailTB_PreviewKeyDown(sender, e);
        }

        private void EmailTB_TextChanged(object sender, TextChangedEventArgs e)
        {
            ViewMode.Trigger_EmailTB_TextChanged(sender, e);
        }

        private void DeleteEmailItem_MouseLeftButtonUp(object sender, MouseButtonEventArgs e)
        {
            ViewMode.Trigger_DeleteEmailItem_MouseLeftButtonUp(sender, e);
        }

        private void MessageTB_TextChanged(object sender, TextChangedEventArgs e)
        {
            ViewMode.Trigger_MessageTB_TextChanged(sender, e);
        }
    }
}
