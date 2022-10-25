using CustomControls.common.sharedResource;
using CustomControls.components.RightsDisplay.model;
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

namespace CustomControls.components.RightsDisplay
{
    /// <summary>
    /// Interaction logic for RightsStackPanle.xaml
    /// </summary>
    public partial class RightsStackPanle : UserControl
    {
        private RightsStPanViewModel viewModel;

        public RightsStackPanle()
        {
            this.Resources.MergedDictionaries.Add(SharedDictionaryManager.StringResource);
            InitializeComponent();
            this.DataContext = viewModel = new RightsStPanViewModel(this);
        }

        /// <summary>
        /// ViewModel for RightsStackPanle.xaml 
        /// </summary>
        public RightsStPanViewModel ViewModel { get => viewModel; set => this.DataContext = viewModel = value; }
    }
}
