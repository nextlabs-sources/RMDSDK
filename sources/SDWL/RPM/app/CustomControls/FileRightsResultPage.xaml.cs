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
    /// FileRightsResultPage.xaml  DataCommands
    /// </summary>
    public class FRResult_DataCommands
    {
        private static RoutedCommand close;
        static FRResult_DataCommands()
        {
            InputGestureCollection input = new InputGestureCollection();
            input.Add(new KeyGesture(Key.Enter));
            close = new RoutedCommand(
              "Close", typeof(FRResult_DataCommands), input);
        }

        /// <summary>
        /// FileRightsResultPage.xaml close button command
        /// </summary>
        public static RoutedCommand Close
        {
            get { return close; }
        }
    }

    /// <summary>
    /// ViewModel for FileRightsResultPage.xaml
    /// </summary>
    public class FileRightsResultViewMode : INotifyPropertyChanged
    {
        private FileRightsResultPage host;
        // CaptionDesc3.xaml ViewModel
        private CaptionDesc3ViewMode caption3ViewModel;
        // AdhocAndClassifiedRights.xaml
        private AdhocAndClassifiedRightsViewModel adhocAndClassifiedRightsVM;

        public FileRightsResultViewMode(FileRightsResultPage page)
        {
            host = page;
            caption3ViewModel = host.captionDesc3.ViewModel;
            adhocAndClassifiedRightsVM = host.adhocAndclassifiedRights.ViewModel;
        }

        /// <summary>
        /// CaptionDesc component viewModel, can set title, fileName, promptText, desitination
        /// </summary>
        public CaptionDesc3ViewMode Caption3VM { get => caption3ViewModel; }

        /// <summary>
        /// Adhoc and Classified(Central policy) Rights UI ViewModel
        /// </summary>
        public AdhocAndClassifiedRightsViewModel AdhocAndClassifiedRightsVM { get => adhocAndClassifiedRightsVM; }


        public event PropertyChangedEventHandler PropertyChanged;
        protected void OnPropertyChanged(string propertyName)
        {
            PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(propertyName));
        }
    }

    /// <summary>
    /// Interaction logic for FileRightsResultPage.xaml
    /// </summary>
    public partial class FileRightsResultPage : Page
    {
        private FileRightsResultViewMode viewMode;
        public FileRightsResultPage()
        {
            this.Resources.MergedDictionaries.Add(SharedDictionaryManager.StringResource);
            this.Resources.MergedDictionaries.Add(SharedDictionaryManager.UnifiedBtnStyle);
            InitializeComponent();
            this.DataContext = viewMode = new FileRightsResultViewMode(this);
        }

        /// <summary>
        /// ViewModel for FileRightsResultPage.xaml
        /// </summary>
        public FileRightsResultViewMode ViewMode { get => viewMode; set => this.DataContext = viewMode = value; }
    }
}
