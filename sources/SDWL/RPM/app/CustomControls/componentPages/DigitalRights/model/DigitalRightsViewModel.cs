using CustomControls.common.helper;
using CustomControls.components.ValiditySpecify.model;
using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Linq;
using System.Text;
using System.Windows;

namespace CustomControls.pages.DigitalRights.model
{
    /// <summary>
    /// ViewModel for SelectDigitalRights.xaml
    /// </summary>
    public class DigitalRightsViewModel : INotifyPropertyChanged
    {
        private SelectDigitalRights host;
        private string describeText;
        private System.Windows.TextAlignment describeTextAlignment = System.Windows.TextAlignment.Center;
        private double waterMkTbMaxWidth = 300;
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
            describeText = this.host.TryFindResource("SelectRights_Title").ToString();
            watermarkvalue = "$(Date)$(Time)$(Break)$(User)";
            expireDateValue = this.host.TryFindResource("ValidityCom_Never_Description2").ToString();
            expiry = new NeverExpireImpl();

            warterMarkCheckStatus = CheckStatus.UNCHECKED;
        }

        /// <summary>
        /// Describe title, defult value is 'User-defined rights are pre-defined permissions that you can apply to your documents.'
        /// </summary>
        public string DescribeText { get => describeText; set { describeText = value; OnPropertyChanged("DescribeText"); } }

        /// <summary>
        /// Describe title TextAlignment, defult value is 'System.Windows.TextAlignment.Center'
        /// </summary>
        public TextAlignment DescribeTextAlignment { get => describeTextAlignment; set { describeTextAlignment = value; OnPropertyChanged("DescribeTextAlignment"); } }
        
        /// <summary>
        /// The MaxWidth of TextBlock to display Watermarkvalue, defult value is 300
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
        private void SetWaterMark(string value)
        {
            if (value == null)
            {
                return;
            }

            string initWatermark = value;
            if (initWatermark.Contains("\n"))
            {
                initWatermark = initWatermark.Replace("\n", "$(Break)");
            }
            watermarkvalue = initWatermark;
        }
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
