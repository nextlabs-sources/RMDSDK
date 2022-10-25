using System;
using System.Collections.Generic;
using System.Globalization;
using System.Linq;
using System.Text;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Media;

namespace CustomControls.components.TreeView.helper
{
    public class IndentConverter : IValueConverter
    {
        public object Convert(object value, Type targetType, object parameter, CultureInfo culture)
        {
            double colunwidth = 12;
            double left = 0.0;

            UIElement element = value as TreeViewItem;
            while (element.GetType() != typeof(System.Windows.Controls.TreeView))
            {
                element = (UIElement)VisualTreeHelper.GetParent(element);
                if (element.GetType() == typeof(TreeViewItem))
                    left += colunwidth;
            }
            return new Thickness(left, 5, 0, 0);
        }

        public object ConvertBack(object value, Type targetType, object parameter, CultureInfo culture)
        {
            throw new NotImplementedException();
        }

    }
}
