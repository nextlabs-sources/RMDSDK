using System;
using System.ComponentModel;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Controls.Primitives;
using System.Windows.Input;
using System.Windows.Media;

namespace CustomControls.common.listViewLayoutManager
{
    public class ListViewLayoutManager
    {

        public static readonly DependencyProperty EnabledProperty = DependencyProperty.RegisterAttached(
            "Enabled",
            typeof(bool),
            typeof(ListViewLayoutManager),
            new FrameworkPropertyMetadata(new PropertyChangedCallback(OnLayoutManagerEnabledChanged)));

        public ListViewLayoutManager(ListView listView)
        {
            if (listView == null)
            {
                throw new ArgumentNullException("listView");
            }

            this.listView = listView;
            this.listView.Loaded += ListViewLoaded;
        }

        public ListView ListView
        {
            get { return this.listView; }
        } 

        public ScrollBarVisibility VerticalScrollBarVisibility
        {
            get { return this.verticalScrollBarVisibility; }
            set { this.verticalScrollBarVisibility = value; }
        } 

        public static void SetEnabled(DependencyObject dependencyObject, bool enabled)
        {
            dependencyObject.SetValue(EnabledProperty, enabled);
        } 

        private void RegisterEvents(DependencyObject start)
        {
            for (int i = 0; i < VisualTreeHelper.GetChildrenCount(start); i++)
            {
                Visual childVisual = VisualTreeHelper.GetChild(start, i) as Visual;
                if (childVisual is Thumb)
                {
                    GridViewColumn gridViewColumn = FindColumn(childVisual);
                    if (gridViewColumn == null)
                    {
                        continue;
                    }

                    Thumb thumb = childVisual as Thumb;
                    thumb.PreviewMouseMove += new MouseEventHandler(ThumbPreviewMouseMove);
                    thumb.PreviewMouseLeftButtonDown += new MouseButtonEventHandler(ThumbPreviewMouseLeftButtonDown);
                    DependencyPropertyDescriptor.FromProperty(
                        GridViewColumn.WidthProperty,
                        typeof(GridViewColumn)).AddValueChanged(gridViewColumn, GridColumnWidthChanged);
                }
                else if (this.scrollViewer == null && childVisual is ScrollViewer)
                {
                    this.scrollViewer = childVisual as ScrollViewer;
                    this.scrollViewer.ScrollChanged += new ScrollChangedEventHandler(ScrollViewerScrollChanged);
                    // assume we do the regulation of the horizontal scrollbar
                    this.scrollViewer.HorizontalScrollBarVisibility = ScrollBarVisibility.Hidden;
                    this.scrollViewer.VerticalScrollBarVisibility = this.verticalScrollBarVisibility;
                }

                RegisterEvents(childVisual);  
            }
        } 

        private GridViewColumn FindColumn(DependencyObject element)
        {
            if (element == null)
            {
                return null;
            }

            while (element != null)
            {
                if (element is GridViewColumnHeader)
                {
                    return ((GridViewColumnHeader)element).Column;
                }
                element = VisualTreeHelper.GetParent(element);
            }

            return null;
        } 

        protected virtual void ResizeColumns()
        {
            GridView view = this.listView.View as GridView;
            if (view == null)
            {
                return;
            }

            double actualWidth = this.scrollViewer != null ? this.scrollViewer.ViewportWidth : this.listView.ActualWidth;
            if (actualWidth <= 0)
            {
                return;
            }

            double resizeableRegionCount = 0;
            double otherColumnsWidth = 0;
            
            // determine column sizes
            foreach (GridViewColumn gridViewColumn in view.Columns)
            {
                otherColumnsWidth += gridViewColumn.ActualWidth;
            }

            if (resizeableRegionCount <= 0)
            {
                this.scrollViewer.HorizontalScrollBarVisibility = ScrollBarVisibility.Auto;
                return;
            }

            double resizeableColumnsWidth = actualWidth - otherColumnsWidth;
            if (resizeableColumnsWidth <= 0)
            {
                return;
            }

        } 

 
       
        private double SetRangeColumnToBounds(GridViewColumn gridViewColumn)
        {
            double startWidth = gridViewColumn.Width;

            double? minWidth = RangeColumn.GetRangeMinWidth(gridViewColumn);
            double? maxWidth = RangeColumn.GetRangeMaxWidth(gridViewColumn);

            if ((minWidth.HasValue && maxWidth.HasValue) && (minWidth > maxWidth))
            {
                return 0; // invalid case
            }

            if (minWidth.HasValue && gridViewColumn.Width < minWidth.Value)
            {
                gridViewColumn.Width = minWidth.Value;
            }
            else if (maxWidth.HasValue && gridViewColumn.Width > maxWidth.Value)
            {
                gridViewColumn.Width = maxWidth.Value;
            }

            return gridViewColumn.Width - startWidth;
        } 

        private void ListViewLoaded(object sender, RoutedEventArgs e)
        {
            RegisterEvents(this.listView);
            ResizeColumns();
            this.loaded = true;
        } 

        private void ThumbPreviewMouseMove(object sender, MouseEventArgs e)
        {
            Thumb thumb = sender as Thumb;
            GridViewColumn gridViewColumn = FindColumn(thumb);
            if (gridViewColumn == null)
            {
                return;
            }


            // check range column bounds
            if (thumb.IsMouseCaptured && RangeColumn.IsRangeColumn(gridViewColumn))
            {
                double? minWidth = RangeColumn.GetRangeMinWidth(gridViewColumn);
                double? maxWidth = RangeColumn.GetRangeMaxWidth(gridViewColumn);

                if ((minWidth.HasValue && maxWidth.HasValue) && (minWidth > maxWidth))
                {
                    return; // invalid case
                }

                if (this.resizeCursor == null)
                {
                    this.resizeCursor = thumb.Cursor; // save the resize cursor
                }

                if (minWidth.HasValue && gridViewColumn.Width <= minWidth.Value)
                {
                    thumb.Cursor = Cursors.No;
                }
                else if (maxWidth.HasValue && gridViewColumn.Width >= maxWidth.Value)
                {
                    thumb.Cursor = Cursors.No;
                }
                else
                {
                    thumb.Cursor = this.resizeCursor; // between valid min/max
                }
            }
        } 

        private void ThumbPreviewMouseLeftButtonDown(object sender, MouseButtonEventArgs e)
        {
            Thumb thumb = sender as Thumb;
            GridViewColumn gridViewColumn = FindColumn(thumb);

        }

        private void GridColumnWidthChanged(object sender, EventArgs e)
        {
            if (this.loaded)
            {
                GridViewColumn gridViewColumn = sender as GridViewColumn;

                // ensure range column within the bounds
                if (RangeColumn.IsRangeColumn(gridViewColumn))
                {
                    if (SetRangeColumnToBounds(gridViewColumn) != 0)
                    {
                        return;
                    }
                }

                ResizeColumns();
            }
        } 

        private void ScrollViewerScrollChanged(object sender, ScrollChangedEventArgs e)
        {
            if (this.loaded && e.ViewportWidthChange != 0)
            {
                ResizeColumns();
            }
        }

        private static void OnLayoutManagerEnabledChanged(DependencyObject dependencyObject, DependencyPropertyChangedEventArgs e)
        {
            ListView listView = dependencyObject as ListView;
            if (listView != null)
            {
                bool enabled = (bool)e.NewValue;
                if (enabled)
                {
                    new ListViewLayoutManager(listView);
                }
            }
        } 

        private readonly ListView listView;
        private ScrollViewer scrollViewer;
        private bool loaded = false;
        private Cursor resizeCursor;
        private ScrollBarVisibility verticalScrollBarVisibility = ScrollBarVisibility.Auto;

    } 

}
