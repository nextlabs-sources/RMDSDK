using System.Collections;
using System.Windows;
using System.Windows.Documents;
using System.Windows.Media;

namespace CustomControls.common.sortListView
{
    public class UIElementAdorner : Adorner
    {
        private UIElement child;

        /// <summary>
        /// 
        /// </summary>
        /// <param name="element">the UIElement that this adorner is bound to</param>
        /// <param name="child">the adorner</param>
        public UIElementAdorner(UIElement element, UIElement child)
            : base(element)
        {
            this.child = child;
            AddLogicalChild(child);
            AddVisualChild(child);
        }

        protected override Size ArrangeOverride(Size finalSize)
        {
            child.Arrange(new Rect(finalSize));

            return finalSize;
        }

        protected override Size MeasureOverride(Size constraint)
        {
            child.Measure(constraint);

            return AdornedElement.RenderSize;
        }

        protected override int VisualChildrenCount
        {
            get { return 1; }
        }

        protected override Visual GetVisualChild(int index)
        {
            return child;
        }

        protected override IEnumerator LogicalChildren
        {
            get
            {
                ArrayList list = new ArrayList();
                list.Add(child);
                return (IEnumerator)list.GetEnumerator();
            }
        }

        /// <summary>
        /// 
        /// </summary>
        public UIElement Child
        {
            get { return child; }
        }
    }
}
