using CustomControls.common.sharedResource;
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

namespace CustomControls.components
{
    public enum FileType
    {
        Adhoc,
        CentralPolicy
    }

    #region Convert
    public class FileTypeToAdVisibilityConvert : IValueConverter
    {
        public object Convert(object value, Type targetType, object parameter, CultureInfo culture)
        {
            FileType type = (FileType)value;
            if (type == FileType.Adhoc)
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
    public class FileTypeToCpVisibilityConvert : IValueConverter
    {
        public object Convert(object value, Type targetType, object parameter, CultureInfo culture)
        {
            FileType type = (FileType)value;
            if (type == FileType.CentralPolicy)
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
    /// ViewModel for FileReShareResultPage.xaml
    /// </summary>
    public class AdhocAndClassifiedRightsViewModel : INotifyPropertyChanged
    {
        private AdhocAndClassifiedRights host;
       
        private FileType fileType = FileType.CentralPolicy;
        
        // RightsStackPanle.xaml
        private RightsStPanViewModel rightsDisplayViewModel;
        // ClassifiedRightsView.xaml
        private ClassifiedRightsViewModel classifiedRightsViewModel;

        public AdhocAndClassifiedRightsViewModel(AdhocAndClassifiedRights host)
        {
            this.host = host;
            rightsDisplayViewModel = this.host.rightSP.ViewModel;
            classifiedRightsViewModel = this.host.classifiedRights.ViewModel;
        }

        /// <summary>
        /// It's control adhocRights and centralTag UI, defult value is FileType.CentralPolicy and display centralTag UI
        /// </summary>
        public FileType FileType { get => fileType; set { fileType = value; OnPropertyChanged("FileType"); } }

        /// <summary>
        /// Adhoc rights UI(contain rights, warterMark, expiry date) ViewModel
        /// </summary>
        public RightsStPanViewModel AdhocRightsVM { get => rightsDisplayViewModel; }

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
    /// Interaction logic for AdhocAndClassifiedRights.xaml
    /// </summary>
    public partial class AdhocAndClassifiedRights : UserControl
    {
        private AdhocAndClassifiedRightsViewModel viewModel;
        public AdhocAndClassifiedRights()
        {
            this.Resources.MergedDictionaries.Add(SharedDictionaryManager.StringResource);
            InitializeComponent();
            this.DataContext = viewModel = new AdhocAndClassifiedRightsViewModel(this);
        }
        /// <summary>
        /// AdhocAndClassifiedRights.xaml ViewModel
        /// </summary>
        public AdhocAndClassifiedRightsViewModel ViewModel { get => viewModel; set => this.DataContext = viewModel = value; }
    }
}
