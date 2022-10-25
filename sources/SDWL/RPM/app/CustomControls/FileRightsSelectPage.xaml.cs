using CustomControls.common.sharedResource;
using CustomControls.components;
using CustomControls.components.CentralPolicy.model;
using CustomControls.components.DigitalRights.model;
using CustomControls.windows;
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
    public enum ProtectType
    {
        Adhoc,
        CentralPolicy
    }

    #region Convert
    public class Caption4VisibilityConvert : IValueConverter
    {
        public object Convert(object value, Type targetType, object parameter, CultureInfo culture)
        {
            Visibility captionVisible = (Visibility)value;
            if (captionVisible == Visibility.Visible)
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
    /// FileRightsSelectPage.xaml  DataCommands
    /// </summary>
    public class FRS_DataCommands
    {
        private static RoutedCommand changeDest;
        private static RoutedCommand positive;
        private static RoutedCommand cancel;
        private static RoutedCommand justUpload;
        static FRS_DataCommands()
        {
            changeDest = new RoutedCommand(
              "ChangeDestination", typeof(FRS_DataCommands));

            // if the IsEnable property of positive button have bindinged,
            // best not binding input gesture in Command. 
            // otherwise user can trigger command by input gesture when button isEnable property is false.
            positive = new RoutedCommand(
              "Positive", typeof(FRS_DataCommands));

            InputGestureCollection input = new InputGestureCollection();
            input.Add(new KeyGesture(Key.Escape));
            cancel = new RoutedCommand(
              "Cancel", typeof(FRS_DataCommands), input);

            justUpload = new RoutedCommand(
              "JustUpload", typeof(FRS_DataCommands));
        }
        /// <summary>
        /// FileRightsSelect.xaml change destination button command
        /// </summary>
        public static RoutedCommand ChangeDestination
        {
            get { return changeDest; }
        }
        /// <summary>
        ///  FileRightsSelect.xaml positive button command
        /// </summary>
        public static RoutedCommand Positive
        {
            get { return positive; }
        }
        /// <summary>
        /// FileRightsSelect.xaml cancel button command
        /// </summary>
        public static RoutedCommand Cancel
        {
            get { return cancel; }
        }
        /// <summary>
        /// FileRightsSelect.xaml justUpload button command
        /// </summary>
        public static RoutedCommand JustUpload
        {
            get { return justUpload; }
        }
    }

    /// <summary>
    /// ViewModel for FileRightsSelectPage.xaml
    /// </summary>
    public class FileRightsSelectViewMode : INotifyPropertyChanged
    {
        private FileRightsSelectPage host;
        // CaptionDesc.xaml ViewModel
        private CaptionDescViewMode captionViewModel;
        private Visibility captionDescVisible = Visibility.Visible;
        // CaptionDesc4.xaml ViewModel
        private CaptionDesc4ViewMode caption4ViewModel;

        private Thickness savePathStpMargin = new Thickness(222, 4, 0, 0);
        private Visibility savePathStpVisibility = Visibility.Visible;
        private string savePathDesc = "";
        private string savePath = "";
        private Visibility changDestBtnVisible = Visibility.Visible;
        private string fileTypeSelect_Sp_Lable = "";
        private Visibility descAndRadioVisible = Visibility.Visible;
        private bool adRdIsEnable = true;
        private bool cplRdIsEnable = true;
        private string positiveBtnContent = "";
        private bool positiveBtnIsEnable = true;
        private Visibility justUploadBtnVisibility = Visibility.Collapsed;
        private HorizontalAlignment btnSpHorizontalAlignment = HorizontalAlignment.Center;
        private ProtectType protectType = ProtectType.CentralPolicy;
        // adhoc page ViewModel
        private string adhocDescribeText;
        private TextAlignment adhocDescribeTextAlignment = System.Windows.TextAlignment.Center;
        private DigitalRightsViewModel adhocPage_ViewModel;
        // central Policy page ViewModel
        private string cpDescribeText;
        private string cpWarnDesText;
        private Visibility cpWarnDesVisible = Visibility.Collapsed;
        private TextAlignment cpTextAlignment = TextAlignment.Center;
        private Classification[] cpClassifications = new Classification[0];
        private Dictionary<string, List<string>> cpAddInheritedClassification = new Dictionary<string, List<string>>();

        public FileRightsSelectViewMode(FileRightsSelectPage page)
        {
            host = page;
            captionViewModel = host.captionCom.ViewModel;
            caption4ViewModel = host.captionCom4.ViewModel;

            savePathDesc = host.TryFindResource("FileRightsSelect_SavePath_Lable").ToString();
            fileTypeSelect_Sp_Lable = this.host.TryFindResource("FileRightsSelect_Sp_Lable").ToString();
            adhocDescribeText = this.host.TryFindResource("Rights_User_Text").ToString();
            adhocPage_ViewModel = host.page_Adhoc.ViewModel;

            cpDescribeText = this.host.TryFindResource("Rights_Company_Text").ToString();
            cpWarnDesText = this.host.TryFindResource("Rights_Warning_Text").ToString();
        }

        /// <summary>
        /// Raido button checked event handler
        /// </summary>
        public event RoutedEventHandler OnRadioBtnChecked;

        /// <summary>
        /// if centralPolicy page selected classifications changed, will trigger this event hander.
        /// Other users can use this to process or directly monitor central policy page SelectClassificationChanged routed event in xaml.
        /// </summary>
        public event RoutedPropertyChangedEventHandler<SelectClassificationEventArgs> OnClassificationChanged;

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
        /// Save path stackPanel margin,defult value is (222, 4, 0, 0)
        /// </summary>
        public Thickness SavePathStpMargin { get => savePathStpMargin; set { savePathStpMargin = value; OnPropertyChanged("SavePathStpMargin"); } }

        /// <summary>
        /// Control Save path StackPanel visibility, defult value is Visibility.Visible
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
        /// Control change destination button visibility, defult value is Visibility.Visible, it's part of SavePath StackPanel
        /// </summary>
        public Visibility ChangDestBtnVisible { get => changDestBtnVisible; set { changDestBtnVisible = value; OnPropertyChanged("ChangDestVisible"); } }

        /// <summary>
        /// File type select describe, defult value is 'Specify permissions to the file'
        /// </summary>
        public string FileTypeSelect_Sp_Lable { get => fileTypeSelect_Sp_Lable; set { fileTypeSelect_Sp_Lable = value; OnPropertyChanged("FileTypeSelect_Sp_Lable"); } }

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
        /// Positive button content, defult value is ""
        /// </summary>
        public string PositiveBtnContent { get => positiveBtnContent; set { positiveBtnContent = value; OnPropertyChanged("PositiveBtnContent"); } }

        /// <summary>
        /// Positive button IsEnable, defult value is true
        /// </summary>
        public bool PositiveBtnIsEnable { get => positiveBtnIsEnable; set { positiveBtnIsEnable = value; OnPropertyChanged("PositiveBtnIsEnable"); } }

        /// <summary>
        /// ‘Do not protecct, just upload’ button visibility,defult value is Collapsed
        /// </summary>
        public Visibility JustUploadBtnVisibility { get => justUploadBtnVisibility; set { justUploadBtnVisibility = value; OnPropertyChanged("JustUploadBtnVisibility"); } }

        /// <summary>
        /// Positive and Cancel button StackPanel HorizontalAlignment, defult value is center.
        /// </summary>
        public HorizontalAlignment BtnSpHorizontalAlignment { get => btnSpHorizontalAlignment; set { btnSpHorizontalAlignment = value; OnPropertyChanged("BtnSpHorizontalAlignment"); } }

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
        /// Invoke Radio btn checked event handler
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        internal void TriggerRadioBtnCheckedEvent(object sender, RoutedEventArgs e)
        {
            OnRadioBtnChecked?.Invoke(sender, e);
        }

        /// <summary>
        /// Invoke OnClassificationChanged event hander
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
    /// Interaction logic for FileRightsSelectPage.xaml
    /// </summary>
    public partial class FileRightsSelectPage : Page
    {
        private FileRightsSelectViewMode viewMode;
        public FileRightsSelectPage()
        {
            this.Resources.MergedDictionaries.Add(SharedDictionaryManager.StringResource);
            this.Resources.MergedDictionaries.Add(SharedDictionaryManager.UnifiedBtnStyle);

            InitializeComponent();

            this.DataContext = viewMode = new FileRightsSelectViewMode(this);

            BindingCommand();
        }
        /// <summary>
        ///  ViewModel for FileRightsSelectUserControl.xaml
        /// </summary>
        public FileRightsSelectViewMode ViewMode { get => viewMode; set => this.DataContext = viewMode = value; }

        /// <summary>
        /// Binding ChangeWaterMark and ChangeExpiry command
        /// </summary>
        private void BindingCommand()
        {
            // Create bindings.
            CommandBinding binding;
            binding = new CommandBinding(SDR_DataCommands.ChangeWaterMark);
            binding.Executed += ChangeWarterMarkCommand;
            this.CommandBindings.Add(binding);

            binding = new CommandBinding(SDR_DataCommands.ChangeExpiry);
            binding.Executed += ChangeExpiryCommand;
            this.CommandBindings.Add(binding);
        }
        private void ChangeWarterMarkCommand(object sender, ExecutedRoutedEventArgs e)
        {
            try
            {
                EditWatermarkWindow editWatermarkWindow = new EditWatermarkWindow(ViewMode.AdhocPage_ViewModel.Watermarkvalue);
                editWatermarkWindow.WatermarkHandler += (ss, ee) =>
                {
                    ViewMode.AdhocPage_ViewModel.Watermarkvalue = ee.Watermarkvalue;
                };
                editWatermarkWindow.ShowDialog();
            }
            catch (Exception msg)
            {
                Console.WriteLine("Error in EditWatermarkWindow:", msg);
            }
        }
        private void ChangeExpiryCommand(object sender, ExecutedRoutedEventArgs e)
        {
            try
            {
                ValiditySpecifyWindow validitySpecifyWindow = new ValiditySpecifyWindow(ViewMode.AdhocPage_ViewModel.Expiry);
                validitySpecifyWindow.ValidationUpdated += (ss, ee) =>
                {
                    ViewMode.AdhocPage_ViewModel.Expiry = ee.Expiry;
                    ViewMode.AdhocPage_ViewModel.ExpireDateValue = ee.ValidityContent;
                };
                validitySpecifyWindow.ShowDialog();
            }
            catch (Exception msg)
            {
                Console.WriteLine("Error in EditWatermarkWindow:", msg);
            }
        }

        private void OnSelectClassificationChanged(object sender, RoutedPropertyChangedEventArgs<SelectClassificationEventArgs> e)
        {
            Console.WriteLine($"CustomControl SelectClassificationChanged event: isValid({e.NewValue.IsValid}),select count({e.NewValue.KeyValues.Count})");
            
            ViewMode.TriggerClassificationChangedEvent(sender, e);
        }

        // if user care radionButton checked event, can define routed event in ViewModel and trigger it in here.
        // or directly monitor ToggleButton.Checked RoutedEvent in xaml
        private void Rb_Central_Checked(object sender, RoutedEventArgs e)
        {
            ViewMode.CaptionVM.Description = this.TryFindResource("CapDesc_Describe_Central").ToString();
            ViewMode.TriggerRadioBtnCheckedEvent(sender,e);
        }

        private void Rb_Adhoc_Checked(object sender, RoutedEventArgs e)
        {
            ViewMode.CaptionVM.Description = this.TryFindResource("CapDesc_Describe_Adhoc").ToString();
            ViewMode.TriggerRadioBtnCheckedEvent(sender,e);
        }
    }
}
