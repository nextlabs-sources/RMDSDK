using CustomControls.components.RightsDisplay.model;
using CustomControls.components.ValiditySpecify.model;
using CustomControls.components.CentralPolicy.model;
using CustomControls.components.DigitalRights.model;
using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Drawing;
using System.Drawing.Imaging;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Media.Imaging;

namespace WinFormControlLibrary
{
    public class DataConvertHelp
    {
        /// <summary>
        /// Winform Bitmap convert to WPF BitmapImage
        /// </summary>
        /// <param name="bitmap"></param>
        /// <returns></returns>
        internal static BitmapImage GDIToWpfBitmap(Bitmap bitmap)
        {
            using (MemoryStream stream = new MemoryStream())
            {
                bitmap.Save(stream, ImageFormat.Png);
                stream.Position = 0;
                BitmapImage result = new BitmapImage();
                result.BeginInit();
                // According to MSDN, "The default OnDemand cache option retains access to the stream until the image is needed."
                // Force the bitmap to load right now so we can dispose the stream.
                result.CacheOption = BitmapCacheOption.OnLoad;
                result.StreamSource = stream;
                result.EndInit();
                //result.Freeze();
                return result;
            }
        }

        #region Expiration and UI Expiration interconvert
        public static long DateTimeToTimestamp(DateTime time)
        {
            DateTime startDateTime = TimeZone.CurrentTimeZone.ToLocalTime(new DateTime(1970, 1, 1, 0, 0, 0));
            return Convert.ToInt64((time - startDateTime).TotalMilliseconds);
        }
        public static string TimestampToDateTime(long time)
        {
            DateTime startDateTime = TimeZone.CurrentTimeZone.ToLocalTime(new DateTime(1970, 1, 1, 0, 0, 0));
            DateTime newTime = startDateTime.AddMilliseconds(time);
            return newTime.ToString("MMMM dd, yyyy");
        }

        /// <summary>
        /// WinFormControlLibrary.Expiration type 
        /// convert to 
        /// CustomControls.components.ValiditySpecify.model.IExpiry type
        /// </summary>
        /// <param name="expiration"></param>
        /// <returns></returns>
        internal static IExpiry Expiration2ValiditySpecifyModel(Expiration expiration)
        {
            IExpiry expiry = new NeverExpireImpl(); ;
            switch (expiration.type)
            {
                case ExpiryType.NEVER_EXPIRE:
                    expiry = new NeverExpireImpl();
                    break;
                case ExpiryType.RELATIVE_EXPIRE:
                    int years = (int)(expiration.Start >> 32);
                    int months = (int)expiration.Start;
                    int weeks = (int)(expiration.End >> 32);
                    int days = (int)expiration.End;
                    expiry = new RelativeImpl(years, months, weeks, days);
                    break;
                case ExpiryType.ABSOLUTE_EXPIRE:
                    expiry = new AbsoluteImpl(expiration.End);
                    break;
                case ExpiryType.RANGE_EXPIRE:
                    expiry = new RangeImpl(expiration.Start, expiration.End);
                    break;
            }
            return expiry;
        }
        /// <summary>
        ///  CustomControls.components.ValiditySpecify.model.IExpiry type
        ///  convert to 
        ///  WinFormControlLibrary.Expiration type 
        /// </summary>
        /// <param name="expiry"></param>
        /// <returns></returns>
        internal static Expiration ValiditySpecifyModel2Expiration(IExpiry expiry)
        {
            Expiration expiration = new Expiration();

            int exType = expiry.GetOpetion();
            //Get current year,month,day.
            int year = DateTime.Now.Year;
            int month = DateTime.Now.Month;
            int day = DateTime.Now.Day;
            DateTime dateStart = new DateTime(year, month, day, 0, 0, 0);
            switch (exType)
            {
                case 0:
                    INeverExpire neverExpire = (INeverExpire)expiry;
                    expiration.type = ExpiryType.NEVER_EXPIRE;
                    break;
                case 1:
                    IRelative relative = (IRelative)expiry;
                    int years = relative.GetYears();
                    int months = relative.GetMonths();
                    int weeks = relative.GetWeeks();
                    int days = relative.GetDays();
                    Console.WriteLine("years:{0}-months:{1}-weeks:{2}-days{3}", years, months, weeks, days);

                    expiration.type = ExpiryType.RELATIVE_EXPIRE;

                    DateTime relativeEnd = dateStart.AddYears(years).AddMonths(months).AddDays(7 * weeks + days - 1).AddHours(23).AddMinutes(59).AddSeconds(59);
                    expiration.Start = 0;
                    expiration.End = DateTimeToTimestamp(relativeEnd);

                    break;
                case 2:
                    IAbsolute absolute = (IAbsolute)expiry;
                    long endAbsDate = absolute.EndDate();
                    Console.WriteLine("absEndDate:{0}", endAbsDate);

                    expiration.type = ExpiryType.ABSOLUTE_EXPIRE;
                    expiration.Start = DateTimeToTimestamp(dateStart);
                    expiration.End = endAbsDate;
                    break;
                case 3:
                    IRange range = (IRange)expiry;
                    long startDate = range.StartDate();
                    long endDate = range.EndDate();
                    Console.WriteLine("StartDate:{0},EndDate{1}", startDate, endDate);

                    expiration.type = ExpiryType.RANGE_EXPIRE;
                    expiration.Start = startDate;
                    expiration.End = endDate;
                    break;
            }
            return expiration;
        }
        #endregion

        /// <summary>
        /// WinFormControlLibrary.Rights type 
        /// convert to 
        /// CustomControls.pages.DigitalRights.model.FileRights type
        /// </summary>
        /// <param name="filerights"></param>
        /// <returns></returns>
        internal static HashSet<FileRights> Rights2DigitalRights(HashSet<Rights> rights)
        {
            HashSet<FileRights> filerights = new HashSet<FileRights>();
            foreach (var item in rights)
            {
                switch (item)
                {
                    case Rights.RIGHT_VIEW:
                        filerights.Add(FileRights.RIGHT_VIEW);
                        break;
                    case Rights.RIGHT_EDIT:
                        filerights.Add(FileRights.RIGHT_EDIT);
                        break;
                    case Rights.RIGHT_PRINT:
                        filerights.Add(FileRights.RIGHT_PRINT);
                        break;
                    case Rights.RIGHT_CLIPBOARD:
                        filerights.Add(FileRights.RIGHT_CLIPBOARD);
                        break;
                    case Rights.RIGHT_SAVEAS:
                        filerights.Add(FileRights.RIGHT_SAVEAS);
                        break;
                    case Rights.RIGHT_DECRYPT:
                        filerights.Add(FileRights.RIGHT_DECRYPT);
                        break;
                    case Rights.RIGHT_SCREENCAPTURE:
                        filerights.Add(FileRights.RIGHT_SCREENCAPTURE);
                        break;
                    case Rights.RIGHT_SEND:
                        filerights.Add(FileRights.RIGHT_SEND);
                        break;
                    case Rights.RIGHT_CLASSIFY:
                        filerights.Add(FileRights.RIGHT_CLASSIFY);
                        break;
                    case Rights.RIGHT_SHARE:
                        filerights.Add(FileRights.RIGHT_SHARE);
                        break;
                    case Rights.RIGHT_DOWNLOAD:
                        filerights.Add(FileRights.RIGHT_DOWNLOAD);
                        break;
                    case Rights.RIGHT_WATERMARK:
                        filerights.Add(FileRights.RIGHT_WATERMARK);
                        break;
                    case Rights.RIGHT_VALIDITY:
                        filerights.Add(FileRights.RIGHT_VALIDITY);
                        break;
                }
            }
            return filerights;
        }

        /// <summary>
        ///  CustomControls.pages.DigitalRights.model.FileRights type
        ///  convert to 
        ///  WinFormControlLibrary.Rights type
        /// </summary>
        /// <param name="fileRights"></param>
        /// <returns></returns>
        internal static HashSet<Rights> DigitalRights2Rights(HashSet<FileRights> fileRights)
        {
            HashSet<Rights> rights = new HashSet<Rights>();
            foreach (var item in fileRights)
            {
                switch (item)
                {
                    case FileRights.RIGHT_VIEW:
                        rights.Add(Rights.RIGHT_VIEW);
                        break;
                    case FileRights.RIGHT_EDIT:
                        rights.Add(Rights.RIGHT_EDIT);
                        break;
                    case FileRights.RIGHT_PRINT:
                        rights.Add(Rights.RIGHT_PRINT);
                        break;
                    case FileRights.RIGHT_CLIPBOARD:
                        rights.Add(Rights.RIGHT_CLIPBOARD);
                        break;
                    case FileRights.RIGHT_SAVEAS:
                        rights.Add(Rights.RIGHT_SAVEAS);
                        break;
                    case FileRights.RIGHT_DECRYPT:
                        rights.Add(Rights.RIGHT_DECRYPT);
                        break;
                    case FileRights.RIGHT_SCREENCAPTURE:
                        rights.Add(Rights.RIGHT_SCREENCAPTURE);
                        break;
                    case FileRights.RIGHT_SEND:
                        rights.Add(Rights.RIGHT_SEND);
                        break;
                    case FileRights.RIGHT_CLASSIFY:
                        rights.Add(Rights.RIGHT_CLASSIFY);
                        break;
                    case FileRights.RIGHT_SHARE:
                        rights.Add(Rights.RIGHT_SHARE);
                        break;
                    case FileRights.RIGHT_DOWNLOAD:
                        rights.Add(Rights.RIGHT_DOWNLOAD);
                        break;
                    case FileRights.RIGHT_WATERMARK:
                        rights.Add(Rights.RIGHT_WATERMARK);
                        break;
                    //case FileRights.RIGHT_VALIDITY:
                    //    rights.Add(Rights.RIGHT_VALIDITY);
                    //    break;
                }
            }
            return rights;
        }

        /// <summary>
        /// WinFormControlLibrary.Classification type 
        /// convert to 
        /// CustomControls.pages.CentralPolicy.model.ProjectClassification type
        /// </summary>
        /// <param name="classifications"></param>
        /// <returns></returns>
        internal static CustomControls.components.CentralPolicy.model.Classification[] Classifications2CentralPolicyModel(Classification[] classifications)
        {
            CustomControls.components.CentralPolicy.model.Classification[] tags = new CustomControls.components.CentralPolicy.model.Classification[classifications.Length];
            for (int i = 0; i < classifications.Length; i++)
            {
                tags[i].name = classifications[i].name;
                tags[i].isMultiSelect = classifications[i].isMultiSelect;
                tags[i].isMandatory = classifications[i].isMandatory;
                tags[i].labels = classifications[i].labels;
            }
            return tags;
        }

        /// <summary>
        /// WinFormControlLibrary.Rights type 
        /// convert to 
        /// CustomControls.components.RightsDisplay.model.RightsItem type
        /// </summary>
        /// <param name="rights"></param>
        /// <returns></returns>
        internal static ObservableCollection<RightsItem> Rights2RightsDisplayModel(HashSet<Rights> rights)
        {
            ObservableCollection<RightsItem> rightsItems = new ObservableCollection<RightsItem>();
            foreach (var item in rights)
            {
                switch (item)
                {
                    case Rights.RIGHT_VIEW:
                        rightsItems.Add(new RightsItem(GDIToWpfBitmap(Properties.Resources.icon_rights_view),"View"));
                        break;
                    case Rights.RIGHT_EDIT:
                        rightsItems.Add(new RightsItem(GDIToWpfBitmap(Properties.Resources.icon_rights_edit), "Edit"));
                        break;
                    case Rights.RIGHT_PRINT:
                        rightsItems.Add(new RightsItem(GDIToWpfBitmap(Properties.Resources.icon_rights_print), "Print"));
                        break;
                    case Rights.RIGHT_CLIPBOARD:
                        break;
                    case Rights.RIGHT_SAVEAS:
                        rightsItems.Add(new RightsItem(GDIToWpfBitmap(Properties.Resources.icon_rights_save_as), "Save As"));
                        break;
                    case Rights.RIGHT_DECRYPT:
                        rightsItems.Add(new RightsItem(GDIToWpfBitmap(Properties.Resources.icon_rights_extract), "Extract"));
                        break;
                    case Rights.RIGHT_SCREENCAPTURE:
                        break;
                    case Rights.RIGHT_SEND:
                        break;
                    case Rights.RIGHT_CLASSIFY:
                        break;
                    case Rights.RIGHT_SHARE:
                        rightsItems.Add(new RightsItem(GDIToWpfBitmap(Properties.Resources.icon_rights_share), "Re-share"));
                        break;
                    case Rights.RIGHT_DOWNLOAD:
                        break;
                    case Rights.RIGHT_WATERMARK:
                        rightsItems.Add(new RightsItem(GDIToWpfBitmap(Properties.Resources.icon_rights_watermark), "Watermark"));
                        break;
                    case Rights.RIGHT_VALIDITY:
                        rightsItems.Add(new RightsItem(GDIToWpfBitmap(Properties.Resources.icon_rights_validity), "Validity"));
                        break;
                }
            }
            return rightsItems;
        }

    }
}
