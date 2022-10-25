using CustomControls.components.DigitalRights.model;
using System;
using System.Collections.Generic;
using System.Globalization;
using System.Linq;
using System.Text;
using System.Windows.Data;

namespace CustomControls.components.DigitalRights.helper
{
    public class WatermarkContainerVisibleConverter : IValueConverter
    {
        public object Convert(object value, Type targetType, object parameter, CultureInfo culture)
        {
            CheckStatus status = (CheckStatus)value;
            switch (status)
            {
                case CheckStatus.CHECKED:
                    return @"Visible";
                case CheckStatus.UNCHECKED:
                    return @"Hidden";
                default:
                    return @"Hidden";
            }
        }

        public object ConvertBack(object value, Type targetType, object parameter, CultureInfo culture)
        {
            throw new NotImplementedException();
        }
    }
}
