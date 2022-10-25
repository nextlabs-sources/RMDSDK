using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Globalization;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Data;
using System.Windows.Media;

namespace CustomControls.pages.Share
{
    public class TextForegroundStatusConverter : IValueConverter
    {
        public object Convert(object value, Type targetType, object parameter, CultureInfo culture)
        {
            EmailStatus status = (EmailStatus)value;
            switch (status)
            {
                case EmailStatus.DIRTY:
                    return @"White";
                case EmailStatus.NORMAL:
                    return @"Black";
                default:
                    return null;
            }
        }

        public object ConvertBack(object value, Type targetType, object parameter, CultureInfo culture)
        {
            throw new NotImplementedException();
        }
    }

    public class BackgroundStatusConverter : IValueConverter
    {
        public object Convert(object value, Type targetType, object parameter, CultureInfo culture)
        {
            EmailStatus status = (EmailStatus)value;
            switch (status)
            {
                case EmailStatus.NORMAL:
                    return @"#E5E5E5";
                case EmailStatus.DIRTY:
                    return @"RED";
                default:
                    return null;
            }
        }

        public object ConvertBack(object value, Type targetType, object parameter, CultureInfo culture)
        {
            throw new NotImplementedException();
        }
    }

    public class WatermarkVisibleConverter : IValueConverter
    {
        public object Convert(object value, Type targetType, object parameter, CultureInfo culture)
        {
            Visibility visibility = Visibility.Collapsed;

            ObservableCollection<RightsItem> rightsItems = (ObservableCollection<RightsItem>)value;
            foreach (var item in rightsItems)
            {
                if (item.Rights.Equals("Watermark"))
                {
                    visibility = Visibility.Visible;
                }
            }

            return visibility;
        }

        public object ConvertBack(object value, Type targetType, object parameter, CultureInfo culture)
        {
            throw new NotImplementedException();
        }
    }

    public class TipInfoVisibleConverter : IValueConverter
    {
        public object Convert(object value, Type targetType, object parameter, CultureInfo culture)
        {
            Visibility visibility = Visibility.Collapsed;
            string currentTextLength = (string)value;
            int maxLength = 250;
            int remainingLength = maxLength - currentTextLength.Length;

            if (remainingLength < 0)
            {
                visibility = Visibility.Visible;
            }
            else
            {
                visibility = Visibility.Collapsed;
            }

            return visibility;
        }

        public object ConvertBack(object value, Type targetType, object parameter, CultureInfo culture)
        {
            throw new NotImplementedException();
        }
    }

    public class RemaingLengthForegroundConverter : IValueConverter
    {
        public object Convert(object value, Type targetType, object parameter, CultureInfo culture)
        {
            string currentText = (string)value;
            int maxLength = 250;
          
            System.Windows.Media.Brush foreground = new SolidColorBrush(Colors.Black);

            if ((maxLength - currentText.Length) < 0)
            {
                foreground = new SolidColorBrush(Colors.Red);
            }
            else
            {
                foreground = new SolidColorBrush(Colors.Black);
            }

            return foreground;
        }

        public object ConvertBack(object value, Type targetType, object parameter, CultureInfo culture)
        {
            throw new NotImplementedException();
        }
    }

    public class ShareTextForegroundConverter : IValueConverter
    {
        public object Convert(object value, Type targetType, object parameter, CultureInfo culture)
        {
            Int32 status = (Int32)value;
            System.Windows.Media.Brush result = new SolidColorBrush(Colors.Gray);

            if ((status & ShareStatus.RESHARE_SUCCEEDED) == ShareStatus.RESHARE_SUCCEEDED)
            {
                result = new SolidColorBrush(System.Windows.Media.Color.FromRgb(0X2F, 0X80, 0XED));
            }
            else if ((status & ShareStatus.SHARE_MY_VAULT_FILE_SUCCEEDED) == ShareStatus.SHARE_MY_VAULT_FILE_SUCCEEDED)
            {
                result = new SolidColorBrush(System.Windows.Media.Color.FromRgb(0X2F, 0X80, 0XED));
            }
            else if ((status & ShareStatus.SHARE_PROJECT_FILE_SUCCEEDED) == ShareStatus.SHARE_PROJECT_FILE_SUCCEEDED)
            {
                result = new SolidColorBrush(System.Windows.Media.Color.FromRgb(0X2F, 0X80, 0XED));
            }
            else if ((status & ShareStatus.EXPIRED) == ShareStatus.EXPIRED)
            {
                result = new SolidColorBrush(System.Windows.Media.Color.FromRgb(0XFF, 0X00, 0X00));
            }

            return result;
        }

        public object ConvertBack(object value, Type targetType, object parameter, CultureInfo culture)
        {
            throw new NotImplementedException();
        }
    }

    public class ProBarVisibleConverter : IValueConverter
    {
        public object Convert(object value, Type targetType, object parameter, CultureInfo culture)
        {
            Visibility result = Visibility.Collapsed;
            Int32 status = (Int32)value;

            if ((status & ShareStatus.PROGRESS_BAR) == ShareStatus.PROGRESS_BAR)
            {
                result = Visibility.Visible;
            }

            return result;
        }

        public object ConvertBack(object value, Type targetType, object parameter, CultureInfo culture)
        {
            throw new NotImplementedException();
        }
    }

    public class DefDescriptionVisibleConverter : IValueConverter
    {
        public object Convert(object value, Type targetType, object parameter, CultureInfo culture)
        {
            Visibility result = Visibility.Visible;
            Int32  status = (Int32)value;

            if ((status & ShareStatus.EXPIRED) == ShareStatus.EXPIRED)
            {
                result = Visibility.Collapsed;
            }  
            else if ((status & ShareStatus.RESHARE_SUCCEEDED) == ShareStatus.RESHARE_SUCCEEDED)
            {
                result = Visibility.Collapsed;
            }
            else if ((status & ShareStatus.SHARE_MY_VAULT_FILE_SUCCEEDED) == ShareStatus.SHARE_MY_VAULT_FILE_SUCCEEDED)
            {
                result = Visibility.Collapsed;
            }
            else if ((status & ShareStatus.SHARE_PROJECT_FILE_SUCCEEDED) == ShareStatus.SHARE_PROJECT_FILE_SUCCEEDED)
            {
                result = Visibility.Collapsed;
            }
            return result;
        }

        public object ConvertBack(object value, Type targetType, object parameter, CultureInfo culture)
        {
            throw new NotImplementedException();
        }
    }

    public class DescriptionVisibleConverter : IValueConverter
    {
        public object Convert(object value, Type targetType, object parameter, CultureInfo culture)
        {
            Visibility result = Visibility.Collapsed;
            Int32 status = (Int32)value;

            if ((status & ShareStatus.EXPIRED) == ShareStatus.EXPIRED)
            {
                result = Visibility.Visible;
            }
            else if ((status & ShareStatus.RESHARE_SUCCEEDED) == ShareStatus.RESHARE_SUCCEEDED)
            {
                result = Visibility.Visible;
            }
            else if ((status & ShareStatus.SHARE_MY_VAULT_FILE_SUCCEEDED) == ShareStatus.SHARE_MY_VAULT_FILE_SUCCEEDED)
            {
                result = Visibility.Visible;
            }
            else if ((status & ShareStatus.SHARE_PROJECT_FILE_SUCCEEDED) == ShareStatus.SHARE_PROJECT_FILE_SUCCEEDED)
            {
                result = Visibility.Visible;
            }
            return result;
        }

        public object ConvertBack(object value, Type targetType, object parameter, CultureInfo culture)
        {
            throw new NotImplementedException();
        }
    }

    public class DescriptionTextConverter : IValueConverter
    {
        public object Convert(object value, Type targetType, object parameter, CultureInfo culture)
        {
            string result = string.Empty;
            Int32 status = (Int32)value;

            if ((status & ShareStatus.EXPIRED) == ShareStatus.EXPIRED)
            {
                result = "Expired";
            }
            else if ((status & ShareStatus.RESHARE_SUCCEEDED) == ShareStatus.RESHARE_SUCCEEDED)
            {
                result = "Successfully shared";
            }
            else if ((status & ShareStatus.SHARE_MY_VAULT_FILE_SUCCEEDED) == ShareStatus.SHARE_MY_VAULT_FILE_SUCCEEDED)
            {
                result = "Successfully shared";
            }
            else if ((status & ShareStatus.SHARE_PROJECT_FILE_SUCCEEDED) == ShareStatus.SHARE_PROJECT_FILE_SUCCEEDED)
            {
                result = "Successfully shared";
            }

            return result;
        }

        public object ConvertBack(object value, Type targetType, object parameter, CultureInfo culture)
        {
            throw new NotImplementedException();
        }
    }

    public class ShareButtonEnabled : IValueConverter
    {
        public object Convert(object value, Type targetType, object parameter, CultureInfo culture)
        {
            bool result = false;
            Int32 status = (Int32)value;
            if ((status & ShareStatus.SHARE_BUTTON_ENABLED) == ShareStatus.SHARE_BUTTON_ENABLED)
            {
                result = true;
            }
            return result;
        }

        public object ConvertBack(object value, Type targetType, object parameter, CultureInfo culture)
        {
            throw new NotImplementedException();
        }
    }


    public class CalculateRemaingLengthByCurrentText : IValueConverter
    {
        public object Convert(object value, Type targetType, object parameter, CultureInfo culture)
        {
            string defResult = "250";
            string currentText = (string)value;
            int maxLength = 250;
            if ((currentText.Length) <= 0)
            {
                return defResult;
            }
            else
            {
               return (maxLength - currentText.Length).ToString();
            }
        }

        public object ConvertBack(object value, Type targetType, object parameter, CultureInfo culture)
        {
            throw new NotImplementedException();
        }
    }

    public class CloseBtnVisibleConverter : IValueConverter
    {
        public object Convert(object value, Type targetType, object parameter, CultureInfo culture)
        {
            Visibility result = Visibility.Collapsed;
            Int32 status = (Int32)value;

            if ((status & ShareStatus.EXPIRED) == ShareStatus.EXPIRED)
            {
                result = Visibility.Visible;
            }
            else if ((status & ShareStatus.RESHARE_SUCCEEDED) == ShareStatus.RESHARE_SUCCEEDED)
            {
                result = Visibility.Visible;
            }
            else if ((status & ShareStatus.SHARE_MY_VAULT_FILE_SUCCEEDED) == ShareStatus.SHARE_MY_VAULT_FILE_SUCCEEDED)
            {
                result = Visibility.Visible;
            }
            else if ((status & ShareStatus.SHARE_PROJECT_FILE_SUCCEEDED) == ShareStatus.SHARE_PROJECT_FILE_SUCCEEDED)
            {
                result = Visibility.Visible;
            }
            else if ((status & ShareStatus.ERROR) == ShareStatus.ERROR)
            {
                result = Visibility.Visible;
            }
            return result;
        }

        public object ConvertBack(object value, Type targetType, object parameter, CultureInfo culture)
        {
            throw new NotImplementedException();
        }
    }

    public class ShareBtnVisibleConverter : IValueConverter
    {
        public object Convert(object value, Type targetType, object parameter, CultureInfo culture)
        {
            Visibility result = Visibility.Visible;
            Int32 status = (Int32)value;

            if ((status & ShareStatus.RESHARE_SUCCEEDED) == ShareStatus.RESHARE_SUCCEEDED)
            {
                result = Visibility.Collapsed;
            }
            else if ((status & ShareStatus.SHARE_MY_VAULT_FILE_SUCCEEDED) == ShareStatus.SHARE_MY_VAULT_FILE_SUCCEEDED)
            {
                result = Visibility.Collapsed;
            }
            else if ((status & ShareStatus.SHARE_PROJECT_FILE_SUCCEEDED) == ShareStatus.SHARE_PROJECT_FILE_SUCCEEDED)
            {
                result = Visibility.Collapsed;
            }
            else if ((status & ShareStatus.ERROR) == ShareStatus.ERROR)
            {
                result = Visibility.Collapsed;
            }
            return result;
        }

        public object ConvertBack(object value, Type targetType, object parameter, CultureInfo culture)
        {
            throw new NotImplementedException();
        }
    }


    public class CancelBtnVisibleConverter : IValueConverter
    {
        public object Convert(object value, Type targetType, object parameter, CultureInfo culture)
        {
            Visibility result = Visibility.Visible;
            Int32 status = (Int32)value;

            if ((status & ShareStatus.RESHARE_SUCCEEDED) == ShareStatus.RESHARE_SUCCEEDED)
            {
                result = Visibility.Collapsed;
            }
            else if ((status & ShareStatus.SHARE_MY_VAULT_FILE_SUCCEEDED) == ShareStatus.SHARE_MY_VAULT_FILE_SUCCEEDED)
            {
                result = Visibility.Collapsed;
            }
            else if ((status & ShareStatus.SHARE_PROJECT_FILE_SUCCEEDED) == ShareStatus.SHARE_PROJECT_FILE_SUCCEEDED)
            {
                result = Visibility.Collapsed;
            }
            else if ((status & ShareStatus.ERROR) == ShareStatus.ERROR)
            {
                result = Visibility.Collapsed;
            }
            return result;
        }

        public object ConvertBack(object value, Type targetType, object parameter, CultureInfo culture)
        {
            throw new NotImplementedException();
        }
    }

    public class CommentVisibleConverter : IValueConverter
    {
        public object Convert(object value, Type targetType, object parameter, CultureInfo culture)
        {
            Visibility result = Visibility.Collapsed;
            Int32 status = (Int32)value;

            if ((status & ShareStatus.IS_FROM_MYVAULT) == ShareStatus.IS_FROM_MYVAULT)
            {
                result = Visibility.Collapsed;
            }
            else if ((status & ShareStatus.IS_FROM_SHAREWITHME) == ShareStatus.IS_FROM_SHAREWITHME)
            {
                result = Visibility.Collapsed;
            }
            else if ((status & ShareStatus.IS_FROM_PROJECT) == ShareStatus.IS_FROM_PROJECT)
            {
                result = Visibility.Visible;
            }

            return result;
        }

        public object ConvertBack(object value, Type targetType, object parameter, CultureInfo culture)
        {
            throw new NotImplementedException();
        }
    }


}
