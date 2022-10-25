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
    /// FileRightsShareResultPage.xaml  DataCommands
    /// </summary>
    public class FRShareResult_DataCommands
    {
        private static RoutedCommand close;

        static FRShareResult_DataCommands()
        {
            // if the IsEnable property of positive button have bindinged,
            // best not binding input gesture in Command. 
            // otherwise user can trigger command by input gesture when button isEnable property is false.

            InputGestureCollection input = new InputGestureCollection();
            input.Add(new KeyGesture(Key.Escape));
            close = new RoutedCommand(
              "Close", typeof(FRShareResult_DataCommands), input);
        }
       
        /// <summary>
        /// FileRightsShareResultPage.xaml close button command
        /// </summary>
        public static RoutedCommand Close
        {
            get { return close; }
        }
    }

    /// <summary>
    /// ViewModel for FileRightsShareResultPage.xaml
    /// </summary>
    public class FileRightsShareResultViewMode : INotifyPropertyChanged
    {
        private FileRightsShareResultPage host;
        // CaptionDesc4.xaml ViewModel
        private CaptionDesc4ViewMode caption4ViewModel;
        private Thickness resultLableMargin = new Thickness(222, 5, 0, 15);
        private SolidColorBrush resultLabelForegrd = new SolidColorBrush(Color.FromRgb(0X27, 0XAE, 0X60));
        private string resultLabel;

        // AdhocAndClassifiedRights.xaml
        private AdhocAndClassifiedRightsViewModel adhocAndClassifiedRightsVM;

        private Thickness emailListMargin = new Thickness(140, 12, 140, 0);
        private ObservableCollection<string> emailList = new ObservableCollection<string>();

        private Visibility msgStpVisibility = Visibility.Visible;
        private string message = string.Empty;

        public FileRightsShareResultViewMode(FileRightsShareResultPage page)
        {
            host = page;
            caption4ViewModel = host.captionDesc4.ViewModel;
            resultLabel = host.TryFindResource("FileRightsShareResult_Successful_Lable").ToString();
            adhocAndClassifiedRightsVM = host.adhocAndclassifiedRights.ViewModel;
        }


        /// <summary>
        /// CaptionDesc2 component viewModel, can set title and file name
        /// </summary>
        public CaptionDesc4ViewMode Caption4VM { get => caption4ViewModel; }

        /// <summary>
        /// Result label TextBlock margin,defult value is (222, 5, 0, 15)
        /// </summary>
        public Thickness ResultLableMargin { get => resultLableMargin; set { resultLableMargin = value; OnPropertyChanged("ResultLableMargin"); } }

        /// <summary>
        /// Result label foreground,defult value is "#27AE60"
        /// </summary>
        public SolidColorBrush ResultLabelForegrd { get => resultLabelForegrd; set { resultLabelForegrd = value; OnPropertyChanged("ResultLabelForegrd"); } }

        /// <summary>
        /// Result label, defult value is 'Successfully shared'.
        /// </summary>
        public string ResultLabel { get => resultLabel; set { resultLabel = value; OnPropertyChanged("ResultLabel"); } }

        /// <summary>
        /// Adhoc and Classified(Central policy) Rights UI ViewModel
        /// </summary>
        public AdhocAndClassifiedRightsViewModel AdhocAndClassifiedRightsVM { get => adhocAndClassifiedRightsVM; }

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
        /// Message displayed in texbox， defult value is empty.
        /// </summary>
        public string Message { get => message; set { message = value; OnPropertyChanged("Message"); } }


        public event PropertyChangedEventHandler PropertyChanged;
        protected void OnPropertyChanged(string propertyName)
        {
            PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(propertyName));
        }
    }

    /// <summary>
    /// Interaction logic for FileRightsShareResultPage.xaml
    /// </summary>
    public partial class FileRightsShareResultPage : Page
    {
        private FileRightsShareResultViewMode viewMode;
        public FileRightsShareResultPage()
        {
            this.Resources.MergedDictionaries.Add(SharedDictionaryManager.StringResource);
            this.Resources.MergedDictionaries.Add(SharedDictionaryManager.UnifiedBtnStyle);

            InitializeComponent();

            this.DataContext = viewMode = new FileRightsShareResultViewMode(this);
        }

        /// <summary>
        /// ViewModel for FileRightsShareResultPage.xaml 
        /// </summary>
        public FileRightsShareResultViewMode ViewMode { get => viewMode; set => this.DataContext = viewMode = value; }
}
}
