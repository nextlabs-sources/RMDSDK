using CustomControls.common.sharedResource;
using CustomControls.components.RightsDisplay.model;
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

namespace CustomControls.officeUserControl
{
    /// <summary>
    /// FileInfoEx.xaml  DataCommands
    /// </summary>
    public class FInfoEx_DataCommands
    {
        private static RoutedCommand modify;
        private static RoutedCommand cancel;
        static FInfoEx_DataCommands()
        {
            InputGestureCollection input = new InputGestureCollection();
            input.Add(new KeyGesture(Key.Enter));
            modify = new RoutedCommand(
              "Modify", typeof(FInfoEx_DataCommands), input);

            input = new InputGestureCollection();
            input.Add(new KeyGesture(Key.Escape));
            cancel = new RoutedCommand(
              "Cancel", typeof(FInfoEx_DataCommands), input);
        }
        /// <summary>
        ///  FileInfoEx.xaml modify button command
        /// </summary>
        public static RoutedCommand Modify
        {
            get { return modify; }
        }
        /// <summary>
        /// FileInfoEx.xaml cancel button command
        /// </summary>
        public static RoutedCommand Cancel
        {
            get { return cancel; }
        }
    }
    /// <summary>
    /// ViewModel for FileInfoEx.xaml
    /// </summary>
    public class FileInfoExViewModel : INotifyPropertyChanged
    {
        private FileInfoEx host;

        private BitmapImage icon;
        private string filePath = "";
        // RightsStackPanle.xaml
        private RightsStPanViewModel rightsDisplayViewModel;

        // CentralTagView.xaml
        private Visibility centralSpVisible = Visibility.Visible;
        private string centralText;
        private double tagViewMaxWidth = 200;
        private Dictionary<string, List<string>> centralTag = new Dictionary<string, List<string>>();

        private Visibility accessDenyVisibility = Visibility.Collapsed;
        private string accessDenyText;

        private Visibility modifyBtnVisible = Visibility.Collapsed;

        public FileInfoExViewModel(FileInfoEx host)
        {
            this.host = host;
            this.host.RightsSp.ViewModel = rightsDisplayViewModel = new RightsStPanViewModel(this.host.RightsSp);

            centralText = this.host.TryFindResource("Rights_Company_Text").ToString();
            accessDenyText = this.host.TryFindResource("ClassifiedRight_No_Permission").ToString();
        }

        /// <summary>
        /// Binding file icon
        /// </summary>
        public BitmapImage Icon { get => icon; set { icon = value; OnPropertyChanged("Icon"); } }

        /// <summary>
        /// Display file path, defult value is ""
        /// </summary>
        public string FilePath { get => filePath; set { filePath = value; OnPropertyChanged("FilePath"); } }

        /// <summary>
        /// Rights stackPanel UI(contain rights, warterMark, expiry date) ViewModel
        /// </summary>
        public RightsStPanViewModel RightsDisplayViewModel { get => rightsDisplayViewModel; }

        /// <summary>
        /// CentralPolicy StackPanel visibe,defult value is Visible
        /// </summary>
        public Visibility CentralSpVisible { get => centralSpVisible; set { centralSpVisible = value; OnPropertyChanged("CentralSpVisible"); } }

        /// <summary>
        /// CentralPolicy describe text, defult value is "Company-defined rights ...."
        /// </summary>
        public string CentralText { get => centralText; set { centralText = value; OnPropertyChanged("CentralText"); } }

        /// <summary>
        /// The max width of TextBlock to display CentralPolicy tags, should set before CentralTag property. defult value is 500.
        /// </summary>
        public double TagViewMaxWidth { get => tagViewMaxWidth; set { tagViewMaxWidth = value; OnPropertyChanged("TagViewMaxWidth"); } }

        /// <summary>
        /// CentralPolicy tags
        /// </summary>
        public Dictionary<string, List<string>> CentralTag { get => centralTag; set { centralTag = value; OnPropertyChanged("CentralTag"); } }

        /// <summary>
        /// AccessDeniedView UI visibility,defult vallue is Collapsed. if this value is Visibility.Visible, the RightsStackPanle UI will Collapsed.
        /// </summary>
        public Visibility AccessDenyVisibility { get => accessDenyVisibility; set { accessDenyVisibility = value; OnPropertyChanged("AccessDenyVisibility"); } }
        
        /// <summary>
        /// AccessDeniedView UI display text, defult value is "You do not have any right on this file."
        /// </summary>
        public string AccessDenyText { get => accessDenyText; set { accessDenyText = value; OnPropertyChanged("AccessDenyText"); } }

        /// <summary>
        /// Modify rights button visible,defult value is Collapsed
        /// </summary>
        public Visibility ModifyBtnVisible { get => modifyBtnVisible; set { modifyBtnVisible = value; OnPropertyChanged("ModifyBtnVisible"); } }

        

        public event PropertyChangedEventHandler PropertyChanged;
        protected void OnPropertyChanged(string propertyName)
        {
            PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(propertyName));
        }
    }

    /// <summary>
    /// Interaction logic for FileInfoEx.xaml
    /// </summary>
    public partial class FileInfoEx : UserControl
    {
        private FileInfoExViewModel viewModel;
        public FileInfoEx()
        {
            this.Resources.MergedDictionaries.Add(SharedDictionaryManager.StringResource);

            InitializeComponent();
            this.DataContext = viewModel = new FileInfoExViewModel(this);
        }

        /// <summary>
        /// ViewModel for FileInfoEx.xaml 
        /// </summary>
        public FileInfoExViewModel ViewModel { get => viewModel; set { this.DataContext = viewModel = value; } }
    }
}
