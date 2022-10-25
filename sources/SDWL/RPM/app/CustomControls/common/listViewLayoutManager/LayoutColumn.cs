using System;
using System.Windows;
using System.Windows.Controls;

namespace CustomControls.common.listViewLayoutManager
{
    public abstract class LayoutColumn
    {

        protected static double? GetColumnWidth(GridViewColumn column, DependencyProperty dp)
        {
            if (column == null)
            {
                throw new ArgumentNullException("column");
            }
            object value = column.ReadLocalValue(dp);
            if (value != null && value.GetType() == typeof(double))
            {
                return (double)value;
            }

            return null;
        } 
    } 
}
