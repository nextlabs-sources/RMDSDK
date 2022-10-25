using CustomControls.common.sharedResource;
using CustomControls.componentPages.Preference;
using CustomControls.pages.Preference;
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
    /// Use for PreferenceUserControl internal binding
    /// </summary>
    public enum PreferenceType
    {
        System,
        Document,
        RPM
    }

    #region Convert
    public class ProtectTypeToSysBtnBackgroundConvert : IValueConverter
    {
        public object Convert(object value, Type targetType, object parameter, CultureInfo culture)
        {
            PreferenceType type = (PreferenceType)value;
            if (type == PreferenceType.System)
            {
                return new SolidColorBrush(Color.FromRgb(211, 206, 206));
            }
            return new SolidColorBrush(Color.FromRgb(255, 255, 255));
        }

        public object ConvertBack(object value, Type targetType, object parameter, CultureInfo culture)
        {
            throw new NotImplementedException();
        }
    }
    public class ProtectTypeToDcmBtnBackgroundConvert : IValueConverter
    {
        public object Convert(object value, Type targetType, object parameter, CultureInfo culture)
        {
            PreferenceType type = (PreferenceType)value;
            if (type == PreferenceType.Document)
            {
                return new SolidColorBrush(Color.FromRgb(211, 206, 206));
            }
            return new SolidColorBrush(Color.FromRgb(255, 255, 255));
        }

        public object ConvertBack(object value, Type targetType, object parameter, CultureInfo culture)
        {
            throw new NotImplementedException();
        }
    }
    public class ProtectTypeToRpmBtnBackgroundConvert : IValueConverter
    {
        public object Convert(object value, Type targetType, object parameter, CultureInfo culture)
        {
            PreferenceType type = (PreferenceType)value;
            if (type == PreferenceType.RPM)
            {
                return new SolidColorBrush(Color.FromRgb(211, 206, 206));
            }
            return new SolidColorBrush(Color.FromRgb(255, 255, 255));
        }

        public object ConvertBack(object value, Type targetType, object parameter, CultureInfo culture)
        {
            throw new NotImplementedException();
        }
    }

    public class ProtectTypeToSysVisibilityConvert : IValueConverter
    {
        public object Convert(object value, Type targetType, object parameter, CultureInfo culture)
        {
            PreferenceType type = (PreferenceType)value;
            if (type == PreferenceType.System)
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
    public class ProtectTypeToDcmVisibilityConvert : IValueConverter
    {
        public object Convert(object value, Type targetType, object parameter, CultureInfo culture)
        {
            PreferenceType type = (PreferenceType)value;
            if (type == PreferenceType.Document)
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
    public class ProtectTypeToRpmVisibilityConvert : IValueConverter
    {
        public object Convert(object value, Type targetType, object parameter, CultureInfo culture)
        {
            PreferenceType type = (PreferenceType)value;
            if (type == PreferenceType.RPM)
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
    /// ViewModel for PreferenceUserControl.xaml
    /// </summary>
    public class PreferenceViewModel : INotifyPropertyChanged
    {
        private PreferenceUserControl host;
        private SystemPViewModel systemPViewModel;
        private DocumentPViewModel documentPViewModel;
        private RpmFolderPViewModel rpmFolderPViewModel;
        private PreferenceType type = PreferenceType.System;

        public PreferenceViewModel(PreferenceUserControl userControl)
        {
            host = userControl;
            systemPViewModel = host.sysP.ViewModel;
            documentPViewModel = host.dcmP.ViewModel;
            rpmFolderPViewModel = host.rpmP.ViewModel;
        }

        /// <summary>
        /// SystemPage ViewModel
        /// </summary>
        public SystemPViewModel SystemPViewModel { get => systemPViewModel; }

        /// <summary>
        /// DocumentPage ViewModel
        /// </summary>
        public DocumentPViewModel DocumentPViewModel { get => documentPViewModel; }

        /// <summary>
        /// RpmFolderPage ViewModel
        /// </summary>
        public RpmFolderPViewModel RpmFolderPViewModel { get => rpmFolderPViewModel; }

        /// <summary>
        /// internal binding type
        /// </summary>
        public PreferenceType Type { get => type; set { type = value; OnPropertyChanged("Type"); } }


        public event PropertyChangedEventHandler PropertyChanged;
        protected void OnPropertyChanged(string propertyName)
        {
            PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(propertyName));
        }
    }
    /// <summary>
    /// Interaction logic for PreferenceUserControl.xaml
    /// </summary>
    public partial class PreferenceUserControl : UserControl
    {
        private PreferenceViewModel viewModel;
        public PreferenceUserControl()
        {
            this.Resources.MergedDictionaries.Add(SharedDictionaryManager.StringResource);
            InitializeComponent();
            this.DataContext = viewModel = new PreferenceViewModel(this);
        }

        public PreferenceViewModel ViewModel { get => viewModel; set { this.DataContext = viewModel = value; } }

        private void Btn_Click(object sender, RoutedEventArgs e)
        {
            Button button = e.Source as Button;
            if (button.Name == "btnSystem")
            {
                viewModel.Type = PreferenceType.System;
            }
            else if (button.Name == "btnDocument")
            {
                viewModel.Type = PreferenceType.Document;
            }
            else if (button.Name == "btnRpm")
            {
                viewModel.Type = PreferenceType.RPM;
            }
        }
    }
}
