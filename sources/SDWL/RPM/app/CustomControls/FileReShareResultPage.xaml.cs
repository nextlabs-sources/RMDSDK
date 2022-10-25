using CustomControls.common.sharedResource;
using CustomControls.components;
using CustomControls.components.RightsDisplay.model;
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
    /// <summary>
    /// FileReShareResultPage.xaml  DataCommands
    /// </summary>
    public class FReSResult_DataCommands
    {
        private static RoutedCommand close;
        static FReSResult_DataCommands()
        {
            InputGestureCollection input = new InputGestureCollection();
            input.Add(new KeyGesture(Key.Enter));
            close = new RoutedCommand(
              "Close", typeof(FReSResult_DataCommands), input);
        }

        /// <summary>
        /// FileShareResultPage.xaml close button command
        /// </summary>
        public static RoutedCommand Close
        {
            get { return close; }
        }
    }

    /// <summary>
    /// ViewModel for FileReShareResultPage.xaml
    /// </summary>
    public class FileReShareResultViewModel : INotifyPropertyChanged
    {
        private FileReShareResultPage host;
        // CaptionDesc2.xaml
        private CaptionDesc2ViewMode captionDesc2ViewMode;

        private string repoName;
        private AdhocAndClassifiedRightsViewModel adhocAndClassifiedRights;

        public FileReShareResultViewModel(FileReShareResultPage host)
        {
            this.host = host;
            this.host.captionDesc2.ViewModel = captionDesc2ViewMode = new CaptionDesc2ViewMode()
            {
                Title = this.host.TryFindResource("FileReShareResult_Title").ToString()
            };
            
            adhocAndClassifiedRights = this.host.adhocAndclassifiedRights.ViewModel;
        }

        /// <summary>
        /// CaptionDesc2.xaml viewModel
        /// </summary>
        public CaptionDesc2ViewMode CaptionDesc2ViewMode { get => captionDesc2ViewMode; }

        /// <summary>
        /// Display repository name,defult value is null
        /// </summary>
        public string RepoName { get => repoName; set { repoName = value; OnPropertyChanged("RepoName"); } }

        /// <summary>
        /// Adhoc and Classified(Central policy) Rights UI ViewModel
        /// </summary>
        public AdhocAndClassifiedRightsViewModel AdhocAndClassifiedRightsVM { get => adhocAndClassifiedRights; }


        public event PropertyChangedEventHandler PropertyChanged;
        protected void OnPropertyChanged(string propertyName)
        {
            PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(propertyName));
        }
    }
    /// <summary>
    /// Interaction logic for FileReShareResultPage.xaml
    /// </summary>
    public partial class FileReShareResultPage : Page
    {
        private FileReShareResultViewModel viewModel;
        public FileReShareResultPage()
        {
            this.Resources.MergedDictionaries.Add(SharedDictionaryManager.StringResource);
            this.Resources.MergedDictionaries.Add(SharedDictionaryManager.UnifiedBtnStyle);
            InitializeComponent();
            this.DataContext = viewModel = new FileReShareResultViewModel(this);
        }

        /// <summary>
        /// ViewModel for FileReShareResultPage.xaml
        /// </summary>
        public FileReShareResultViewModel ViewModel { get => viewModel; set => this.DataContext = viewModel = value; }
    }
}
