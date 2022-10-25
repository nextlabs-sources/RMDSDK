using CustomControls.components.TreeView.viewModel;
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

namespace CustomControls.components.TreeView.view
{
    /// <summary>
    /// Interaction logic for TreeViewComponent.xaml
    /// </summary>
    public partial class TreeViewComponent : UserControl
    {
        private TreeViewViewModel viewModel;

        public TreeViewComponent()
        {
            InitializeComponent();
            this.DataContext = viewModel = new TreeViewViewModel();
        }
        /// <summary>
        /// ViewModel for TreeViewComponent.xaml
        /// </summary>
        public TreeViewViewModel ViewModel { get => viewModel; set => this.DataContext = viewModel = value; }
    }
}
