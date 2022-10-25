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

namespace CustomControls.pages.Preference
{
    /// <summary>
    /// SystemP.xaml  DataCommands
    /// </summary>
    public class Sys_DataCommands
    {
        private static RoutedCommand save;
        private static RoutedCommand apply;
        private static RoutedCommand cancel;
        static Sys_DataCommands()
        {
            save = new RoutedCommand(
              "Save", typeof(Sys_DataCommands));

            apply = new RoutedCommand(
              "Apply", typeof(Sys_DataCommands));

            cancel = new RoutedCommand(
              "Cancel", typeof(Sys_DataCommands));
        }
  
        /// <summary>
        /// System page save button command
        /// </summary>
        public static RoutedCommand Save
        {
            get { return save; }
        }
        /// <summary>
        /// System page apply button command
        /// </summary>
        public static RoutedCommand Apply
        {
            get { return apply; }
        }
        /// <summary>
        /// System page cancel button command
        /// </summary>
        public static RoutedCommand Cancel
        {
            get { return cancel; }
        }
    }

    /// <summary>
    /// ViewModel for SystemP.xaml
    /// </summary>
    public class SystemPViewModel : INotifyPropertyChanged
    {
        private bool? isShowNotify = false;
        private bool? isLeaveCopy = false;
        private bool btnApplyIsEnable;

        /// <summary>
        /// Control IsShowNotify checkBox isChecked
        /// </summary>
        public bool? IsShowNotify { get => isShowNotify; set { isShowNotify = value; OnPropertyChanged("IsShowNotify"); } }

        /// <summary>
        ///  Control IsLeaveCopy checkBox isChecked
        /// </summary>
        public bool? IsLeaveCopy { get => isLeaveCopy; set { isLeaveCopy = value; OnPropertyChanged("IsLeaveCopy"); } }

        /// <summary>
        /// Apply button isEnable, use for save 'Apply button' IsEnable status
        /// </summary>
        public bool BtnApplyIsEnable { get => btnApplyIsEnable; set { btnApplyIsEnable = value; OnPropertyChanged("BtnApplyIsEnable"); } }


        public event PropertyChangedEventHandler PropertyChanged;
        protected void OnPropertyChanged(string propertyName)
        {
            PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(propertyName));
        }
    }

    /// <summary>
    /// Interaction logic for SystemP.xaml
    /// </summary>
    public partial class SystemP : Page
    {
        private SystemPViewModel viewModel;
        public SystemP()
        {
            this.Resources.MergedDictionaries.Add(SharedDictionaryManager.StringResource);
            this.Resources.MergedDictionaries.Add(SharedDictionaryManager.UnifiedBtnStyle);
            this.Resources.MergedDictionaries.Add(SharedDictionaryManager.UnifiedCheckBoxStyle);

            InitializeComponent();

            this.DataContext = viewModel = new SystemPViewModel();
        }

        /// <summary>
        /// ViewModel for SystemP.xaml
        /// </summary>
        public SystemPViewModel ViewModel { get => viewModel; set { this.DataContext = viewModel = value; } }

        private void CheckBox_Checked(object sender, RoutedEventArgs e)
        {
            CheckBox checkBox = sender as CheckBox;
            if (checkBox != null && checkBox.Name != null)
            {
                viewModel.BtnApplyIsEnable = true;
            }
        }

        private void Apply_Button_Click(object sender, RoutedEventArgs e)
        {
            viewModel.BtnApplyIsEnable = false;
        }

    }
}
