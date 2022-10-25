using CustomControls.common.sharedResource;
using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Globalization;
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
    #region Convert
    /// <summary>
    /// When select mulit files,display file count
    /// </summary>
    public class FileCountTextConverter : IValueConverter
    {
        public object Convert(object value, Type targetType, object parameter, CultureInfo culture)
        {
            try
            {
                int fileCount = (int)value;
                if (fileCount > 1)
                {
                    return string.Format(@"({0})", fileCount);
                }
                return "";
            }
            catch (Exception)
            {
                return "";
            }
        }

        public object ConvertBack(object value, Type targetType, object parameter, CultureInfo culture)
        {
            throw new NotImplementedException();
        }
    }
    #endregion

    /// <summary>
    /// CaptionDesc.xaml  DataCommands
    /// </summary>
    public class CapD_DataCommands
    {
        private static RoutedCommand change;
        
        static CapD_DataCommands()
        {
            change = new RoutedCommand(
              "Change", typeof(CapD_DataCommands));
        }
        /// <summary>
        /// CaptionDesc.xaml change button command
        /// </summary>
        public static RoutedCommand Change
        {
            get { return change; }
        }
        
    }

    /// <summary>
    /// ViewModel for CaptionDesc.xaml
    /// </summary>
    public class CaptionDescViewMode : INotifyPropertyChanged
    {
        private CaptionDesc host;
        private string mTitle = "test Title";
        private string mDesc = "test Description";
        private Visibility mDescVisibility = Visibility.Visible;
        private int fileCount = 1;
        private string fileName;
        private Visibility changeBtnVisible = Visibility.Visible;
        private string permissionDecribe;
        private Dictionary<string, List<string>> filePermissions = new Dictionary<string, List<string>>();
        private Visibility permissionVisibility = Visibility.Collapsed;

        public CaptionDescViewMode(CaptionDesc captionDesc)
        {
            host = captionDesc;
        }
        /// <summary>
        /// Title, defult value is 'test Title'
        /// </summary>
        public string Title { get => mTitle; set { mTitle = value; OnBindUIPropertyChanged("Title"); } }

        /// <summary>
        /// Description, defult value is 'test Description'
        /// </summary>
        public string Description { get => mDesc; set { mDesc = value; OnBindUIPropertyChanged("Description"); } }

        /// <summary>
        /// Description visibility, defult value is Visible
        /// </summary>
        public Visibility DescriptionVisibility { get => mDescVisibility; set { mDescVisibility = value; OnBindUIPropertyChanged("DescriptionVisibility"); } }

        /// <summary>
        /// File count, defult value is 1.
        /// </summary>
        public int FileCount { get => fileCount; set { fileCount = value; OnBindUIPropertyChanged("FileCount"); } }

        /// <summary>
        /// File names, defult value is null
        /// </summary>
        public string FileName { get => fileName; set { fileName = value; OnBindUIPropertyChanged("FileName"); } }

        /// <summary>
        /// Change button visible, defult value is Visibility.Visible
        /// </summary>
        public Visibility ChangeBtnVisible { get => changeBtnVisible; set { changeBtnVisible = value; OnBindUIPropertyChanged("ChangeBtnVisible"); } }

        /// <summary>
        /// original file permissions description, defult value is null.
        /// </summary>
        public string PermissionDescribe { get => permissionDecribe; set { permissionDecribe = value; OnBindUIPropertyChanged("PermissionDescribe"); } }

        /// <summary>
        /// original file permissions, defult value is empty. 
        /// When setting this property. if file is adhoc, key string is empty, value list are rights.
        /// if file is centralPolicy, key string is tag name, value list are tag values.
        /// </summary>
        public Dictionary<string, List<string>> FilePermissions { get => filePermissions; set => SetDisplayPermission(value); }

        /// <summary>
        /// internal binding to display original file permissions, defult value is Visibility.Collapsed
        /// </summary>
        public Visibility PermissionVisibility { get => permissionVisibility; set { permissionVisibility = value; OnBindUIPropertyChanged("PermissionVisibility"); } }

        private void SetDisplayPermission(Dictionary<string, List<string>> keyValues)
        {
            filePermissions = keyValues;

            var tags = keyValues;
            //Check nonull for tags.
            if (tags != null || tags.Count != 0)
            {
                //Get the iterator of the dictionary.
                var iterator = tags.GetEnumerator();
                //If there is any items inside it.
                while (iterator.MoveNext())
                {
                    //Get the current one.
                    var current = iterator.Current;

                    string key = current.Key;
                    List<string> values = current.Value;
                    for (int i = 0; i < values.Count; i++)
                    {
                        host.tb_FilePermission.Inlines.Add(CreateRunValue(values[i]));
                        if (i < values.Count - 1)
                        {
                            host.tb_FilePermission.Inlines.Add(CreateRunValue(", "));
                        }
                    }
                    if (!string.IsNullOrWhiteSpace(key))
                    {
                        host.tb_FilePermission.Inlines.Add(CreateRunValue(" ("));
                        host.tb_FilePermission.Inlines.Add(CreateRunKey(key));
                        host.tb_FilePermission.Inlines.Add(CreateRunValue(")   "));
                    }
                }
                PermissionVisibility = Visibility.Visible;
            }
            else
            {
                PermissionVisibility = Visibility.Collapsed;
            }
        }
        private Run CreateRunValue(string value)
        {
            return new Run
            {
                Foreground = new SolidColorBrush(Colors.Black),
                FontSize = 14,
                Text = value,
                FontWeight = FontWeights.Normal,
            };
        }
        private Run CreateRunKey(string value)
        {
            return new Run
            {
                Foreground = new SolidColorBrush(Colors.Black),
                FontSize = 14,
                Text = value,
                FontWeight = FontWeights.DemiBold,
            };
        }


        public event PropertyChangedEventHandler PropertyChanged;

        protected void OnBindUIPropertyChanged(string propertyName)
        {
            PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(propertyName));
        }

    }

    /// <summary>
    /// Interaction logic for CaptionDesc.xaml
    /// </summary>
    public partial class CaptionDesc : UserControl
    {
        private CaptionDescViewMode viewModel;
        public CaptionDesc()
        {
            this.Resources.MergedDictionaries.Add(SharedDictionaryManager.StringResource);
            this.Resources.MergedDictionaries.Add(SharedDictionaryManager.UnifiedBtnStyle);

            InitializeComponent();
            this.DataContext = viewModel = new CaptionDescViewMode(this);
        }

        /// <summary>
        ///  ViewModel for CaptionDesc.xaml
        /// </summary>
        public CaptionDescViewMode ViewModel { get => viewModel; set => this.DataContext = viewModel = value; }
    }
}
