using ServiceManager.resources.languages;
using ServiceManager.rmservmgr.app.recentNotification;
using ServiceManager.rmservmgr.common.helper;
using System;
using System.Collections.Generic;
using System.Globalization;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Data;
using System.Windows.Media.Imaging;

namespace ServiceManager.rmservmgr.ui.windows.serviceManager.helper
{
    /// <summary>
    /// Using to Converter network status(bool type) to Image flag
    /// </summary>
    public class NetworkStatusBool2ShortLineImageConverter : IValueConverter
    {
        public object Convert(object value, Type targetType, object parameter, CultureInfo culture)
        {
            if ((bool)value)
            {
                return @"/resources/icons/online_short_line.png";
            }
            else
            {
                return @"/resources/icons/offline_short_line.png";
            }
        }
        public object ConvertBack(object value, Type targetType, object parameter, CultureInfo culture)
        {
            throw new NotImplementedException();
        }
    }
    /// <summary>
    /// Using to Converter network status(bool type) to Image flag
    /// </summary>
    public class NetworkStatusBool2LongLineImageConverter : IValueConverter
    {
        public object Convert(object value, Type targetType, object parameter, CultureInfo culture)
        {
            if ((bool)value)
            {
                return @"/resources/icons/online_long_line.png";
            }
            else
            {
                return @"/resources/icons/offline_long_line.png";
            }
        }
        public object ConvertBack(object value, Type targetType, object parameter, CultureInfo culture)
        {
            throw new NotImplementedException();
        }
    }

    public class NetworkStatusBool2StringForeground : IValueConverter
    {
        public object Convert(object value, Type targetType, object parameter, CultureInfo culture)
        {
            if ((bool)value)
            {
                return "Green";
            }
            else
            {
                return "Red";
            }
        }

        public object ConvertBack(object value, Type targetType, object parameter, CultureInfo culture)
        {
            throw new NotImplementedException();
        }
    }

    /// <summary>
    /// Using to Converter network status(bool type) to prompt info(string type)
    /// </summary>
    public class NetworkStatusBool2StringInfo : IValueConverter
    {
        public object Convert(object value, Type targetType, object parameter, CultureInfo culture)
        {
            if ((bool)value)
            {
                return CultureStringInfo.ServiceManageWin_Status_Online;
            }
            else
            {
                return CultureStringInfo.ServiceManageWin_Status_Offline;
            }
        }

        public object ConvertBack(object value, Type targetType, object parameter, CultureInfo culture)
        {
            throw new NotImplementedException();
        }
    }

    public class CheckAllVisibilityConvert : IValueConverter
    {
        public object Convert(object value, Type targetType, object parameter, CultureInfo culture)
        {
            int count = (int)value;
            if (count > 0)
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

    public class NoMatchTextBlockVisibilityConvert : IValueConverter
    {
        public object Convert(object value, Type targetType, object parameter, CultureInfo culture)
        {
            int count = (int)value;
            if (count > 0)
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

    public class FilterOrClearBtnIsEnableConvert : IValueConverter
    {
        public object Convert(object value, Type targetType, object parameter, CultureInfo culture)
        {
            int count = (int)value;
            if (count > 0)
            {
                return true;
            }
            return false;
        }

        public object ConvertBack(object value, Type targetType, object parameter, CultureInfo culture)
        {
            throw new NotImplementedException();
        }
    }

    public class FilterOkBtnIsEnable : IValueConverter
    {
        public object Convert(object value, Type targetType, object parameter, CultureInfo culture)
        {
            bool? temp = (bool?)value;
            if (temp == null || temp == true)
            {
                return true;
            }
            else
            {
                return false;
            }
        }

        public object ConvertBack(object value, Type targetType, object parameter, CultureInfo culture)
        {
            throw new NotImplementedException();
        }
    }

    public class NotifyListFileStatusIconConvert : IMultiValueConverter
    {
        public object Convert(object[] value, Type targetType, object parameter, CultureInfo culture)
        {
            try
            {
                // now nxrmtray will not display file status icon.
                return null;

                bool result = (bool)value[0];
                int fileStatus = (int)value[1];

                if (!result)
                {
                    return null;
                }

                // check para
                CommonUtils.FileStatusEnumTryParse(fileStatus, out FileStatus fileStatusEnum);

                switch (fileStatusEnum)
                {
                    case FileStatus.Offline:
                        return new BitmapImage(new Uri(@"/resources/icons/fileTypeStatus/offline.png", UriKind.Relative));
                    case FileStatus.WaitingUpload:
                        return new BitmapImage(new Uri(@"/resources/icons/fileTypeStatus/waitupload.png", UriKind.Relative));
                    default:
                        return null;
                }
            }
            catch (Exception e)
            {
                Console.WriteLine(e.ToString());

                return null;
            }

        }

        public object[] ConvertBack(object value, Type[] targetType, object parameter, CultureInfo culture)
        {
            throw new NotImplementedException();
        }
    }
    public class NotifyListFileIconConvert : IMultiValueConverter
    {
        public object Convert(object[] value, Type targetType, object parameter, CultureInfo culture)
        {
            try
            {
                string target = (string)value[0];
                bool result = (bool)value[1];

                if (!result)
                {
                    return new BitmapImage(new Uri(@"/resources/icons/warn.png", UriKind.Relative));
                }

                bool isNxl = target.EndsWith(".nxl");
                string originFilename = target;
                if (isNxl)
                {
                    int lastindex = target.LastIndexOf('.');
                    if (lastindex != -1)
                    {
                        originFilename = target.Substring(0, lastindex); // remove .nxl
                    }
                }

                string fileType = System.IO.Path.GetExtension(originFilename); // return .txt or null or empty
                if (string.IsNullOrEmpty(fileType))
                {
                    fileType = "---";
                }
                else if (fileType.IndexOf('.') != -1)
                {
                    fileType = fileType.Substring(fileType.IndexOf('.') + 1).ToLower();
                    if (!CommonUtils.IsSupportFileTypeEx(fileType))
                    {
                        fileType = "---";
                    }
                }
                else
                {
                    fileType = "---";
                }

                string uritemp = "";
                if (isNxl)
                {
                    uritemp = string.Format(@"/resources/icons/fileTypeIcons/{0}_G.png", fileType.ToUpper());
                }
                else
                {
                    uritemp = string.Format(@"/resources/icons/fileTypeIcons/{0}.png", fileType.ToUpper());
                }
                var stream = new Uri(uritemp, UriKind.Relative);
                return new BitmapImage(stream);
            }
            catch (Exception e)
            {
                Console.WriteLine(e.ToString());

                return new BitmapImage(new Uri(@"/resources/icons/fileTypeIcons/---_G.png", UriKind.Relative));
            }

        }

        public object[] ConvertBack(object value, Type[] targetType, object parameter, CultureInfo culture)
        {
            throw new NotImplementedException();
        }
    }

    public class NotifyListAppTextLongConvert : IValueConverter
    {
        public object Convert(object value, Type targetType, object parameter, CultureInfo culture)
        {
            string application = (string)value;
            if (application.Length > 10)
            {
                application = application.Substring(0, 10) + "...";
            }
            return application;
        }

        public object ConvertBack(object value, Type targetType, object parameter, CultureInfo culture)
        {
            throw new NotImplementedException();
        }
    }

    public class DateTimeConvert : IValueConverter
    {
        private string DateStringFromNow(DateTime dt)
        {
            string result = string.Empty;

            TimeSpan span = DateTime.Now - dt;

            if (span.TotalDays > 60)
            {
                result = dt.ToShortDateString();
            }
            else if (span.TotalDays > 30)
            {
                result = CultureStringInfo.ServiceManageWin_One_Month;
            }
            else if (span.TotalDays > 14)
            {
                result = CultureStringInfo.ServiceManageWin_Two_Weeks;
            }
            else if (span.TotalDays > 7)
            {
                result = CultureStringInfo.ServiceManageWin_One_Week;
            }

            else if (span.TotalDays >= 1 && span.TotalDays < 2)
            {
                result = string.Format("{0} {1}",
                (int)Math.Floor(span.TotalDays), CultureStringInfo.ServiceManageWin_Day_Ago);
            }
            else if (span.TotalDays >= 2)
            {
                result = string.Format("{0} {1}",
                (int)Math.Floor(span.TotalDays), CultureStringInfo.ServiceManageWin_Days_Ago);
            }
            else if (span.TotalHours >= 1 && span.TotalHours < 2)
            {
                result = string.Format("{0} {1} ", (int)Math.Floor(span.TotalHours), CultureStringInfo.ServiceManageWin_Hour_Ago);
            }
            else if (span.TotalHours >= 2)
            {
                result = string.Format("{0} {1} ", (int)Math.Floor(span.TotalHours), CultureStringInfo.ServiceManageWin_Hours_Ago);
            }
            else if (span.TotalMinutes >= 2)
            {
                result = string.Format("{0} {1}", (int)Math.Floor(span.TotalMinutes), CultureStringInfo.ServiceManageWin_Minutes_Ago);
            }
            else if (span.TotalMinutes >= 1 && span.TotalMinutes < 2)
            {
                result = string.Format("{0} {1}", (int)Math.Floor(span.TotalMinutes), CultureStringInfo.ServiceManageWin_Minute_Ago);
            }
           
            else if (span.TotalMilliseconds >= 0)
            {
                result = CultureStringInfo.ServiceManageWin_Just_Now;
            }

            return result;
        }
        public object Convert(object value, Type targetType, object parameter, CultureInfo culture)
        {
            DateTime date = (DateTime)value;
            return DateStringFromNow(date);
        }

        public object ConvertBack(object value, Type targetType, object parameter, CultureInfo culture)
        {
            throw new NotImplementedException();
        }
    }

}
