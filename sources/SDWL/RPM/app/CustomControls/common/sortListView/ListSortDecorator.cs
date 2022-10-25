
using System.ComponentModel;
using System.Windows;
using System.Windows.Controls;

namespace CustomControls.common.sortListView
{
    /// <summary>
    /// Define a control Lookless control: to handle the sort direction and draw a Sort arrow.
    /// 
    /// Follow steps 1a or 1b and then 2 to use this custom control in a XAML file.
    ///
    /// Step 1a) Using this custom control in a XAML file that exists in the current project.
    /// Add this XmlNamespace attribute to the root element of the markup file where it is 
    /// to be used:
    ///
    ///     xmlns:MyNamespace="clr-namespace:SkydrmLocal.rmc.ui.components.sortListView"
    ///
    ///
    /// Step 1b) Using this custom control in a XAML file that exists in a different project.
    /// Add this XmlNamespace attribute to the root element of the markup file where it is 
    /// to be used:
    ///
    ///     xmlns:MyNamespace="clr-namespace:SkydrmLocal.rmc.ui.components.sortListView;assembly=SkydrmLocal.rmc.ui.components.sortListView"
    ///
    /// You will also need to add a project reference from the project where the XAML file lives
    /// to this project and Rebuild to avoid compilation errors:
    ///
    ///     Right click on the target project in the Solution Explorer and
    ///     "Add Reference"->"Projects"->[Browse to and select this project]
    ///
    ///
    /// Step 2)
    /// Go ahead and use your control in the XAML file.
    ///
    ///     <MyNamespace:ListSortDecorator/>
    ///
    /// </summary>
    public class ListSortDecorator : Control
    {
        // Using a DependencyProperty as the backing store for SortDirection.  This enables animation, styling, binding, etc...
        public static readonly DependencyProperty SortDirectionProperty =
            DependencyProperty.Register("SortDirection", typeof(ListSortDirection), typeof(ListSortDecorator));

        static ListSortDecorator()
        {
            DefaultStyleKeyProperty.OverrideMetadata(typeof(ListSortDecorator), new FrameworkPropertyMetadata(typeof(ListSortDecorator)));
        }

        // CLR property wrapper.
        public ListSortDirection SortDirection
        {
            get { return (ListSortDirection)GetValue(SortDirectionProperty); }
            set { SetValue(SortDirectionProperty, value); }
        }
    }
}
