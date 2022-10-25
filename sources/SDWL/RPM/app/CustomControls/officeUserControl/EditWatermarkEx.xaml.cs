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
using CustomControls.components;

namespace CustomControls.officeUserControl
{
    /// <summary>
    /// EditWatermarkEx.xaml  DataCommands
    /// </summary>
    public class EditWEx_DataCommands
    {
        private static RoutedCommand positive;
        private static RoutedCommand cancel;
        static EditWEx_DataCommands()
        {
            InputGestureCollection input = new InputGestureCollection();
            input.Add(new KeyGesture(Key.Enter));
            positive = new RoutedCommand(
              "Positive", typeof(EditWEx_DataCommands), input);

            input = new InputGestureCollection();
            input.Add(new KeyGesture(Key.Escape));
            cancel = new RoutedCommand(
              "Cancel", typeof(EditWEx_DataCommands), input);
        }
        /// <summary>
        ///  EditWatermarkEx.xaml positive button command
        /// </summary>
        public static RoutedCommand Positive
        {
            get { return positive; }
        }
        /// <summary>
        /// EditWatermarkEx.xaml cancel button command
        /// </summary>
        public static RoutedCommand Cancel
        {
            get { return cancel; }
        }
    }

    /// <summary>
    /// ViewModel for EditWatermarkEx.xaml
    /// </summary>
    public class EditWarterMarkExDataModel : INotifyPropertyChanged
    {
        private string warterMark;
        private bool isEnableSaveBtn = true;

        /// <summary>
        /// if warterMark changed, will trigger this event
        /// This event is specially added for office add-ins winform, Other users can use this event or listen directly 
        /// EditWatermark.WarterMarkChanged routed event.
        /// </summary>
        public event RoutedPropertyChangedEventHandler<WarterMarkChangedEventArgs> OnWarterMarkChanged;

        /// <summary>
        /// Use for init value, can't get updated value
        /// </summary>
        public string WarterMark { get => warterMark; set { warterMark = value; OnPropertyChanged("WarterMark"); } }

        /// <summary>
        /// Save button isEnable, defult value is true
        /// </summary>
        public bool IsEnableSaveBtn { get => isEnableSaveBtn; set { isEnableSaveBtn = value; OnPropertyChanged("IsEnableSaveBtn"); } }

        /// <summary>
        /// Trigger WarterMarkChanged event
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        internal void TriggerWarterMarkChangedEvent(object sender, RoutedPropertyChangedEventArgs<WarterMarkChangedEventArgs> e)
        {
            OnWarterMarkChanged?.Invoke(sender, e);
        }

        public event PropertyChangedEventHandler PropertyChanged;
        protected void OnPropertyChanged(string propertyName)
        {
            PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(propertyName));
        }
    }

    /// <summary>
    /// Interaction logic for EditWatermarkEx.xaml
    /// </summary>
    public partial class EditWatermarkEx : UserControl
    {
        private EditWarterMarkExDataModel viewModel;
        public EditWatermarkEx()
        {
            this.Resources.MergedDictionaries.Add(SharedDictionaryManager.StringResource);
            InitializeComponent();
            this.DataContext = viewModel = new EditWarterMarkExDataModel();
        }

        /// <summary>
        /// ViewModel for EditWatermarkEx.xaml
        /// </summary>
        public EditWarterMarkExDataModel ViewModel { get => viewModel; set { this.DataContext = viewModel = value; } }


        private void Edit_WarterMarkChanged(object sender, RoutedPropertyChangedEventArgs<WarterMarkChangedEventArgs> e)
        {
            viewModel.TriggerWarterMarkChangedEvent(sender, e);
        }
    }
}
