using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Globalization;
using System.Linq;
using System.Text;
using System.Windows;
using System.Windows.Data;
using CustomControls.common.helper;
using CustomControls.components.DigitalRights.model;
using static CustomControls.windows.fileInfo.viewModel.FileInfoWindowViewModel;

namespace CustomControls.windows.fileInfo.helper
{
        public class LocalFileRights2ResouceConverter : IValueConverter
        {
            public object Convert(object value, Type targetType, object parameter, CultureInfo culture)
            {
                FileRights fileRights = (FileRights)value;
                switch (fileRights)
                {
                    case FileRights.RIGHT_VIEW:
                        return @"/CustomControls;component/resources/icons/rights_view.png";

                    case FileRights.RIGHT_SHARE:
                        return @"/CustomControls;component/resources/icons/rights_share.png";

                    case FileRights.RIGHT_PRINT:
                        return @"/CustomControls;component/resources/icons/rights_print.png";

                    case FileRights.RIGHT_DOWNLOAD:
                        return @"/CustomControls;component/resources/icons/rights_save_as.png";

                    case FileRights.RIGHT_WATERMARK:
                        return @"/CustomControls;component/resources/icons/rights_watermark.png";

                    case FileRights.RIGHT_VALIDITY:
                        return @"/CustomControls;component/resources/icons/rights_validity.png";

                    case FileRights.RIGHT_EDIT:
                        return @"/CustomControls;component/resources/icons/rights_edit.png";

                    case FileRights.RIGHT_SAVEAS:
                        return @"/CustomControls;component/resources/icons/rights_save_as.png";

                    case FileRights.RIGHT_DECRYPT:
                        return @"/CustomControls;component/resources/icons/rights_extract2.png";
                    default:
                        return "";
                      //  return @"/CustomControls;component/resources/icons/Icon_access_denied.png";
                }
            }

            public object ConvertBack(object value, Type targetType, object parameter, CultureInfo culture)
            {
                throw new NotImplementedException();
            }
        }

        public class DisplayWaterMark2DisplayWaterMarkVisibilityConverter : IValueConverter
        {
            public object Convert(object value, Type targetType, object parameter, CultureInfo culture)
            {
                string s = (string)value;
                if (string.IsNullOrEmpty(s))
                {
                    return Visibility.Collapsed;
                }
                else
                {
                    return Visibility.Visible;
                }
            }

            public object ConvertBack(object value, Type targetType, object parameter, CultureInfo culture)
            {
                throw new NotImplementedException();
            }
        }

        public class ListCount2BoolConverter : IValueConverter
        {
            public object Convert(object value, Type targetType, object parameter, CultureInfo culture)
            {
                return (int)value > 0 ? Visibility.Visible : Visibility.Collapsed;
            }

            public object ConvertBack(object value, Type targetType, object parameter, CultureInfo culture)
            {
                throw new NotImplementedException();
            }
        }

        public class ForegroundConverter : IValueConverter
        {
            public object Convert(object value, Type targetType, object parameter, CultureInfo culture)
            {
                string displayExpiration = Utils.FormatExpiration((Expiration)value);
                if (string.Equals(displayExpiration, "Expired", StringComparison.CurrentCultureIgnoreCase))
                {
                    return @"#EB5757";
                }
                return @"Gray";
            }

            public object ConvertBack(object value, Type targetType, object parameter, CultureInfo culture)
            {
                throw new NotImplementedException();
            }
        }

        public class NameToBackground : IValueConverter
        {
            public object Convert(object value, Type targetType, object parameter, CultureInfo culture)
            {
                return NameColorHelper.SelectionBackgroundColor(value.ToString());

            }

            public object ConvertBack(object value, Type targetType, object parameter, CultureInfo culture)
            {
                throw new NotImplementedException();
            }
        }

        public class NameToForeground : IValueConverter
        {
            public object Convert(object value, Type targetType, object parameter, CultureInfo culture)
            {
                return NameColorHelper.SelectionTextColor(value.ToString());
            }

            public object ConvertBack(object value, Type targetType, object parameter, CultureInfo culture)
            {
                throw new NotImplementedException();
            }
        }

        public class CheckoutFirstChar : IValueConverter
        {
            public object Convert(object value, Type targetType, object parameter, CultureInfo culture)
            {
                if (string.IsNullOrEmpty(value.ToString()))
                {
                    return "";
                }
                else
                {
                    return value.ToString().Substring(0, 1).ToUpper();
                }
            }

            public object ConvertBack(object value, Type targetType, object parameter, CultureInfo culture)
            {
                throw new NotImplementedException();
            }
        }

        public class ValidityHidenProperty2ValidityVisiblitiyConverter : IValueConverter
        {
            public object Convert(object value, Type targetType, object parameter, CultureInfo culture)
            {
                bool hiden = (bool)value;
                return hiden ? Visibility.Collapsed : Visibility.Visible;
            }

            public object ConvertBack(object value, Type targetType, object parameter, CultureInfo culture)
            {
                throw new NotImplementedException();
            }
        }

        public class ShareWithCount2StringConverter : IValueConverter
        {
            public object Convert(object value, Type targetType, object parameter, CultureInfo culture)
            {
                int count = 0;
                count = (int)value;
                string result;
                if (count > 1)
                {
                     result = "members";
                }
                else
                {
                     result = "member";
                }
                return result;
            }

            public object ConvertBack(object value, Type targetType, object parameter, CultureInfo culture)
            {
                throw new NotImplementedException();
            }
        }

        public class OriginalFileVisibilityConverter : IValueConverter
        {
            public object Convert(object value, Type targetType, object parameter, CultureInfo culture)
            {
                Visibility result = Visibility.Visible;
                EventSource eventSource = (EventSource)value;
                switch (eventSource)
                {
                    case EventSource.NormalFolder:
                         result = Visibility.Hidden;
                    break;
                    case EventSource.NxrmApp:
                         result = Visibility.Visible;
                    break;
                }
                return result;
            }

            public object ConvertBack(object value, Type targetType, object parameter, CultureInfo culture)
            {
                throw new NotImplementedException();
            }
        }

        public class LastModifyDateVisibilityConverter : IValueConverter
        {
            public object Convert(object value, Type targetType, object parameter, CultureInfo culture)
            {
                try
                {
                    string date = value.ToString();
                    DateTime result;
                    if (string.IsNullOrEmpty(date) || date.Equals("UnKnown") || !DateTime.TryParse(date, out result))
                    {
                        return Visibility.Hidden;
                    }
                    return Visibility.Visible;
                }
                catch (Exception e)
                {
                    Console.WriteLine(e);
                }
                return Visibility.Hidden;
            }

            public object ConvertBack(object value, Type targetType, object parameter, CultureInfo culture)
            {
                throw new NotImplementedException();
            }
        }

        public class ShareWithStringConverter : IValueConverter
        {
            public object Convert(object value, Type targetType, object parameter, CultureInfo culture)
            {
                string result = null;
                try
                {
                    FileMetadate fileMetadate = (FileMetadate)value;
                    switch (fileMetadate)
                    {
                        case FileMetadate.isFromMyVault:
                        case FileMetadate.isFromPorject:
                        case FileMetadate.isFromSystemBucket:
                        case FileMetadate.isFromWorkSpace:
                            result = "Shared with";
                            break;

                        case FileMetadate.isFromShareWithMe:
                            result = "Shared by";
                            break;
                    }
                }
                catch (Exception e)
                {
                    Console.WriteLine(e);
                }
                return result;
            }

            public object ConvertBack(object value, Type targetType, object parameter, CultureInfo culture)
            {
                throw new NotImplementedException();
            }
        }


        public class EmailUiVisibleConverter : IValueConverter
        {
            public object Convert(object value, Type targetType, object parameter, CultureInfo culture)
            {
                Visibility result = Visibility.Collapsed;
                try
                {
                    FileMetadate fileMetadate = (FileMetadate)value;
                    switch (fileMetadate)
                    {
                        case FileMetadate.isFromMyVault:
                        case FileMetadate.isFromPorject:
                        case FileMetadate.isFromSystemBucket:
                        case FileMetadate.isFromWorkSpace:
                            result = Visibility.Visible;
                        break;

                        case FileMetadate.isFromShareWithMe:
                            result = Visibility.Collapsed;
                        break;
                    }
                }
                catch (Exception e)
                {
                    Console.WriteLine(e);
                }
                return result;
            }

            public object ConvertBack(object value, Type targetType, object parameter, CultureInfo culture)
            {
                throw new NotImplementedException();
            }
        }


    public class IsByCentrolPolicy2Visible : IValueConverter
        {
            public object Convert(object value, Type targetType, object parameter, CultureInfo culture)
            {
                bool hiden = (bool)value;
                return hiden ? Visibility.Visible : Visibility.Collapsed;
            }

            public object ConvertBack(object value, Type targetType, object parameter, CultureInfo culture)
            {
                throw new NotImplementedException();
            }
        }

    public class ProcessFileRights : IValueConverter
    {
        public object Convert(object value, Type targetType, object parameter, CultureInfo culture)
        {
            ObservableCollection<FileRights> rights = (ObservableCollection<FileRights>)value;
            rights.Remove(FileRights.RIGHT_CLIPBOARD);
            rights.Remove(FileRights.RIGHT_SCREENCAPTURE);
            rights.Remove(FileRights.RIGHT_SEND);
            rights.Remove(FileRights.RIGHT_CLASSIFY);
            return rights;
        }

        public object ConvertBack(object value, Type targetType, object parameter, CultureInfo culture)
        {
            throw new NotImplementedException();
        }
    }

    public class FileRights2Visible : IValueConverter
    {
        public object Convert(object value, Type targetType, object parameter, CultureInfo culture)
        {
            return (int)value < 1 ? Visibility.Visible : Visibility.Collapsed;
        }

        public object ConvertBack(object value, Type targetType, object parameter, CultureInfo culture)
        {
            throw new NotImplementedException();
        }
    }

    public class FormatFileSize : IValueConverter
    {
        private String Format(Int64 fileSize)
        {
            if (fileSize < 0)
            {
                throw new ArgumentOutOfRangeException("fileSize");
            }
            else if (fileSize >= 1024 * 1024 * 1024)
            {
                return string.Format("{0:########0.00} GB", ((Double)fileSize) / (1024 * 1024 * 1024));
            }
            else if (fileSize >= 1024 * 1024)
            {
                return string.Format("{0:####0.00} MB", ((Double)fileSize) / (1024 * 1024));
            }
            else if (fileSize >= 1024)
            {
                return string.Format("{0:####0.00} KB", ((Double)fileSize) / 1024);
            }
            else
            {
                return string.Format("{0} bytes", fileSize);
            }
        }

        public object Convert(object value, Type targetType, object parameter, CultureInfo culture)
        {
            return Format((Int64)value);
        }

        public object ConvertBack(object value, Type targetType, object parameter, CultureInfo culture)
        {
            throw new NotImplementedException();
        }
    }


    public class FormatExpiration : IValueConverter
    {
        public object Convert(object value, Type targetType, object parameter, CultureInfo culture)
        {
            string result = string.Empty;
            result = Utils.FormatExpiration((Expiration)value);
            return result;
        }

        public object ConvertBack(object value, Type targetType, object parameter, CultureInfo culture)
        {
            throw new NotImplementedException();
        }
    }

}
