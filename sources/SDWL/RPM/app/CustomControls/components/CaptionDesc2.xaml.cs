using System;
using System.Collections.Generic;
using System.ComponentModel;
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
    /// ViewModel for CaptionDesc2.xaml
    /// </summary>
    public class CaptionDesc2ViewMode : INotifyPropertyChanged
    {
        private string title;
        private string fileName;

        /// <summary>
        /// Title, defult value is null
        /// </summary>
        public string Title { get => title; set { title = value; OnBindUIPropertyChanged("Title"); } }

        /// <summary>
        /// File names, defult value is null
        /// </summary>
        public string FileName { get => fileName; set { fileName = value; OnBindUIPropertyChanged("FileName"); } }


        public event PropertyChangedEventHandler PropertyChanged;
        protected void OnBindUIPropertyChanged(string propertyName)
        {
            PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(propertyName));
        }

    }
    /// <summary>
    /// Interaction logic for CaptionDesc2.xaml
    /// </summary>
    public partial class CaptionDesc2 : UserControl
    {
        private CaptionDesc2ViewMode viewModel;
        public CaptionDesc2()
        {
            InitializeComponent();
            this.DataContext = viewModel = new CaptionDesc2ViewMode();
        }

        /// <summary>
        ///  ViewModel for CaptionDesc2.xaml
        /// </summary>
        public CaptionDesc2ViewMode ViewModel { get => viewModel; set => this.DataContext = viewModel = value; }
    }
}
