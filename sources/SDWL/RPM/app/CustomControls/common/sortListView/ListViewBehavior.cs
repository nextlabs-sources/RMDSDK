using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.ComponentModel;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Documents;

namespace CustomControls.common.sortListView
{

    /// <summary>
    /// Define and add AttachProperty for ListView, then implement Sort by click Column header.
    /// </summary>
    public class ListViewBehavior : DependencyObject
    {
        /// <summary>
        /// Declare and register AttachProperty - HeaderSortProperty(bool)
        /// </summary>
        public static readonly DependencyProperty HeaderSortProperty =
            DependencyProperty.RegisterAttached("HeaderSort", typeof(bool), typeof(ListViewBehavior), 
                new UIPropertyMetadata(new PropertyChangedCallback(OnHeaderSortPropertyChanged)));

        /// <summary>
        /// Declare and register AttachProperty Key - SortInfoProperty(SortInfo)
        /// </summary>
        internal static readonly DependencyPropertyKey SortInfoProperty =
            DependencyProperty.RegisterAttachedReadOnly("SortInfo", typeof(SortInfo), typeof(ListViewBehavior), new PropertyMetadata());

        /// <summary>
        ///  Declare and register AttachProperty - SortFieldProperty(string)
        /// </summary>
        public static readonly DependencyProperty SortFieldProperty =
            DependencyProperty.RegisterAttached("SortField", typeof(string), typeof(ListViewBehavior));

        #region add CLR property for AttachProperty

        // HeaderSortProperty CLR Property
        public static bool GetHeaderSort(DependencyObject obj)
        {
            return (bool)obj.GetValue(HeaderSortProperty);
        }

        public static void SetHeaderSort(DependencyObject obj, bool value)
        {
            obj.SetValue(HeaderSortProperty, value);
        }

        // SortInfoProperty CLR Property
        public static SortInfo GetSortInfo(DependencyObject obj)
        {
            return (SortInfo)obj.GetValue(SortInfoProperty.DependencyProperty);
        }

        internal static void SetSortInfo(DependencyObject obj, SortInfo value)
        {
            obj.SetValue(SortInfoProperty.DependencyProperty, value);
        }


        // SortFieldProperty CLR Property
        public static string GetSortField(DependencyObject obj)
        {
            return (string)obj.GetValue(SortFieldProperty);
        }

        public static void SetSortField(DependencyObject obj, string value)
        {
            obj.SetValue(SortFieldProperty, value);
        }

        #endregion

        private static void OnHeaderSortPropertyChanged(DependencyObject sender, DependencyPropertyChangedEventArgs e)
        {
            ListView listView = sender as ListView;
            if (listView == null)
                throw new InvalidOperationException("HeaderSort Property can only be set on a ListView");

            if ((bool)e.NewValue)
            {
                // Register a column header for Click handler
                listView.AddHandler(GridViewColumnHeader.ClickEvent, new RoutedEventHandler(OnListViewHeaderClick));
            }
            else
            {
                listView.RemoveHandler(GridViewColumnHeader.ClickEvent, new RoutedEventHandler(OnListViewHeaderClick));
            }
        }

      
        private static void OnListViewHeaderClick(object sender, RoutedEventArgs e)
        {

            ListView listView = e.Source as ListView;
            GridViewColumnHeader header = e.OriginalSource as GridViewColumnHeader;

            InnerSort(listView, header, false);
        }

        /// <summary>
        /// Init sort direction -- default is Ascending.
        /// </summary>
        public static void InitSortDirection(ListView listView, GridViewColumnHeader header)
        {
            SortInfo sortInfo = listView.GetValue(SortInfoProperty.DependencyProperty) as SortInfo;
            if (sortInfo != null && sortInfo.LastSortColumn == header)
            {
                (sortInfo.CurrentAdorner.Child as ListSortDecorator).SortDirection = ListSortDirection.Descending;
            }
        }


        private static void InnerSort(ListView listView, GridViewColumnHeader header, bool IsDefaultSort)
        {
            if (header == null || listView == null)
            {
                return;
            }

            try
            {
                SortInfo sortInfo = listView.GetValue(SortInfoProperty.DependencyProperty) as SortInfo;
                if (sortInfo != null)
                {
                    AdornerLayer.GetAdornerLayer(sortInfo.LastSortColumn).Remove(sortInfo.CurrentAdorner);

                    // clear sort descriptions.
                    listView.Items.SortDescriptions.Clear();
                }
                else
                {
                    sortInfo = new SortInfo();
                }

                if (sortInfo.LastSortColumn == header) // click the same header.
                {
                    if (IsDefaultSort)
                    {
                        (sortInfo.CurrentAdorner.Child as ListSortDecorator).SortDirection = ListSortDirection.Descending;
                    }
                    else
                    {
                       if ((sortInfo.CurrentAdorner.Child as ListSortDecorator).SortDirection == ListSortDirection.Ascending)
                        {
                            (sortInfo.CurrentAdorner.Child as ListSortDecorator).SortDirection = ListSortDirection.Descending;
                        }
                        else
                        {
                            (sortInfo.CurrentAdorner.Child as ListSortDecorator).SortDirection = ListSortDirection.Ascending;
                        }

                    }
                }
                else
                {
                    // Default sort is 'Descending' of DateModified.
                    var sorDecorator = new ListSortDecorator() { SortDirection = ListSortDirection.Descending };
                    sortInfo.CurrentAdorner = new UIElementAdorner(header, sorDecorator);
                }

                sortInfo.LastSortColumn = header;
                listView.SetValue(SortInfoProperty, sortInfo);

                AdornerLayer.GetAdornerLayer(header)?.Add(sortInfo.CurrentAdorner);

                if (header.Column != null) 
                {
                    SortDescription sortDescriptioin = new SortDescription()
                    {
                        Direction = (sortInfo.CurrentAdorner.Child as ListSortDecorator).SortDirection,
                        PropertyName = header.Column.GetValue(SortFieldProperty) as string ?? header.Column.Header as string
                    };

                    // sort
                    listView.Items.SortDescriptions.Add(sortDescriptioin);

                }

            }
            catch (Exception e)
            {
                Console.WriteLine("CustomControl:"+e.Message);
            }

        }

        public static void DefaultSort(ListView listView, GridViewColumnHeader header)
        {
            if (listView == null || header == null)
            {
                return;
            }

            InnerSort(listView, header, true);
        }

        public static void DoSortByClickMenu(ListView listView, GridViewColumnHeader header)
        {
            if (listView == null || header == null)
            {
                return;
            }

            InnerSort(listView, header, false);
        }
    }
}
