using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;

namespace CustomControls.components
{
    /// <summary>
    /// Interaction logic for AccessDeniedView.xaml
    /// </summary>
    public partial class AccessDeniedView : UserControl
    {
        public AccessDeniedView()
        {
            InitializeComponent();
        }

        #region Declare and register Dependency Property
        public static readonly DependencyProperty InfoTextProperty = DependencyProperty.Register("InfoText", typeof(string),
                typeof(AccessDeniedView));

        public string InfoText
        {
            get { return (string)GetValue(InfoTextProperty); }
            set { SetValue(InfoTextProperty, value); }
        }
        #endregion
    }
}
