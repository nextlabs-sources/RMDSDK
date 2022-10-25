using CustomControls.common.sharedResource;
using CustomControls.components.ValiditySpecify.model;
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

namespace CustomControls.pages.Preference
{
    /// <summary>
    /// DocumentP.xaml  DataCommands
    /// </summary>
    public class Dcm_DataCommands
    {
        private static RoutedCommand save;
        private static RoutedCommand apply;
        private static RoutedCommand cancel;
        static Dcm_DataCommands()
        {
            save = new RoutedCommand(
              "Save", typeof(Dcm_DataCommands));

            apply = new RoutedCommand(
              "Apply", typeof(Dcm_DataCommands));

            cancel = new RoutedCommand(
              "Cancel", typeof(Dcm_DataCommands));
        }
        /// <summary>
        /// Document page save button command
        /// </summary>
        public static RoutedCommand Save
        {
            get { return save; }
        }
        /// <summary>
        /// Document page apply button command
        /// </summary>
        public static RoutedCommand Apply
        {
            get { return apply; }
        }
        /// <summary>
        /// Document page cancel button command
        /// </summary>
        public static RoutedCommand Cancel
        {
            get { return cancel; }
        }
    }

    /// <summary>
    /// ViewModel for DocumentP.xaml
    /// </summary>
    public class DocumentPViewModel : INotifyPropertyChanged
    {
        private string warterMark = "";
        private IExpiry expiry = new NeverExpireImpl();
        private bool btnSaveIsEnable = true;
        private bool btnApplyIsEnable = true;
        

        /// <summary>
        /// WareterMark value
        /// </summary>
        public string WarterMark { get => warterMark; set { warterMark = value; OnPropertyChanged("WarterMark"); } }
        
        /// <summary>
        /// Expiry value
        /// </summary>
        public IExpiry Expiry { get => expiry; set { expiry = value; OnPropertyChanged("Expiry"); } }

        /// <summary>
        /// Save button isEnable
        /// </summary>
        public bool BtnSaveIsEnable { get => btnSaveIsEnable; set { btnSaveIsEnable = value; OnPropertyChanged("BtnSaveIsEnable"); } }

        /// <summary>
        /// Apply button isEnable 
        /// </summary>
        public bool BtnApplyIsEnable { get => btnApplyIsEnable; set { btnApplyIsEnable = value; OnPropertyChanged("BtnApplyIsEnable"); } }

        /// <summary>
        /// if warterMark changed, will trigger this event.
        /// Other users can use this event or listen directly EditWarterMark.WarterMarkChanged routed event.
        /// </summary>
        public event RoutedPropertyChangedEventHandler<components.WarterMarkChangedEventArgs> OnWarterMarkChanged;

        /// <summary>
        /// if expiry changed, will trigger this event.
        /// Other users can use this event or listen directly ValiditySpecify.ExpiryValueChanged routed event.
        /// </summary>
        public event RoutedPropertyChangedEventHandler<ExpiryValueChangedEventArgs> OnExpiryValueChanged;

        /// <summary>
        /// Trigger OnWarterMarkChanged route event
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        internal void TriggerWarterMarkChangedEvent(object sender, RoutedPropertyChangedEventArgs<components.WarterMarkChangedEventArgs> e)
        {
            OnWarterMarkChanged?.Invoke(sender, e);
        }
        /// <summary>
        /// Trigger OnExpiryValueChanged route event
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        internal void TriggerExpiryValueChangedEvent(object sender, RoutedPropertyChangedEventArgs<ExpiryValueChangedEventArgs> e)
        {
            OnExpiryValueChanged?.Invoke(sender, e);
        }

        public event PropertyChangedEventHandler PropertyChanged;
        protected void OnPropertyChanged(string propertyName)
        {
            PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(propertyName));
        }
    }
    /// <summary>
    /// Interaction logic for DocumentP.xaml
    /// </summary>
    public partial class DocumentP : Page
    {
        private DocumentPViewModel viewModel;
        public DocumentP()
        {
            this.Resources.MergedDictionaries.Add(SharedDictionaryManager.StringResource);
            this.Resources.MergedDictionaries.Add(SharedDictionaryManager.UnifiedBtnStyle);

            InitializeComponent();
            this.DataContext = viewModel = new DocumentPViewModel();
        }

        /// <summary>
        /// ViewModel for DocumentP.xaml
        /// </summary>
        public DocumentPViewModel ViewModel { get => viewModel; set { this.DataContext = viewModel = value; } }

        private void EditWaterMark_WarterMarkChanged(object sender, RoutedPropertyChangedEventArgs<components.WarterMarkChangedEventArgs> e)
        {
            ViewModel.TriggerWarterMarkChangedEvent(sender, e);
        }
        private void ValidityComponent_ExpiryValueChanged(object sender, RoutedPropertyChangedEventArgs<ExpiryValueChangedEventArgs> e)
        {
            ViewModel.TriggerExpiryValueChangedEvent(sender, e);
        }

    }
}
