using CustomControls.common.sharedResource;
using CustomControls.components.CentralPolicy.model;
using CustomControls.components.DigitalRights.model;
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

namespace CustomControls.officeUserControl
{
    public enum ProtectType
    {
        Adhoc,
        CentralPolicy
    }

    #region Convert
    public class ProtectTypeToBoolenConverter : IValueConverter
    {
        //ProtectType to radiobutton
        public object Convert(object value, Type targetType, object parameter, CultureInfo culture)
        {
            ProtectType type = (ProtectType)value;
            return type == (ProtectType)int.Parse(parameter.ToString());
        }
        //radiobutton to ProtectType
        public object ConvertBack(object value, Type targetType, object parameter, CultureInfo culture)
        {
            bool isChecked = (bool)value;
            if (!isChecked)
            {
                return null;
            }
            return (ProtectType)int.Parse(parameter.ToString());
        }
    }

    public class ProtectTypeToAdVisibilityConvert : IValueConverter
    {
        public object Convert(object value, Type targetType, object parameter, CultureInfo culture)
        {
            ProtectType type = (ProtectType)value;
            if (type == ProtectType.Adhoc)
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
    public class ProtectTypeToCpVisibilityConvert : IValueConverter
    {
        public object Convert(object value, Type targetType, object parameter, CultureInfo culture)
        {
            ProtectType type = (ProtectType)value;
            if (type == ProtectType.CentralPolicy)
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
    /// FileRightsSelectEx.xaml  DataCommands
    /// </summary>
    public class FRSEx_DataCommands
    {
        private static RoutedCommand skip;
        private static RoutedCommand positive;
        private static RoutedCommand cancel;
        static FRSEx_DataCommands()
        {
            skip = new RoutedCommand(
              "Skip", typeof(FRSEx_DataCommands));

            // if the IsEnable property of positive button have bindinged,
            // best not binding input gesture in Command. 
            // otherwise user can trigger command by input gesture when button isEnable property is false.
            positive = new RoutedCommand(
              "Positive", typeof(FRSEx_DataCommands));

            InputGestureCollection input = new InputGestureCollection();
            input.Add(new KeyGesture(Key.Escape));
            cancel = new RoutedCommand(
              "Cancel", typeof(FRSEx_DataCommands), input);
        }
        /// <summary>
        /// FileRightsSelectEx.xaml skip button command
        /// </summary>
        public static RoutedCommand Skip
        {
            get { return skip; }
        }
        /// <summary>
        ///  FileRightsSelectEx.xaml positive button command
        /// </summary>
        public static RoutedCommand Positive
        {
            get { return positive; }
        }
        /// <summary>
        /// FileRightsSelectEx.xaml cancel button command
        /// </summary>
        public static RoutedCommand Cancel
        {
            get { return cancel; }
        }
    }

    /// <summary>
    /// ViewModel for FileRightsSelectEx.xaml
    /// </summary>
    public class FileRightsSelectExDataMode : INotifyPropertyChanged
    {
        private FileRightsSelectEx host;
        private string infoTitle;
        private BitmapImage icon;
        private string filePath = "";
        private Visibility descAndRadioVisible = Visibility.Visible;
        private bool adRdIsEnable = true;
        private bool cplRdIsEnable = true;
        private Visibility infoTextVisible = Visibility.Collapsed;
        private Visibility skipBtnVisible = Visibility.Collapsed;
        private string positiveBtnContent;
        private bool positiveBtnIsEnable = true;
        private string cancelBtnContent;
        private ProtectType protectType = ProtectType.CentralPolicy;
        // adhoc page dataModel
        private string adhocDescribeText;
        private TextAlignment adhocDescribeTextAlignment = System.Windows.TextAlignment.Center;
        private DigitalRightsViewModel adhocPage_ViewModel;
        // central Policy page dataModel
        private string cpDescribeText;
        private string cpWarnDesText;
        private Visibility cpWarnDesVisible = Visibility.Collapsed;
        private TextAlignment cpTextAlignment = TextAlignment.Center;
        private Classification[] cpClassifications = new Classification[0];
        private Dictionary<string, List<string>> cpAddInheritedClassification = new Dictionary<string, List<string>>();
        // ProgressBar
        private Visibility progressVisible = Visibility.Collapsed;

        public FileRightsSelectExDataMode(FileRightsSelectEx userControl)
        {
            host = userControl;
            adhocDescribeText = this.host.TryFindResource("Rights_User_Text").ToString();
            host.page_Adhoc.ViewModel = adhocPage_ViewModel = new DigitalRightsViewModel(host.page_Adhoc);

            infoTitle = this.host.TryFindResource("CapDesc_Describe_Adhoc").ToString();
            positiveBtnContent = this.host.TryFindResource("Windows_Btn_OK").ToString();
            cancelBtnContent = this.host.TryFindResource("Windows_Btn_Cancel").ToString();
            cpDescribeText = this.host.TryFindResource("Rights_Company_Text").ToString();
            cpWarnDesText = this.host.TryFindResource("Rights_Warning_Text").ToString();
        }

        /// <summary>
        /// if centralPolicy page selected classifications changed, will trigger this event handler
        /// This eventHandler is specially added for office add-ins winform, Other users can use this to process or directly monitor central
        /// policy page SelectClassificationChanged routed event in xaml.
        /// </summary>
        public event RoutedPropertyChangedEventHandler<SelectClassificationEventArgs> OnClassificationChanged;

        /// <summary>
        /// Display top title info 
        /// </summary>
        public string InfoTitle { get => infoTitle; set { infoTitle = value; OnPropertyChanged("InfoTitle"); } }

        /// <summary>
        /// Binding Icon
        /// </summary>
        public BitmapImage Icon { get => icon; set { icon = value; OnPropertyChanged("Icon"); } }

        /// <summary>
        /// Display file path, defult value is ""
        /// </summary>
        public string FilePath { get => filePath; set { filePath = value; OnPropertyChanged("FilePath"); } }
       
        /// <summary>
        /// Control description textBlock and raidoButton visibility. defult value is Visibility.Visible
        /// </summary>
        public Visibility DescAndRadioVisible { get => descAndRadioVisible; set { descAndRadioVisible = value; OnPropertyChanged("DescAndRadioVisible"); } }

        /// <summary>
        /// Control Adhoc RadioButton IsEnable, defult value is true
        /// </summary>
        public bool AdhocRadioIsEnable { get => adRdIsEnable; set { adRdIsEnable = value; OnPropertyChanged("AdhocRadioIsEnable"); } }

        /// <summary>
        ///  Control Central RadioButton IsEnable, defult value is true
        /// </summary>
        public bool CentralRadioIsEnable { get => cplRdIsEnable; set { cplRdIsEnable = value; OnPropertyChanged("CentralRadioIsEnable"); } }

        /// <summary>
        /// Button prompt message bar visibility,defult value Collapsed
        /// </summary>
        public Visibility InfoTextVisible { get => infoTextVisible; set { infoTextVisible = value; OnPropertyChanged("InfoText"); } }

        /// <summary>
        /// Skip button visibility, defult value Collapsed
        /// </summary>
        public Visibility SkipBtnVisible { get => skipBtnVisible; set { skipBtnVisible = value; OnPropertyChanged("SkipBtnVisible"); } }

        /// <summary>
        /// Positive button content, defult value is "OK"
        /// </summary>
        public string PositiveBtnContent { get => positiveBtnContent; set { positiveBtnContent = value; OnPropertyChanged("PositiveBtnContent"); } }

        /// <summary>
        /// Positive button IsEnable, defult value is true
        /// </summary>
        public bool PositiveBtnIsEnable { get => positiveBtnIsEnable; set { positiveBtnIsEnable = value; OnPropertyChanged("PositiveBtnIsEnable"); } }

        /// <summary>
        /// Cancel button content, defult value is "Cancel"
        /// </summary>
        public string CancelBtnContent { get => cancelBtnContent; set => cancelBtnContent = value; }

        /// <summary>
        /// Protect file type, corresponding adhoc and central radioButton checked. defult value is CentralPolicy.
        /// </summary>
        public ProtectType ProtectType { get => protectType; set { protectType = value; OnPropertyChanged("ProtectType"); } }

        /// <summary>
        /// Adhoc Describe title, defult value is 'User-defined rights are pre-defined permissions that you can apply to your documents.'
        /// </summary>
        public string AdhocDesText { get => adhocDescribeText; set { adhocDescribeText = value; OnPropertyChanged("AdhocDesText"); } }

        /// <summary>
        /// Adhoc Describe title TextAlignment, defult value is 'System.Windows.TextAlignment.Center'
        /// </summary>
        public TextAlignment AdhocDesTextAlign { get => adhocDescribeTextAlignment; set { adhocDescribeTextAlignment = value; OnPropertyChanged("AdhocDesTextAlign"); } }

        /// <summary>
        /// Adhoc page viewModel, user can set rights, wartermark, expirydate by this, also can get its by this. 
        /// </summary>
        public DigitalRightsViewModel AdhocPage_ViewModel { get => adhocPage_ViewModel; }

        /// <summary>
        /// CentralPolicy page describe text, defult value is "Company define........"
        /// </summary>
        public string CpDesText { get => cpDescribeText; set { cpDescribeText = value; OnPropertyChanged("CpDesText"); } }

        /// <summary>
        /// CentralPolicy page warning describe text in under of "CtP_DescribeText", defult value is "Warning: You are protecting the file with no classification."
        /// </summary>
        public string CpWarnDesText { get => cpWarnDesText; set { cpWarnDesText = value; OnPropertyChanged("CpWarnDesText"); } }

        /// <summary>
        /// Warning describe text visibility,defult value is Collapsed
        /// </summary>
        public Visibility CpWarnDesVisible { get => cpWarnDesVisible; set { cpWarnDesVisible = value; OnPropertyChanged("CpWarnDesVisible"); } }

        /// <summary>
        /// CentralPolicy page describe all text alignment, defult value is Center
        /// </summary>
        public TextAlignment CpTextAlign { get => cpTextAlignment; set { cpTextAlignment = value; OnPropertyChanged("CpTextAlign"); } }

        /// <summary>
        /// CentralPolicy page use this to init and display classification, defult value is null
        /// </summary>
        public Classification[] CtP_Classifications { get => cpClassifications; set { cpClassifications = value; OnPropertyChanged("CtP_Classifications"); } }

        /// <summary>
        /// For modify rights, display the Inherited classifications and defult selected.
        /// This property must be set after setting CtP_Classifications property
        /// </summary>
        public Dictionary<string, List<string>> CtP_AddInheritedClassification { get => cpAddInheritedClassification; set { cpAddInheritedClassification = value; OnPropertyChanged("CtP_AddInheritedClassification"); } }

        /// <summary>
        /// ProgressBar Visibility,defult value Collapsed
        /// </summary>
        public Visibility ProgressVisible { get => progressVisible; set { progressVisible = value; OnPropertyChanged("ProgressVisible"); } }



        /// <summary>
        /// Trigger OnClassificationChanged event
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        internal void TriggerClassificationChangedEvent(object sender, RoutedPropertyChangedEventArgs<SelectClassificationEventArgs> e)
        {
            OnClassificationChanged?.Invoke(sender, e);
        }

        public event PropertyChangedEventHandler PropertyChanged;

        protected void OnPropertyChanged(string propertyName)
        {
            PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(propertyName));
        }

    }

    /// <summary>
    /// Interaction logic for FileRightsSelectEx.xaml
    /// </summary>
    public partial class FileRightsSelectEx : UserControl
    {
        private FileRightsSelectExDataMode viewMode;

        public FileRightsSelectEx()
        {
            this.Resources.MergedDictionaries.Add(SharedDictionaryManager.StringResource);

            InitializeComponent();

            this.DataContext = viewMode = new FileRightsSelectExDataMode(this);
        }

        /// <summary>
        ///  ViewModel for FileRightsSelectEx.xaml
        /// </summary>
        public FileRightsSelectExDataMode ViewMode { get => viewMode; set => this.DataContext = viewMode = value; }

        private void OnSelectClassificationChanged(object sender, RoutedPropertyChangedEventArgs<SelectClassificationEventArgs> e)
        {
            Console.WriteLine($"ClassificationChanged event: isValid({e.NewValue.IsValid}),count({e.NewValue.KeyValues.Count})");
            ViewMode.TriggerClassificationChangedEvent(sender, e);
        }

        private void Rb_Central_Checked(object sender, RoutedEventArgs e)
        {
            viewMode.InfoTitle = this.TryFindResource("CapDesc_Describe_Central").ToString();
        }

        private void Rb_Adhoc_Checked(object sender, RoutedEventArgs e)
        {
            viewMode.InfoTitle = this.TryFindResource("CapDesc_Describe_Adhoc").ToString();
        }
    }
}
