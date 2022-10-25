using System.Windows;
using System.Windows.Controls;

namespace CustomControls.common.listViewLayoutManager
{
    public sealed class RangeColumn : LayoutColumn
    {

        public static readonly DependencyProperty MinWidthProperty =
            DependencyProperty.RegisterAttached(
                "MinWidth",
                typeof(double),
                typeof(RangeColumn));

        public static readonly DependencyProperty MaxWidthProperty =
            DependencyProperty.RegisterAttached(
                "MaxWidth",
                typeof(double),
                typeof(RangeColumn));

        private RangeColumn()
        {
        } 

        public static double GetMinWidth(DependencyObject obj)
        {
            return (double)obj.GetValue(MinWidthProperty);
        } 

        public static void SetMinWidth(DependencyObject obj, double minWidth)
        {
            obj.SetValue(MinWidthProperty, minWidth);
        } 

        public static double GetMaxWidth(DependencyObject obj)
        {
            return (double)obj.GetValue(MaxWidthProperty);
        } 

        public static void SetMaxWidth(DependencyObject obj, double maxWidth)
        {
            obj.SetValue(MaxWidthProperty, maxWidth);
        } 

        public static bool IsRangeColumn(GridViewColumn column)
        {
            if (column == null)
            {
                return false;
            }
            return GetRangeMinWidth(column).HasValue || GetRangeMaxWidth(column).HasValue;
        } 

        public static double? GetRangeMinWidth(GridViewColumn column)
        {
            return GetColumnWidth(column, MinWidthProperty);
        } 

        public static double? GetRangeMaxWidth(GridViewColumn column)
        {
            return GetColumnWidth(column, MaxWidthProperty);
        } 

        public static GridViewColumn ApplyWidth(GridViewColumn gridViewColumn, double minWidth, double width, double maxWidth)
        {
            SetMinWidth(gridViewColumn, minWidth);
            gridViewColumn.Width = width;
            SetMaxWidth(gridViewColumn, maxWidth);
            return gridViewColumn;
        } 

    } 
}
