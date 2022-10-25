using CustomControls.common.sharedResource;
using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
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
    #region DataModel
    public class FileModel
    {
        public string Name { get; set; }
        public string FullName { get; set; }
    }
    #endregion

    #region Convert

    static class FileIconSupportHelper
    {
        /// <summary>
        /// Used to judge using the new file type icon .png format
        /// </summary>
        /// <param name="fileType"></param>
        /// <returns></returns>
        public static bool IsSupportFileTypeEx(string fileType)
        {
            bool result = false;

            if (string.Equals(fileType, "3dxml", StringComparison.CurrentCultureIgnoreCase)
                    || string.Equals(fileType, "bmp", StringComparison.CurrentCultureIgnoreCase)
                    || string.Equals(fileType, "c", StringComparison.CurrentCultureIgnoreCase)
                    || string.Equals(fileType, "catpart", StringComparison.CurrentCultureIgnoreCase)
                    || string.Equals(fileType, "catshape", StringComparison.CurrentCultureIgnoreCase)
                    || string.Equals(fileType, "cgr", StringComparison.CurrentCultureIgnoreCase)
                    || string.Equals(fileType, "cpp", StringComparison.CurrentCultureIgnoreCase)
                    || string.Equals(fileType, "csv", StringComparison.CurrentCultureIgnoreCase)
                    || string.Equals(fileType, "doc", StringComparison.CurrentCultureIgnoreCase)
                    || string.Equals(fileType, "docm", StringComparison.CurrentCultureIgnoreCase)
                    || string.Equals(fileType, "docx", StringComparison.CurrentCultureIgnoreCase)
                    || string.Equals(fileType, "dotx", StringComparison.CurrentCultureIgnoreCase)
                    || string.Equals(fileType, "dwg", StringComparison.CurrentCultureIgnoreCase)
                    || string.Equals(fileType, "dxf", StringComparison.CurrentCultureIgnoreCase)
                    || string.Equals(fileType, "err", StringComparison.CurrentCultureIgnoreCase)
                    || string.Equals(fileType, "exe", StringComparison.CurrentCultureIgnoreCase)
                    || string.Equals(fileType, "ext", StringComparison.CurrentCultureIgnoreCase)
                    || string.Equals(fileType, "file", StringComparison.CurrentCultureIgnoreCase)
                    || string.Equals(fileType, "gdoc", StringComparison.CurrentCultureIgnoreCase)
                    || string.Equals(fileType, "gdra", StringComparison.CurrentCultureIgnoreCase)
                    || string.Equals(fileType, "gif", StringComparison.CurrentCultureIgnoreCase)
                    || string.Equals(fileType, "gshe", StringComparison.CurrentCultureIgnoreCase)
                    || string.Equals(fileType, "gsli", StringComparison.CurrentCultureIgnoreCase)
                    || string.Equals(fileType, "h", StringComparison.CurrentCultureIgnoreCase)
                    || string.Equals(fileType, "hsf", StringComparison.CurrentCultureIgnoreCase)
                    || string.Equals(fileType, "htm", StringComparison.CurrentCultureIgnoreCase)
                    || string.Equals(fileType, "html", StringComparison.CurrentCultureIgnoreCase)
                    || string.Equals(fileType, "hwf", StringComparison.CurrentCultureIgnoreCase)
                    || string.Equals(fileType, "iges", StringComparison.CurrentCultureIgnoreCase)
                    || string.Equals(fileType, "igs", StringComparison.CurrentCultureIgnoreCase)
                    || string.Equals(fileType, "ipt", StringComparison.CurrentCultureIgnoreCase)
                    || string.Equals(fileType, "java", StringComparison.CurrentCultureIgnoreCase)
                    || string.Equals(fileType, "jpg", StringComparison.CurrentCultureIgnoreCase)
                    || string.Equals(fileType, "js", StringComparison.CurrentCultureIgnoreCase)
                    || string.Equals(fileType, "json", StringComparison.CurrentCultureIgnoreCase)
                    || string.Equals(fileType, "jt", StringComparison.CurrentCultureIgnoreCase)
                    || string.Equals(fileType, "key", StringComparison.CurrentCultureIgnoreCase)
                    || string.Equals(fileType, "log", StringComparison.CurrentCultureIgnoreCase)
                    || string.Equals(fileType, "m", StringComparison.CurrentCultureIgnoreCase)
                    || string.Equals(fileType, "md", StringComparison.CurrentCultureIgnoreCase)
                    || string.Equals(fileType, "model", StringComparison.CurrentCultureIgnoreCase)
                    || string.Equals(fileType, "mov", StringComparison.CurrentCultureIgnoreCase)
                    || string.Equals(fileType, "mp3", StringComparison.CurrentCultureIgnoreCase)
                    || string.Equals(fileType, "mp4", StringComparison.CurrentCultureIgnoreCase)
                    || string.Equals(fileType, "numb", StringComparison.CurrentCultureIgnoreCase)
                    || string.Equals(fileType, "page", StringComparison.CurrentCultureIgnoreCase)
                    || string.Equals(fileType, "par", StringComparison.CurrentCultureIgnoreCase)
                    || string.Equals(fileType, "pdf", StringComparison.CurrentCultureIgnoreCase)
                    || string.Equals(fileType, "png", StringComparison.CurrentCultureIgnoreCase)
                    || string.Equals(fileType, "potm", StringComparison.CurrentCultureIgnoreCase)
                    || string.Equals(fileType, "potx", StringComparison.CurrentCultureIgnoreCase)
                    || string.Equals(fileType, "ppt", StringComparison.CurrentCultureIgnoreCase)
                    || string.Equals(fileType, "pptx", StringComparison.CurrentCultureIgnoreCase)
                    || string.Equals(fileType, "properties", StringComparison.CurrentCultureIgnoreCase)
                    || string.Equals(fileType, "prt", StringComparison.CurrentCultureIgnoreCase)
                    || string.Equals(fileType, "psm", StringComparison.CurrentCultureIgnoreCase)
                    || string.Equals(fileType, "py", StringComparison.CurrentCultureIgnoreCase)
                    || string.Equals(fileType, "rft", StringComparison.CurrentCultureIgnoreCase)
                    || string.Equals(fileType, "rh", StringComparison.CurrentCultureIgnoreCase)
                    || string.Equals(fileType, "rtf", StringComparison.CurrentCultureIgnoreCase)
                    || string.Equals(fileType, "sldasm", StringComparison.CurrentCultureIgnoreCase)
                    || string.Equals(fileType, "sldprt", StringComparison.CurrentCultureIgnoreCase)
                    || string.Equals(fileType, "sql", StringComparison.CurrentCultureIgnoreCase)
                    || string.Equals(fileType, "step", StringComparison.CurrentCultureIgnoreCase)
                    || string.Equals(fileType, "stl", StringComparison.CurrentCultureIgnoreCase)
                    || string.Equals(fileType, "stp", StringComparison.CurrentCultureIgnoreCase)
                    || string.Equals(fileType, "swift", StringComparison.CurrentCultureIgnoreCase)
                    || string.Equals(fileType, "tif", StringComparison.CurrentCultureIgnoreCase)
                    || string.Equals(fileType, "tiff", StringComparison.CurrentCultureIgnoreCase)
                    || string.Equals(fileType, "txt", StringComparison.CurrentCultureIgnoreCase)
                    || string.Equals(fileType, "vb", StringComparison.CurrentCultureIgnoreCase)
                    || string.Equals(fileType, "vds", StringComparison.CurrentCultureIgnoreCase)
                    || string.Equals(fileType, "vsd", StringComparison.CurrentCultureIgnoreCase)
                    || string.Equals(fileType, "vsdx", StringComparison.CurrentCultureIgnoreCase)
                    || string.Equals(fileType, "x_b", StringComparison.CurrentCultureIgnoreCase)
                    || string.Equals(fileType, "x_t", StringComparison.CurrentCultureIgnoreCase)
                    || string.Equals(fileType, "xls", StringComparison.CurrentCultureIgnoreCase)
                    || string.Equals(fileType, "xlsb", StringComparison.CurrentCultureIgnoreCase)
                    || string.Equals(fileType, "xlsm", StringComparison.CurrentCultureIgnoreCase)
                    || string.Equals(fileType, "xlsx", StringComparison.CurrentCultureIgnoreCase)
                    || string.Equals(fileType, "xlt", StringComparison.CurrentCultureIgnoreCase)
                    || string.Equals(fileType, "xltm", StringComparison.CurrentCultureIgnoreCase)
                    || string.Equals(fileType, "xltx", StringComparison.CurrentCultureIgnoreCase)
                    || string.Equals(fileType, "xml", StringComparison.CurrentCultureIgnoreCase)
                    || string.Equals(fileType, "xmt_txt", StringComparison.CurrentCultureIgnoreCase)
                    || string.Equals(fileType, "zip", StringComparison.CurrentCultureIgnoreCase)
                    )
            {
                result = true;
            }
            return result;
        }
    }

    /// <summary>
    /// List file type icon
    /// </summary>
    public class ListFile2IconConverterEx : IValueConverter
    {
        public object Convert(object value, Type targetType, object parameter, CultureInfo culture)
        {
            try
            {
                string name = (string)value;

                string originFilename = name;

                bool isNxl = System.IO.Path.GetExtension(originFilename).Equals(".nxl");

                if (isNxl)
                {
                    int lastindex = name.LastIndexOf('.');
                    if (lastindex != -1)
                    {
                        originFilename = name.Substring(0, lastindex); // remove .nxl
                    }
                }

                string fileType = System.IO.Path.GetExtension(originFilename); // return .txt or null or empty
                if (string.IsNullOrEmpty(fileType))
                {
                    fileType = "---";
                }
                else if (fileType.IndexOf('.') != -1)
                {
                    fileType = fileType.Substring(fileType.IndexOf('.') + 1).ToLower();
                    if (!FileIconSupportHelper.IsSupportFileTypeEx(fileType))
                    {
                        fileType = "---";
                    }
                }
                else
                {
                    fileType = "---";
                }

                string uritemp = "";
                if (isNxl)
                {
                    uritemp = string.Format(@"/CustomControls;component/resources/icons/fileTypeIcons/{0}_G.png", fileType.ToUpper());
                }
                else
                {
                    uritemp = string.Format(@"/CustomControls;component/resources/icons/fileTypeIcons/{0}.png", fileType.ToUpper());
                }
                var stream = new Uri(uritemp, UriKind.Relative);
                return new BitmapImage(stream);
            }
            catch (Exception e)
            {
                Console.WriteLine(e.ToString());

                return new BitmapImage(new Uri(@"/CustomControls;component/resources/icons/fileTypeIcons/---_G.png", UriKind.Relative));
            }

        }

        public object ConvertBack(object value, Type targetType, object parameter, CultureInfo culture)
        {
            throw new NotImplementedException();
        }
    }
    #endregion

    /// <summary>
    /// ViewModel for CaptionDesc4.xaml
    /// </summary>
    public class CaptionDesc4ViewMode : INotifyPropertyChanged
    {
        private CaptionDesc4 host;
        private string mTitle = "test Title";
        private Visibility mTitleVisible = Visibility.Visible;
        private string selectFileDesc = "";
        private int fileCount = 1;
        private ObservableCollection<FileModel> fileNameList = new ObservableCollection<FileModel>();

        public CaptionDesc4ViewMode(CaptionDesc4 captionDesc)
        {
            host = captionDesc;
            selectFileDesc = host.TryFindResource("CapDesc_SelectFile").ToString();
        }
        /// <summary>
        /// Title, defult value is 'test Title'
        /// </summary>
        public string Title { get => mTitle; set { mTitle = value; OnBindUIPropertyChanged("Title"); } }

        /// <summary>
        /// Title visibility, defult value is Visible
        /// </summary>
        public Visibility TitleVisible { get => mTitleVisible; set { mTitleVisible = value; OnBindUIPropertyChanged("TitleVisible"); } }

        /// <summary>
        /// Select file describe, defult value is 'Selected file'
        /// </summary>
        public string SelectFileDesc { get => selectFileDesc; set { selectFileDesc = value; OnBindUIPropertyChanged("SelectFileDesc"); } }

        /// <summary>
        /// File count, defult value is 1.
        /// </summary>
        public int FileCount { get => fileCount; set { fileCount = value; OnBindUIPropertyChanged("FileCount"); } }

        /// <summary>
        /// File name list
        /// </summary>
        public ObservableCollection<FileModel> FileNameList { get => fileNameList; }


        public event PropertyChangedEventHandler PropertyChanged;

        protected void OnBindUIPropertyChanged(string propertyName)
        {
            PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(propertyName));
        }

    }


    /// <summary>
    /// Interaction logic for CaptionDesc4.xaml
    /// </summary>
    public partial class CaptionDesc4 : UserControl
    {
        private CaptionDesc4ViewMode viewModel;
        public CaptionDesc4()
        {
            this.Resources.MergedDictionaries.Add(SharedDictionaryManager.StringResource);

            InitializeComponent();

            this.DataContext = viewModel = new CaptionDesc4ViewMode(this);
        }

        /// <summary>
        ///  ViewModel for CaptionDesc4.xaml
        /// </summary>
        public CaptionDesc4ViewMode ViewModel { get => viewModel; set => this.DataContext = viewModel = value; }
    }
}
