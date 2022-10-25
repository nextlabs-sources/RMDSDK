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

namespace CustomControls.officeUserControl
{
    /// <summary>
    /// ValiditySpecifyEx.xaml  DataCommands
    /// </summary>
    public class VSEx_DataCommands
    {
        private static RoutedCommand positive;
        private static RoutedCommand cancel;
        static VSEx_DataCommands()
        {
            InputGestureCollection input = new InputGestureCollection();
            input.Add(new KeyGesture(Key.Enter));
            positive = new RoutedCommand(
              "Positive", typeof(VSEx_DataCommands), input);

            input = new InputGestureCollection();
            input.Add(new KeyGesture(Key.Escape));
            cancel = new RoutedCommand(
              "Cancel", typeof(VSEx_DataCommands), input);
        }
        /// <summary>
        ///  ValiditySpecifyEx.xaml positive button command
        /// </summary>
        public static RoutedCommand Positive
        {
            get { return positive; }
        }
        /// <summary>
        /// ValiditySpecifyEx.xaml cancel button command
        /// </summary>
        public static RoutedCommand Cancel
        {
            get { return cancel; }
        }
    }

    /// <summary>
    /// ViewModel for ValiditySpecifyEx.xaml
    /// </summary>
    public class ValiditySpecifyExDataModel : INotifyPropertyChanged
    {
        private IExpiry expiry = new NeverExpireImpl();

        /// <summary>
        /// if ExpiryValue changed, will trigger this event
        /// This event is specially added for office add-ins winform, Other users can use this event or listen directly 
        /// ValiditySpecify.ExpiryValueChanged routed event.
        /// </summary>
        public event RoutedPropertyChangedEventHandler<ExpiryValueChangedEventArgs> OnExpiryValueChanged;

        /// <summary>
        /// Expiry value
        /// </summary>
        public IExpiry Expiry { get => expiry; set { expiry = value; OnPropertyChanged("Expiry"); } }

        /// <summary>
        /// Trigger OnExpiryValueChanged event
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
    /// Interaction logic for ValiditySpecifyEx.xaml
    /// </summary>
    public partial class ValiditySpecifyEx : UserControl
    {
        private ValiditySpecifyExDataModel viewModel;
        public ValiditySpecifyEx()
        {
            this.Resources.MergedDictionaries.Add(SharedDictionaryManager.StringResource);
            InitializeComponent();
            this.DataContext = viewModel = new ValiditySpecifyExDataModel();
        }

        /// <summary>
        /// ViewModel for ValiditySpecifyEx.xaml
        /// </summary>
        public ValiditySpecifyExDataModel ViewModel { get => viewModel; set { this.DataContext = viewModel = value; } }

      
        private void ValidityComponent_ExpiryValueChanged(object sender, RoutedPropertyChangedEventArgs<ExpiryValueChangedEventArgs> e)
        {
            viewModel.TriggerExpiryValueChangedEvent(sender, e);
        }
    }
}
