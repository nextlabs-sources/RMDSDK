using CustomControls.components.CentralPolicy.model;
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
using System.Windows.Shapes;
using System.ComponentModel;
using CustomControls.componentPages.Preference;
using CustomControls.common.sharedResource;
using System.IO;
using System.Globalization;

namespace CustomControls.windows
{
    public class IsAutoProtection2TagOpacity : IValueConverter
    {
        public object Convert(object value, Type targetType, object parameter, CultureInfo culture)
        {
            try
            {
                if ((bool)value)
                {
                    return 1;
                }
                else
                {
                    return 0.4;
                }
            }
            catch (Exception)
            {
                return 1;
            }
        }

        public object ConvertBack(object value, Type targetType, object parameter, CultureInfo culture)
        {
            throw new NotImplementedException();
        }
    }


    public class IsNoClassification2Visibility : IValueConverter
    {
        public object Convert(object value, Type targetType, object parameter, CultureInfo culture)
        {
            try
            {
                Classification[] classification = (Classification[])value;
                if (null == classification || classification.Length == 0)
                {
                    return Visibility.Visible;
                }
                else
                {
                    return Visibility.Collapsed;
                }
            }
            catch (Exception)
            {
                return Visibility.Collapsed;
            }
        }

        public object ConvertBack(object value, Type targetType, object parameter, CultureInfo culture)
        {
            throw new NotImplementedException();
        }
    }


    public class Mfd_DataCommands
    {
        private static RoutedCommand apply;
        private static RoutedCommand cancel;
        static Mfd_DataCommands()
        {
            apply = new RoutedCommand(
              "Apply", typeof(Mfd_DataCommands));

            cancel = new RoutedCommand(
              "Cancel", typeof(Mfd_DataCommands));
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

    public class MyFolderConfigWindowViewModel : INotifyPropertyChanged
    {
        //  private MyFolderItem myFolderItem = null;
        private string folderName = "";
        private string folderPath = "";
        private bool autoProtection = false;
        private Dictionary<string, List<string>> selectedClassification = null;
        private Classification[] classification = null;
        private bool btnApplyIsEnable = true;
        private Visibility textNoclassification = Visibility.Collapsed;

        public MyFolderConfigWindowViewModel(string folderName,
                                             string folderPath,
                                             bool autoProtection,
                                             Dictionary<string, List<string>> selectedClassification,
                                             Classification[] classification)
        {
            if (!String.IsNullOrEmpty(folderName))
            {
                this.folderName = String.Copy(folderName);
            }

            if (!String.IsNullOrEmpty(folderPath))
            {
                this.folderPath = String.Copy(folderPath);
            }

            this.autoProtection = autoProtection;

            if (null != selectedClassification)
            {
                this.selectedClassification = new Dictionary<string, List<string>>(selectedClassification.Count, selectedClassification.Comparer);
                foreach (KeyValuePair<string, List<string>> entry in selectedClassification)
                {
                    List<string> vs = new List<string>(entry.Value.Count);
                    entry.Value.ForEach((tagItem) => {
                        if (!String.IsNullOrEmpty(tagItem))
                        {
                            vs.Add(String.Copy(tagItem));
                        }
                    });
                    this.selectedClassification.Add(entry.Key, vs);
                }
            }

            if (null != classification)
            {
               this.classification = classification;
            }

            bool b1 = false;
            if (null == this.classification )
            {
                b1 = true;
            }
            else if(null != this.classification && this.classification.Length == 0)
            {
                b1 = true;
            }

            bool b2 = false;
            if (null == this.selectedClassification)
            {
                b2 = true;
            }
            else if (null != this.selectedClassification && this.selectedClassification.Count() == 0)
            {
                b2 = true;
            }

            if (b1 && b2)
            {
                textNoclassification = Visibility.Visible;
            }
            else
            {
                textNoclassification = Visibility.Collapsed;
            }
        }

        //public MyFolderConfigWindowViewModel(MyFolderItem item, Classification[] classification)
        //{
        //    //for edit
        //    this.myFolderItem = item;
        //    this.classification = classification;
        //}

        //public MyFolderConfigWindowViewModel(Classification[] classification)
        //{
        //    //for add new My SkyDRM folder
        //    myFolderItem = null;
        //    this.classification = classification;
        //}
        //  public MyFolderItem MyFolderItem { get => myFolderItem; set { this.myFolderItem = value; OnPropertyChanged("MyFolderItem"); } }

        public string FolderName { get => folderName; set { this.folderName = value; OnPropertyChanged("FolderName"); } }

        public string FolderPath { get => folderPath; set { this.folderPath = value; OnPropertyChanged("FolderPath"); } }

        public bool AutoProtection { get => autoProtection; set { this.autoProtection = value; OnPropertyChanged("AutoProtection"); } }

        public Dictionary<string, List<string>> SelectedClassification { get => selectedClassification; set { this.selectedClassification = value; OnPropertyChanged("SelectedClassification"); } }

        public Classification[] Classification { get => classification; set { this.classification = value; OnPropertyChanged("Classification"); } }

        public bool BtnApplyIsEnable { get => btnApplyIsEnable; set { this.btnApplyIsEnable = value; OnPropertyChanged("BtnApplyIsEnable"); } }

        public Visibility TextNoclassification { get => textNoclassification; set { this.textNoclassification = value; OnPropertyChanged("TextNoclassification"); } }

        public event PropertyChangedEventHandler PropertyChanged;
        protected void OnPropertyChanged(string propertyName)
        {
            PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(propertyName));
        }
    }


    /// <summary>
    /// Interaction logic for MyFolderConfigWindow.xaml
    /// </summary>
    public partial class MyFolderConfigWindow : Window
    {

        private MyFolderConfigWindowViewModel viewModel = null;
        private MyFolderItem data_source_myFolder_item = null;
        // private ObservableCollection<MyFolderItem> myFolderList = null;
        private SelectClassificationEventArgs selectClassificationEventArgs;
        //template value , new value
        public event Func<MyFolderItem, MyFolderItem, bool> OnClickApply;

        public MyFolderConfigWindowViewModel ViewModel { get => viewModel; set { this.DataContext = viewModel = value; } }

        public MyFolderConfigWindow(MyFolderItem item, Classification[] classification)
        {
            //for edit
            string cp_folderName = "";
            string cp_folderPath = "";
            bool cp_auto_protection = false;
            Dictionary<string, List<string>> cp_selectedClassification = null;

            data_source_myFolder_item = item;

            if (!String.IsNullOrEmpty(item.FolderName))
            {
                cp_folderName = String.Copy(item.FolderName);
            }

            if (!String.IsNullOrEmpty(item.FolderPath))
            {
                cp_folderPath = String.Copy(item.FolderPath);
            }

            cp_auto_protection = item.AutoProtection;

            if (null != item.SelectedClassification)
            {
                cp_selectedClassification = new Dictionary<string, List<string>>(item.SelectedClassification.Count, item.SelectedClassification.Comparer);
                foreach (KeyValuePair<string, List<string>> entry in item.SelectedClassification)
                {
                    List<string> vs = new List<string>(entry.Value.Count);
                    entry.Value.ForEach((tagItem) => {
                        if (!String.IsNullOrEmpty(tagItem))
                        {
                            vs.Add(String.Copy(tagItem));
                        }
                    });
                    cp_selectedClassification.Add(entry.Key, vs);
                }
            }

            this.viewModel = new MyFolderConfigWindowViewModel(cp_folderName, cp_folderPath, cp_auto_protection, cp_selectedClassification, classification);

            this.Resources.MergedDictionaries.Add(SharedDictionaryManager.StringResource);
            this.Resources.MergedDictionaries.Add(SharedDictionaryManager.UnifiedCheckBoxStyle);
            this.Resources.MergedDictionaries.Add(SharedDictionaryManager.UnifiedBtnStyle);
            InitializeComponent();
            this.DataContext = this.viewModel;
            InitCommand();
        }

        public MyFolderConfigWindow(Classification[] classification)
        {
            //for create new one
            // this.myFolderList = myFolderList;
            this.viewModel = new MyFolderConfigWindowViewModel("", "", false, new Dictionary<string, List<string>>(), classification);
            this.Resources.MergedDictionaries.Add(SharedDictionaryManager.StringResource);
            this.Resources.MergedDictionaries.Add(SharedDictionaryManager.UnifiedCheckBoxStyle);
            this.Resources.MergedDictionaries.Add(SharedDictionaryManager.UnifiedBtnStyle);
            InitializeComponent();
            this.DataContext = this.viewModel;
            InitCommand();
        }

        private void InitCommand()
        {
            CommandBinding binding;
            binding = new CommandBinding(Mfd_DataCommands.Apply);
            binding.Executed += Mfd_Apply_Executed;
            binding.CanExecute += CanExecute_Mfd_Apply_Executed;
            this.CommandBindings.Add(binding);

            binding = new CommandBinding(Mfd_DataCommands.Cancel);
            binding.Executed += Mfd_Cancel_Executed;
            this.CommandBindings.Add(binding);
        }

        public void CanExecute_Mfd_Apply_Executed(object sender, CanExecuteRoutedEventArgs e)
        {
            //&& (null != selectClassificationEventArgs.KeyValues) && (selectClassificationEventArgs.KeyValues.Count != 0)
            if ((selectClassificationEventArgs.IsValid) || (!ViewModel.AutoProtection))
            {
                e.CanExecute = true;
            }
            else
            {
                e.CanExecute = false;
            }
        }

        private void Mfd_Apply_Executed(object sender, ExecutedRoutedEventArgs e)
        {
            e.Handled = true;
            Warning_Invalid_Path.Visibility = Visibility.Hidden;

            if (String.IsNullOrEmpty(this.viewModel.FolderPath) || !Directory.Exists(this.viewModel.FolderPath))
            {
                Warning_Invalid_Path.Visibility = Visibility.Visible;
                return;
            }

            //!selectClassificationEventArgs.IsValid
            //if (null == selectClassificationEventArgs.KeyValues ||
            //    selectClassificationEventArgs.KeyValues.Count == 0)
            //{
            //    Warning_Invalid_Tag.Visibility = Visibility.Visible;
            //    return;
            //}

            // this.ViewModel.BtnApplyIsEnable = false;
            string folderName = "";
            if (!String.IsNullOrEmpty(this.viewModel.FolderPath))
            {
                try
                {
                    folderName = System.IO.Path.GetFileName(this.viewModel.FolderPath);
                }
                catch (Exception ex)
                {
                }
            }

            string folderPath = this.viewModel.FolderPath;
            bool auto_protection = this.viewModel.AutoProtection;
            Dictionary<string, List<string>> selectedClassification = selectClassificationEventArgs.KeyValues;

            Nullable<bool> closeWindow = true;

            if (null == data_source_myFolder_item)
            {
                //for create new one
                closeWindow = OnClickApply?.Invoke(null, new MyFolderItem()
                {
                    FolderName = folderName,
                    FolderPath = folderPath,
                    AutoProtection = auto_protection,
                    SelectedClassification = selectedClassification
                });

                ////for create new one
                //myFolderList?.Add(new MyFolderItem() { FolderName = folderName,
                //    FolderPath = folderPath,
                //    Auto_protection = auto_protection,
                //    SelectedClassification = selectedClassification
                //});
            }
            else
            {
                //for edit
                closeWindow = OnClickApply?.Invoke(data_source_myFolder_item, new MyFolderItem()
                {
                    FolderName = folderName,
                    FolderPath = folderPath,
                    AutoProtection = auto_protection,
                    SelectedClassification = selectedClassification
                });

                //data_source_myFolder_item.FolderName = "";
                //if (!String.IsNullOrEmpty(this.viewModel.FolderPath))
                //{
                //    try
                //    {
                //        data_source_myFolder_item.FolderName = System.IO.Path.GetFileName(this.viewModel.FolderPath);
                //    }
                //    catch (Exception ex)
                //    {
                //    }
                //}

                //data_source_myFolder_item.FolderPath = folderPath;
                //data_source_myFolder_item.Auto_protection = auto_protection;
                //data_source_myFolder_item.SelectedClassification = selectedClassification;
            }

            if (closeWindow.GetValueOrDefault(true))
            {
                this.Close();
            }
        }

        private void Mfd_Cancel_Executed(object sender, ExecutedRoutedEventArgs e)
        {
            this.Close();
        }

        private void ClearBtn_Click(object sender, RoutedEventArgs e)
        {
            viewModel.FolderPath = "";
        }

        private void Browse_Button_Click(object sender, RoutedEventArgs e)
        {
            //init Dir.
            string dftRootFolder = ViewModel.FolderPath;
            System.Windows.Forms.FolderBrowserDialog dlg = new System.Windows.Forms.FolderBrowserDialog();
            dlg.Description = "Select a Folder";
            if (Directory.Exists(dftRootFolder))
            {
                dlg.SelectedPath = dftRootFolder;
            }
            var result = dlg.ShowDialog();
            if (result == System.Windows.Forms.DialogResult.OK || result == System.Windows.Forms.DialogResult.Yes)
            {
                // Will occur PathTooLong exception when dlg.SelectedPath is > 260
                try
                {
                    viewModel.FolderPath = dlg.SelectedPath;
                }
                catch (Exception ex)
                {
                    MessageBox.Show("is not allow selete a more than 260 character path");
                }
            }
        }

        private void CheckBox_Checked(object sender, RoutedEventArgs e)
        {
            viewModel.AutoProtection = true;
        }

        private void CheckBox_Unchecked(object sender, RoutedEventArgs e)
        {
            viewModel.AutoProtection = false;
        }

        private void OnSelectClassificationChanged(object sender, RoutedPropertyChangedEventArgs<SelectClassificationEventArgs> e)
        {
            Console.WriteLine($"CustomControl SelectClassificationChanged event: isValid({e.NewValue.IsValid}),select count({e.NewValue.KeyValues.Count})");
            selectClassificationEventArgs = e.NewValue;
            // ViewMode.TriggerClassificationChangedEvent(sender, e);
        }

        private void Apply_Button_Click(object sender, RoutedEventArgs e)
        {

        }

    }
}
