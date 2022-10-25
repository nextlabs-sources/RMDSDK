using CustomControls.components.DigitalRights.model;
using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.ComponentModel;
using System.Linq;
using System.Text;
using System.Windows;
using System.Windows.Documents;
using System.Windows.Media;
using System.Windows.Media.Imaging;

namespace CustomControls.components.RightsDisplay.model
{
    ///  <summary>
    /// ViewModel for RightsStackPanle.xaml 
    /// </summary>
    public class RightsStPanViewModel : INotifyPropertyChanged
    {
        private RightsStackPanle host;
        private HashSet<FileRights> fileRights = new HashSet<FileRights>();
        private ObservableCollection<RightsItem> rightsList = new ObservableCollection<RightsItem>();
        private int rightsColumn = 8;
        private string waterLabel;
        private string validityLabel;
        private string watermarkValue;
        private string validityValue;
        private Visibility waterPanlVisibility;
        private Visibility validityPanlVisibility;

        public RightsStPanViewModel(RightsStackPanle host)
        {
            this.host = host;
            waterLabel = this.host.TryFindResource("Label_WaterMark").ToString();
            validityLabel = this.host.TryFindResource("Label_Validity").ToString();
        }

        /// <summary>
        /// User can use 'FileRights' type set display rights list
        /// </summary>
        public HashSet<FileRights> FileRights { get=> fileRights; set { fileRights = value; SetRightsItemList(value); } }
        private void SetRightsItemList(HashSet<FileRights> fileRights)
        {
            ObservableCollection<RightsItem> rightsItems = new ObservableCollection<RightsItem>();
            if (fileRights.Contains(DigitalRights.model.FileRights.RIGHT_VIEW))
            {
                rightsItems.Add(new RightsItem(new BitmapImage(new Uri("/CustomControls;component/resources/icons/icon_rights_view.png", UriKind.Relative)),
                            host.TryFindResource("RightsItem_View").ToString()));
            }
            if (fileRights.Contains(DigitalRights.model.FileRights.RIGHT_PRINT))
            {
                rightsItems.Add(new RightsItem(new BitmapImage(new Uri("/CustomControls;component/resources/icons/icon_rights_print.png", UriKind.Relative)),
                           host.TryFindResource("RightsItem_Print").ToString()));
            }
            if (fileRights.Contains(DigitalRights.model.FileRights.RIGHT_SHARE))
            {
                rightsItems.Add(new RightsItem(new BitmapImage(new Uri("/CustomControls;component/resources/icons/icon_rights_share.png", UriKind.Relative)),
                           host.TryFindResource("RightsItem_Share").ToString()));
            }
            if (fileRights.Contains(DigitalRights.model.FileRights.RIGHT_SAVEAS))
            {
                rightsItems.Add(new RightsItem(new BitmapImage(new Uri("/CustomControls;component/resources/icons/icon_rights_save_as.png", UriKind.Relative)),
                          host.TryFindResource("RightsItem_SaveAs").ToString()));
            }
            if (fileRights.Contains(DigitalRights.model.FileRights.RIGHT_EDIT))
            {
                rightsItems.Add(new RightsItem(new BitmapImage(new Uri("/CustomControls;component/resources/icons/icon_rights_edit.png", UriKind.Relative)),
                           host.TryFindResource("RightsItem_Edit").ToString()));
            }
            if (fileRights.Contains(DigitalRights.model.FileRights.RIGHT_DECRYPT))
            {
                rightsItems.Add(new RightsItem(new BitmapImage(new Uri("/CustomControls;component/resources/icons/icon_rights_extract.png", UriKind.Relative)),
                           host.TryFindResource("RightsItem_Extract").ToString()));
            }
            if (fileRights.Contains(DigitalRights.model.FileRights.RIGHT_WATERMARK))
            {
                rightsItems.Add(new RightsItem(new BitmapImage(new Uri("/CustomControls;component/resources/icons/icon_rights_watermark.png", UriKind.Relative)),
                           host.TryFindResource("RightsItem_Watermark").ToString()));
            }
            if (fileRights.Contains(DigitalRights.model.FileRights.RIGHT_VALIDITY))
            {
                rightsItems.Add(new RightsItem(new BitmapImage(new Uri("/CustomControls;component/resources/icons/icon_rights_validity.png", UriKind.Relative)),
                           host.TryFindResource("RightsItem_Validity").ToString()));
            }
           
            RightsList = rightsItems;
        }

        /// <summary>
        /// User can use 'RightsItem' type set display rights list
        /// </summary>
        public ObservableCollection<RightsItem> RightsList { get => rightsList; set { rightsList = value; SetRightsColumn(value.Count); OnPropertyChanged("RightsList");} }
        private void SetRightsColumn(int colum)
        {
            RightsColumn = colum;
        }
        /// <summary>
        /// Icon column, when set 'RightsList' property will set this. 
        /// User also can set this property, but must be set after set 'FileRights' or 'RightsList'.
        /// </summary>
        public int RightsColumn { get => rightsColumn; set { rightsColumn = value; OnPropertyChanged("RightsColumn"); } }
        /// <summary>
        /// WaterMark label, defult value is "Watermark:".
        /// </summary>
        public string WaterLabel { get => waterLabel; set { waterLabel = value; OnPropertyChanged("WaterLabel"); } }
        /// <summary>
        /// Validity Label, defult value is "Validity:".
        /// </summary>
        public string ValidityLabel { get => validityLabel; set { validityLabel = value; OnPropertyChanged("ValidityLabel"); } }
        /// <summary>
        /// Wartermark value
        /// </summary>
        public string WatermarkValue { get => watermarkValue; set { SetWaterMark(value); OnPropertyChanged("WatermarkValue"); } }

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
            if (string.IsNullOrWhiteSpace(value))
            {
                return;
            }
            this.host.tbWaterMark.Text = value;
            //if (value == null)
            //{
            //    return;
            //}

            //string initWatermark = value;
            //if (initWatermark.Contains("\n"))
            //{
            //    initWatermark = initWatermark.Replace("\n", DOLLAR_BREAK);
            //}
            //watermarkValue = initWatermark;

            //this.host.tbWaterMark.Inlines.Clear();
            //PRESET_VALUE_EMAIL_ID = host.TryFindResource("Preset_User_ID").ToString();
            //PRESET_VALUE_DATE = host.TryFindResource("Preset_Date").ToString();
            //PRESET_VALUE_TIME = host.TryFindResource("Preset_Time").ToString();
            //PRESET_VALUE_LINE_BREAK = host.TryFindResource("Preset_Line_break").ToString();
            //ConvertString2PresetValue(initWatermark);
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

        /// <summary>
        /// Expiry date value
        /// </summary>
        public string ValidityValue { get => validityValue; set { validityValue = value; OnPropertyChanged("ValidityValue"); } }
        /// <summary>
        /// Watermark stackPanel visibility
        /// </summary>
        public Visibility WaterPanlVisibility { get => waterPanlVisibility; set { waterPanlVisibility = value; OnPropertyChanged("WaterPanlVisibility"); } }
        /// <summary>
        /// Expiry date stackPanel visibility
        /// </summary>
        public Visibility ValidityPanlVisibility { get => validityPanlVisibility; set { validityPanlVisibility = value; OnPropertyChanged("ValidityPanlVisibility"); } }

        

        public event PropertyChangedEventHandler PropertyChanged;
        protected void OnPropertyChanged(string propertyName)
        {
            PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(propertyName));
        }

    }
}
