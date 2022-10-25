using CustomControls.common.helper;
using CustomControls.components.ValiditySpecify.model;
using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Linq;
using System.Text;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Documents;
using System.Windows.Media;
using System.Windows.Media.Imaging;

namespace CustomControls.components.DigitalRights.model
{
    /// <summary>
    /// ViewModel for SelectDigitalRights.xaml
    /// </summary>
    public class DigitalRightsViewModel : INotifyPropertyChanged
    {
        private SelectDigitalRights host;
        private double waterMkTbMaxWidth = 330;
        private double expireDateTbMaxWidth = 300;
        private CheckStatus warterMarkCheckStatus;
        private string watermarkvalue;
        private string expireDateValue;
        private IExpiry expiry;
        private HashSet<FileRights> rights = new HashSet<FileRights>() { FileRights.RIGHT_VIEW, FileRights.RIGHT_VALIDITY };
        private HashSet<FileRights> uncheckRights = new HashSet<FileRights>();
        private HashSet<FileRights> disableRights = new HashSet<FileRights>();
        private HashSet<FileRights> isEnableRights = new HashSet<FileRights>();
        

        public DigitalRightsViewModel(SelectDigitalRights page)
        {
            host = page;
            watermarkvalue = "$(Date)$(Time)$(Break)$(User)";
            expireDateValue = this.host.TryFindResource("ValidityCom_Never_Description2").ToString();
            expiry = new NeverExpireImpl();

            warterMarkCheckStatus = CheckStatus.UNCHECKED;
        }
        
        /// <summary>
        /// The MaxWidth of TextBlock to display Watermarkvalue, defult value is 330
        /// </summary>
        public double WaterMkTbMaxWidth { get => waterMkTbMaxWidth; set { waterMkTbMaxWidth = value; OnPropertyChanged("WaterMkTbMaxWidth"); } }

        /// <summary>
        /// The MaxWidth of TextBlock to display ExpireDateValue, defult value is 300
        /// </summary>
        public double ExpireDateTbMaxWidth { get => expireDateTbMaxWidth; set { expireDateTbMaxWidth = value; OnPropertyChanged("ExpireDateTbMaxWidth"); } }
        
        /// <summary>
        /// Internal binding property, use for wartermark textblock and change button visibility.
        /// </summary>
        public CheckStatus WarterMarkCheckStatus { get=>warterMarkCheckStatus; set { warterMarkCheckStatus = value; OnPropertyChanged("WarterMarkCheckStatus"); } }

        /// <summary>
        /// Display watermarkValue, defult value is '$(Date)$(Time)$(Break)$(User)'.
        /// </summary>
        public string Watermarkvalue { get=>watermarkvalue; set { SetWaterMark(value); OnPropertyChanged("Watermarkvalue"); } }

        /// <summary>
        /// Display standard time format, defult value is 'Never expire'.
        /// </summary>
        public string ExpireDateValue { get=>expireDateValue; set { expireDateValue = value; OnPropertyChanged("ExpireDateValue"); } }

        /// <summary>
        /// ExpireDate data model, user can set value, also can get updated value by this property, defult type is 'NeverExpireImpl'.
        /// </summary>
        public IExpiry Expiry { get => expiry; set { SetExpiry(value); SetExpireDateValue(value); } }

        /// <summary>
        /// Selected rights, user can set defult selected rights, also can get selected rights by this property. RIGHT_VIEW and RIGHT_VALIDITY
        /// are mandatory by default
        /// </summary>
        public HashSet<FileRights> Rights { get => rights; set { SetCheckedRights(value); } }

        /// <summary>
        /// Set uncheck right, it's usually used with 'DisableRights' property
        /// </summary>
        public HashSet<FileRights> UncheckRights { get => uncheckRights; set { uncheckRights = value; SetUnCheckRights(value); } }

        /// <summary>
        /// Disable right checkbox
        /// </summary>
        public HashSet<FileRights> DisableRights { get => disableRights; set { disableRights = value; SetDisableRights(value); } }

        /// <summary>
        /// IsEnable right checkbox
        /// </summary>
        public HashSet<FileRights> IsEnableRights { get => isEnableRights; set { isEnableRights = value; SetIsEnableRights(value); } }

        #region private method
        private const string DOLLAR_USER = "$(User)";
        private const string DOLLAR_BREAK = "$(Break)";
        private const string DOLLAR_DATE = "$(Date)";
        private const string DOLLAR_TIME = "$(Time)";
        private string PRESET_VALUE_EMAIL_ID;
        private string PRESET_VALUE_DATE;
        private string PRESET_VALUE_TIME;
        private string PRESET_VALUE_LINE_BREAK;
        private void SetWaterMark(string value)
        {
            if (value == null)
            {
                return;
            }

            string initWatermark = value;
            if (initWatermark.Contains("\n"))
            {
                initWatermark = initWatermark.Replace("\n", DOLLAR_BREAK);
            }
            watermarkvalue = initWatermark;

            this.host.tbWaterMark.Inlines.Clear();
            PRESET_VALUE_EMAIL_ID = host.TryFindResource("Preset_User_ID").ToString();
            PRESET_VALUE_DATE = host.TryFindResource("Preset_Date").ToString();
            PRESET_VALUE_TIME = host.TryFindResource("Preset_Time").ToString();
            PRESET_VALUE_LINE_BREAK = host.TryFindResource("Preset_Line_break").ToString();
            ConvertString2PresetValue(initWatermark);
        }
        private void ConvertString2PresetValue(string initValue)
        {
            if (string.IsNullOrEmpty(initValue))
            {
                return;
            }

            char[] array = initValue.ToCharArray();
            // record preset value begin index
            int beginIndex = -1;
            // record preset value end index
            int endIndex = -1;
            for (int i = 0; i < array.Length; i++)
            {
                if (array[i] == '$')
                {
                    beginIndex = i;
                }
                else if (array[i] == ')')
                {
                    endIndex = i;
                }

                if (beginIndex != -1 && endIndex != -1 && beginIndex < endIndex)
                {

                    // append text before preset value
                    Run run = new Run(initValue.Substring(0, beginIndex));
                    this.host.tbWaterMark.Inlines.Add(run);

                    // judge if is preset
                    string subStr = initValue.Substring(beginIndex, endIndex - beginIndex + 1);

                    if (subStr.Equals(DOLLAR_USER))
                    {
                        AddPreset(DOLLAR_USER);
                    }
                    else if (subStr.Equals(DOLLAR_BREAK))
                    {
                        AddPreset(DOLLAR_BREAK);
                    }
                    else if (subStr.Equals(DOLLAR_DATE))
                    {
                        AddPreset(DOLLAR_DATE);
                    }
                    else if (subStr.Equals(DOLLAR_TIME))
                    {
                        AddPreset(DOLLAR_TIME);
                    }
                    else
                    {
                        Run r = new Run(subStr);
                        this.host.tbWaterMark.Inlines.Add(r);
                    }

                    // quit
                    break;
                }
            }

            if (beginIndex == -1 || endIndex == -1 || beginIndex > endIndex) // have not preset
            {
                Run run = new Run(initValue);
                this.host.tbWaterMark.Inlines.Add(run);
            }
            else if (beginIndex < endIndex)
            {
                if (endIndex + 1 < initValue.Length)
                {
                    // Converter the remaining by recursive
                    ConvertString2PresetValue(initValue.Substring(endIndex + 1));
                }
            }
        }
        private void AddPreset(string preset)
        {
            Run run = new Run();

            Run space = new Run(" ");
            this.host.tbWaterMark.Inlines.Add(space);

            switch (preset)
            {
                case DOLLAR_USER:
                    run.Text = PRESET_VALUE_EMAIL_ID;
                    run.Background = new SolidColorBrush(Color.FromRgb(0XD4, 0XEF, 0XDF));
                    break;
                case DOLLAR_BREAK:
                    run.Text = PRESET_VALUE_LINE_BREAK;
                    run.Background = new SolidColorBrush(Color.FromRgb(0XFA, 0XD7, 0XB8));
                    break;
                case DOLLAR_DATE:
                    run.Text = PRESET_VALUE_DATE;
                    run.Background = new SolidColorBrush(Color.FromRgb(0XD4, 0XEF, 0XDF));
                    break;
                case DOLLAR_TIME:
                    run.Text = PRESET_VALUE_TIME;
                    run.Background = new SolidColorBrush(Color.FromRgb(0XD4, 0XEF, 0XDF));
                    break;
                default:
                    break;
            }

            this.host.tbWaterMark.Inlines.Add(run);

            Run space2 = new Run(" ");
            this.host.tbWaterMark.Inlines.Add(space2);
        }

        //private void AddPreset(string preset)
        //{
        //    Button btn = new Button();
        //    btn.Style = (Style)host.FindResource("WaterMarkPresetBtn");

        //    switch (preset)
        //    {
        //        case DOLLAR_USER:
        //            btn.Content = PRESET_VALUE_EMAIL_ID;
        //            break;
        //        case DOLLAR_BREAK:
        //            btn.Content = PRESET_VALUE_LINE_BREAK;
        //            break;
        //        case DOLLAR_DATE:
        //            btn.Content = PRESET_VALUE_DATE;
        //            break;
        //        case DOLLAR_TIME:
        //            btn.Content = PRESET_VALUE_TIME;
        //            break;
        //        default:
        //            break;
        //    }
        //    InlineUIContainer uiContainer = new InlineUIContainer(btn);
        //    this.host.tbWaterMark.Inlines.Add(uiContainer);
        //}

        //private void AddPreset(string preset)
        //{
        //    Image imag = new Image()
        //    {
        //        Margin = new Thickness(2, 0, 2, 0),
        //        Stretch = Stretch.None,
        //        //Height = 15,
        //        VerticalAlignment = VerticalAlignment.Bottom
        //    };
            
        //    switch (preset)
        //    {
        //        // In here can't use below Uri, must add "pack://application:,,,".
        //        //imag.Source = new BitmapImage(new Uri("/CustomControls;component/resources/icons/line_break.png", UriKind.Relative));
        //        case DOLLAR_USER:
        //            //imag.Width = 25;
        //            imag.Source = new BitmapImage(new Uri("pack://application:,,,/CustomControls;component/resources/icons/user_id.png"));
        //            break;
        //        case DOLLAR_BREAK:
        //            //imag.Width = 35;
        //            imag.Source = new BitmapImage(new Uri("pack://application:,,,/CustomControls;component/resources/icons/line_break.png"));
        //            break;
        //        case DOLLAR_DATE:
        //            //imag.Width = 25;
        //            imag.Source = new BitmapImage(new Uri("pack://application:,,,/CustomControls;component/resources/icons/date.png"));
        //            break;
        //        case DOLLAR_TIME:
        //            //imag.Width = 25;
        //            imag.Source = new BitmapImage(new Uri("pack://application:,,,/CustomControls;component/resources/icons/time.png"));
        //            break;
        //        default:
        //            break;
        //    }
        //    InlineUIContainer uiContainer = new InlineUIContainer(imag);
        //    this.host.tbWaterMark.Inlines.Add(uiContainer);
        //}

        private void SetExpiry(IExpiry value)
        {
            expiry = value;
        }
        private void SetExpireDateValue(IExpiry value)
        {
            IExpiry expiry = value;
            string expiryDate = "";
            if (expiry != null)
            {
                int opetion = expiry.GetOpetion();
                switch (opetion)
                {
                    case 0:
                        expiryDate = this.host.TryFindResource("ValidityCom_Never_Description2").ToString();
                        break;
                    case 1://validitySpecifyConfig.ExpiryMode == ExpiryMode.RELATIVE
                        IRelative relative = (IRelative)expiry;
                        int years = relative.GetYears();
                        int months = relative.GetMonths();
                        int weeks = relative.GetWeeks();
                        int days = relative.GetDays();
                        if (years == 0 && months == 0 && weeks == 0 && days == 0)
                        {
                            days = 1;
                        }
                        DateTime dateStart = new DateTime(DateTime.Now.Year, DateTime.Now.Month, DateTime.Now.Day, 0, 0, 0);
                        string dateRelativeS = dateStart.ToString(DateTimeHelper.DateTimeFormat);
                        DateTime dateEnd = dateStart.AddYears(years).AddMonths(months).AddDays(7 * weeks + days - 1).AddHours(23).AddMinutes(59).AddSeconds(59);
                        string dateRelativeE = dateEnd.ToString(DateTimeHelper.DateTimeFormat);

                        expiryDate = dateRelativeS + " To " + dateRelativeE;
                        break;
                    case 2://validitySpecifyConfig.ExpiryMode == ExpiryMode.ABSOLUTE_DATE
                        IAbsolute absolute = (IAbsolute)expiry;
                        expiryDate = "Until " + DateTimeHelper.TimestampToDateTime(absolute.EndDate());
                        break;
                    case 3://validitySpecifyConfig.ExpiryMode == ExpiryMode.DATA_RANGE
                        IRange range = (IRange)expiry;
                        string rStart = DateTimeHelper.TimestampToDateTime(range.StartDate());
                        string rEnd = DateTimeHelper.TimestampToDateTime(range.EndDate());
                        expiryDate = rStart + " To " + rEnd;
                        break;
                    default:
                        break;
                }
            }
           ExpireDateValue = expiryDate;
        }
        private void SetCheckedRights(HashSet<FileRights> value)
        {
            rights.Add(FileRights.RIGHT_VIEW);
            rights.Add(FileRights.RIGHT_VALIDITY);

            foreach (var item in value)
            {
                rights.Add(item);
            }
            foreach (var item in value)
            {
                switch (item)
                {
                    case FileRights.RIGHT_EDIT:
                        this.host.Edit.IsChecked = true;
                        break;
                    case FileRights.RIGHT_PRINT:
                        this.host.Print.IsChecked = true;
                        break;
                    case FileRights.RIGHT_SHARE:
                        this.host.Share.IsChecked = true;
                        break;
                    case FileRights.RIGHT_SAVEAS:
                        this.host.SaveAs.IsChecked = true;
                        break;
                    case FileRights.RIGHT_WATERMARK:
                        this.host.Watermark.IsChecked = true;
                        break;
                    case FileRights.RIGHT_DECRYPT:
                        this.host.Decrypt.IsChecked = true;
                        break;
                }
            }
        }
        private void SetUnCheckRights(HashSet<FileRights> value)
        {
            foreach (var item in value)
            {
                switch (item)
                {
                    case FileRights.RIGHT_EDIT:
                        this.host.Edit.IsChecked = false;
                        break;
                    case FileRights.RIGHT_PRINT:
                        this.host.Print.IsChecked = false;
                        break;
                    case FileRights.RIGHT_SHARE:
                        this.host.Share.IsChecked = false;
                        break;
                    case FileRights.RIGHT_SAVEAS:
                        this.host.SaveAs.IsChecked = false;
                        break;
                    case FileRights.RIGHT_WATERMARK:
                        this.host.Watermark.IsChecked = false;
                        break;
                    case FileRights.RIGHT_DECRYPT:
                        this.host.Decrypt.IsChecked = false;
                        break;
                }
            }
        }
        private void SetDisableRights(HashSet<FileRights> value)
        {
            foreach (var item in value)
            {
                switch (item)
                {
                    case FileRights.RIGHT_EDIT:
                        this.host.Edit.IsEnabled = false;
                        break;
                    case FileRights.RIGHT_PRINT:
                        this.host.Print.IsEnabled = false;
                        break;
                    case FileRights.RIGHT_SHARE:
                        this.host.Share.IsEnabled = false;
                        break;
                    case FileRights.RIGHT_SAVEAS:
                        this.host.SaveAs.IsEnabled = false;
                        break;
                    case FileRights.RIGHT_WATERMARK:
                        this.host.Watermark.IsEnabled = false;
                        break;
                    case FileRights.RIGHT_DECRYPT:
                        this.host.Decrypt.IsEnabled = false;
                        break;
                }
            }
        }
        private void SetIsEnableRights(HashSet<FileRights> value)
        {
            foreach (var item in value)
            {
                switch (item)
                {
                    case FileRights.RIGHT_EDIT:
                        this.host.Edit.IsEnabled = true;
                        break;
                    case FileRights.RIGHT_PRINT:
                        this.host.Print.IsEnabled = true;
                        break;
                    case FileRights.RIGHT_SHARE:
                        this.host.Share.IsEnabled = true;
                        break;
                    case FileRights.RIGHT_SAVEAS:
                        this.host.SaveAs.IsEnabled = true;
                        break;
                    case FileRights.RIGHT_WATERMARK:
                        this.host.Watermark.IsEnabled = true;
                        break;
                    case FileRights.RIGHT_DECRYPT:
                        this.host.Decrypt.IsEnabled = true;
                        break;
                }
            }
        }
        #endregion


        public event PropertyChangedEventHandler PropertyChanged;

        protected void OnPropertyChanged(string propertyName)
        {
            PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(propertyName));
        }
    }

}
