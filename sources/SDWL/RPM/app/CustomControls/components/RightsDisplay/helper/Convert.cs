using System;
using System.Collections.Generic;
using System.Globalization;
using System.Linq;
using System.Text;
using System.Windows;
using System.Windows.Data;

namespace CustomControls.components.RightsDisplay.helper
{
    public class LineVisibleConverter : IMultiValueConverter
    {
        public object Convert(object[] value, Type targetType, object parameter, CultureInfo culture)
        {
            try
            {
                Visibility wpVs = (Visibility)value[0];
                Visibility vpVs = (Visibility)value[1];
                if (wpVs== Visibility.Visible || vpVs == Visibility.Visible)
                {
                    return Visibility.Visible;
                }
                return Visibility.Collapsed;
            }
            catch (Exception)
            {
                return Visibility.Collapsed;
            }
            
        }

        public object[] ConvertBack(object value, Type[] targetType, object parameter, CultureInfo culture)
        {
            throw new NotImplementedException();
        }
    }

    public class ForegroundConverter : IValueConverter
    {
        public object Convert(object value, Type targetType, object parameter, CultureInfo culture)
        {
            string displayExpiration = (string)value;
            if (string.Equals(displayExpiration, "Expired", StringComparison.CurrentCultureIgnoreCase))
            {
                return @"#EB5757";
            }
            return @"#828282";
        }

        public object ConvertBack(object value, Type targetType, object parameter, CultureInfo culture)
        {
            throw new NotImplementedException();
        }
    }
}
