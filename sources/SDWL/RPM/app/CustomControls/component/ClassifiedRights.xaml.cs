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
    #region Convert
    public class RightsStPanViewVisibilityConvert : IValueConverter
    {
        public object Convert(object value, Type targetType, object parameter, CultureInfo culture)
        {
            Visibility visible = (Visibility)value;
            if (visible == Visibility.Visible)
            {
                return Visibility.Collapsed;
            }
            return Visibility.Visible;
        }

        public object ConvertBack(object value, Type targetType, object parameter, CultureInfo culture)
        {
            throw new NotImplementedException();
        }
    }
    #endregion

    /// <summary>
    /// ViewModel for ClassifiedRights.xaml
    /// </summary>
    public class ClassifiedRightsViewModel : INotifyPropertyChanged
    {
        private ClassifiedRights host;
        // RightsStackPanle.xaml
        private RightsStPanViewModel rightsDisplayViewModel;
        // CentralTagView.xaml
        private Dictionary<string, List<string>> centralTag=new Dictionary<string, List<string>>();
        private double tagViewMaxWidth = 500;
        private Visibility rightsDisplayVisibility = Visibility.Visible;
        private Visibility accessDenyVisibility = Visibility.Collapsed;
        private string accessDenyText;

        public ClassifiedRightsViewModel(ClassifiedRights host)
        {
            this.host = host;
            rightsDisplayViewModel = this.host.RightsSp.ViewModel;
            accessDenyText = this.host.TryFindResource("ClassifiedRight_No_Permission").ToString();
        }

        /// <summary>
        /// Rights stackPanel UI(contain rights, warterMark, expiry date) ViewModel
        /// </summary>
        public RightsStPanViewModel RightsDisplayVM { get => rightsDisplayViewModel; }
        /// <summary>
        /// CentralPolicy tags
        /// </summary>
        public Dictionary<string, List<string>> CentralTag { get => centralTag; set { centralTag = value; OnPropertyChanged("CentralTag"); } }
        /// <summary>
        /// The max width of TextBlock to display CentralPolicy tags, should set before CentralTag property. defult value is 500.
        /// </summary>
        public double TagViewMaxWidth { get => tagViewMaxWidth; set { tagViewMaxWidth = value; OnPropertyChanged("TagViewMaxWidth"); } }
        /// <summary>
        /// Rights list and accessDeny UI visibility,defult value is visible.
        /// </summary>
        public Visibility RightsDisplayVisibility { get => rightsDisplayVisibility; set { rightsDisplayVisibility = value; OnPropertyChanged("RightsDisplay"); } }
        /// <summary>
        /// AccessDeniedView UI visibility,defult vallue is Collapsed. if this value is Visibility.Visible, the RightsStackPanle UI will Collapsed.
        /// </summary>
        public Visibility AccessDenyVisibility { get => accessDenyVisibility; set { accessDenyVisibility = value; OnPropertyChanged("AccessDenyVisibility"); } }
        /// <summary>
        /// AccessDeniedView UI display text, defult value is "You do not have any right on this file."
        /// </summary>
        public string AccessDenyText { get => accessDenyText; set { accessDenyText = value; OnPropertyChanged("AccessDenyText"); } }


        public event PropertyChangedEventHandler PropertyChanged;
        protected void OnPropertyChanged(string propertyName)
        {
            PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(propertyName));
        }
    }

    /// <summary>
    /// Interaction logic for ClassifiedRights.xaml
    /// </summary>
    public partial class ClassifiedRights : UserControl
    {
        private ClassifiedRightsViewModel viewModel;

        public ClassifiedRights()
        {
            this.Resources.MergedDictionaries.Add(SharedDictionaryManager.StringResource);
            InitializeComponent();
            this.DataContext = viewModel = new ClassifiedRightsViewModel(this);
        }

        /// <summary>
        ///  ViewModel for ClassifiedRights.xaml
        /// </summary>
        public ClassifiedRightsViewModel ViewModel { get => viewModel; set =>this.DataContext = viewModel = value; }

    }
}
