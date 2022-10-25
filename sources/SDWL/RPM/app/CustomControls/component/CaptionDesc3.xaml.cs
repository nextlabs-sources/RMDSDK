using CustomControls.common.sharedResource;
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
    /// ViewModel for CaptionDesc3.xaml
    /// </summary>
    public class CaptionDesc3ViewMode : INotifyPropertyChanged
    {
        private string title;
        private string fileName;
        private string promptText;
        private string desitination;

        /// <summary>
        /// Title, defult value is null
        /// </summary>
        public string Title { get => title; set { title = value; OnBindUIPropertyChanged("Title"); } }

        /// <summary>
        /// File names, defult value is null
        /// </summary>
        public string FileName { get => fileName; set { fileName = value; OnBindUIPropertyChanged("FileName"); } }

        /// <summary>
        /// Before 'save to' text, should dislplay 'have been' or 'has been'
        /// </summary>
        public string PromptText { get => promptText; set { promptText = value; OnBindUIPropertyChanged("PromptText"); } }

        /// <summary>
        /// Display desitination, like Project,WorkSpace
        /// </summary>
        public string Desitination { get => desitination; set { desitination = value; OnBindUIPropertyChanged("Desitination"); } }


        public event PropertyChangedEventHandler PropertyChanged;
        protected void OnBindUIPropertyChanged(string propertyName)
        {
            PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(propertyName));
        }

    }
    /// <summary>
    /// Interaction logic for CaptionDesc3.xaml
    /// </summary>
    public partial class CaptionDesc3 : UserControl
    {
        private CaptionDesc3ViewMode viewModel;

        public CaptionDesc3()
        {
            this.Resources.MergedDictionaries.Add(SharedDictionaryManager.StringResource);

            InitializeComponent();
            this.DataContext = viewModel = new CaptionDesc3ViewMode();
        }
        /// <summary>
        /// ViewModel for CaptionDesc3.xaml
        /// </summary>
        public CaptionDesc3ViewMode ViewModel { get => viewModel; set => this.DataContext = viewModel = value; }
    }
}
