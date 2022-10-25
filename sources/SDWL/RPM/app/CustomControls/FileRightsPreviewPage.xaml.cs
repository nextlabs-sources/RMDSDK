using CustomControls.common.sharedResource;
using CustomControls.components;
using System;
using System.Collections.Generic;
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
    /// FileRightsPreviewPage.xaml  DataCommands
    /// </summary>
    public class FRPreview_DataCommands
    {
        private static RoutedCommand positive;
        private static RoutedCommand back;
        private static RoutedCommand cancel;
        static FRPreview_DataCommands()
        {
            // if the IsEnable property of positive button have bindinged,
            // best not binding input gesture in Command. 
            // otherwise user can trigger command by input gesture when button isEnable property is false.
            positive = new RoutedCommand(
              "Positive", typeof(FRPreview_DataCommands));

            back = new RoutedCommand(
              "Back", typeof(FRPreview_DataCommands));

            InputGestureCollection input = new InputGestureCollection();
            input.Add(new KeyGesture(Key.Escape));
            cancel = new RoutedCommand(
              "Cancel", typeof(FRPreview_DataCommands), input);
        }
        /// <summary>
        /// FileRightsPreviewPage.xaml positive button command
        /// </summary>
        public static RoutedCommand Positive
        {
            get { return positive; }
        }
        /// <summary>
        ///  FileRightsPreviewPage.xaml back button command
        /// </summary>
        public static RoutedCommand Back
        {
            get { return back; }
        }
        /// <summary>
        /// FileRightsPreviewPage.xaml cancel button command
        /// </summary>
        public static RoutedCommand Cancel
        {
            get { return cancel; }
        }
    }

    /// <summary>
    /// ViewModel for FileRightsPreviewPage.xaml
    /// </summary>
    public class FileRightsPreviewViewMode : INotifyPropertyChanged
    {
        private FileRightsPreviewPage host;
        // CaptionDesc.xaml ViewModel
        private CaptionDescViewMode captionViewModel;
        private Visibility captionDescVisible = Visibility.Visible;
        // CaptionDesc4.xaml ViewModel
        private CaptionDesc4ViewMode caption4ViewModel;

        private Thickness savePathStpMargin = new Thickness(222, 5, 0, 0);
        private Visibility savePathStpVisibility = Visibility.Collapsed;
        private string savePathDesc = "";
        private string savePath = "";
        // AdhocAndClassifiedRights.xaml
        private AdhocAndClassifiedRightsViewModel adhocAndClassifiedRightsVM;

        private string positiveBtnContent = "";
        private Visibility backBtnVisibility = Visibility.Visible;

        public FileRightsPreviewViewMode(FileRightsPreviewPage page)
        {
            host = page;
            savePathDesc = host.TryFindResource("FileRightsSelect_SavePath_Lable").ToString();
            captionViewModel = host.captionDesc.ViewModel;
            caption4ViewModel = host.captionCom4.ViewModel;
            adhocAndClassifiedRightsVM = host.adhocAndclassifiedRights.ViewModel;
        }

        /// <summary>
        /// CaptionDesc component viewModel, can set title, describle, file name, original file tags...
        /// </summary>
        public CaptionDescViewMode CaptionVM { get => captionViewModel; }

        /// <summary>
        /// CaptionDesc component Visibility, if Visibility is not Visible will display CaptionDesc4 component. defult value is Visible
        /// </summary>
        public Visibility CaptionDescVisible { get => captionDescVisible; set { captionDescVisible = value; OnPropertyChanged("CaptionDescVisible"); } }

        /// <summary>
        /// CaptionDesc4 component viewModel, can set title, describle, file name.
        /// </summary>
        public CaptionDesc4ViewMode Caption4VM { get => caption4ViewModel; }

        /// <summary>
        /// Save path stackPanel margin,defult value is (222, 5, 0, 0)
        /// </summary>
        public Thickness SavePathStpMargin { get => savePathStpMargin; set { savePathStpMargin = value; OnPropertyChanged("SavePathStpMargin"); } }

        /// <summary>
        /// Control Save path StackPanel visibility, defult value is Visibility.Collapsed
        /// </summary>
        public Visibility SavePathStpVisibility { get => savePathStpVisibility; set { savePathStpVisibility = value; OnPropertyChanged("SavePathStpVisibility"); } }

        /// <summary>
        /// Save Path describe, defult value is "The protected file will be saved to"
        /// </summary>
        public string SavePathDesc { get => savePathDesc; set { savePathDesc = value; OnPropertyChanged("SavePathDesc"); } }

        /// <summary>
        /// Display save path, defult value is "", it's part of SavePath StackPanel
        /// </summary>
        public string SavePath { get => savePath; set { savePath = value; OnPropertyChanged("SavePath"); } }

        /// <summary>
        /// Classified(Central policy) rights UI(contain tag, rights, warterMark, expiry date) ViewModel
        /// </summary>
        public AdhocAndClassifiedRightsViewModel AdhocAndClassifiedRightsVM { get => adhocAndClassifiedRightsVM; }

        /// <summary>
        /// Positive button content, defult value is ""
        /// </summary>
        public string PositiveBtnContent { get => positiveBtnContent; set { positiveBtnContent = value; OnPropertyChanged("PositiveBtnContent"); } }

        /// <summary>
        /// Back button visibility, defult value is Visible
        /// </summary>
        public Visibility BackBtnVisibility { get => backBtnVisibility; set { backBtnVisibility = value; OnPropertyChanged("BackBtnVisibility"); } }


        public event PropertyChangedEventHandler PropertyChanged;
        protected void OnPropertyChanged(string propertyName)
        {
            PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(propertyName));
        }
    }

    /// <summary>
    /// Interaction logic for FileRightsPreviewPage.xaml
    /// </summary>
    public partial class FileRightsPreviewPage : Page
    {
        private FileRightsPreviewViewMode viewMode;
        public FileRightsPreviewPage()
        {
            this.Resources.MergedDictionaries.Add(SharedDictionaryManager.StringResource);
            this.Resources.MergedDictionaries.Add(SharedDictionaryManager.UnifiedBtnStyle);

            InitializeComponent();

            this.DataContext = viewMode = new FileRightsPreviewViewMode(this);
        }
        /// <summary>
        /// ViewModel for FileRightsPreviewPage.xaml 
        /// </summary>
        public FileRightsPreviewViewMode ViewMode { get => viewMode; set => this.DataContext = viewMode = value; }
    }
}
