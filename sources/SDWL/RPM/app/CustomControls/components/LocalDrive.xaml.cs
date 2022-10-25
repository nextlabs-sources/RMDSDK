using CustomControls.common.helper;
using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.ComponentModel;
using System.Diagnostics;
using System.Globalization;
using System.IO;
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
    #region Data Model
    public interface IDriveFile
    {
        string Name { get; set; }
        string PathId { get; set; }
        string DisplayPath { get; set; }
        bool IsFolder { get; }
    }
    public class DriveFile : IDriveFile
    {
        public string Name { get; set; }
        public string PathId { get; set; }
        public string DisplayPath { get; set; }
        public bool IsFolder => false;
    }
    public class DriveFolder : IDriveFile
    {
        public string Name { get; set; }
        public string PathId { get; set; }
        public string DisplayPath { get; set; }
        public bool IsFolder => true;
    }

    public class LocalDriveRepo
    {
        public DriveFolder CurrentWorkingFolder { get; set; } = new DriveFolder() { PathId="/", DisplayPath="/" };
        public List<IDriveFile> GetWorkingFolderFiles()
        {
            List<IDriveFile> ret = new List<IDriveFile>();

            InnerGetFilesFromDrive(CurrentWorkingFolder.PathId, ret);

            return ret;
        }
        /// <summary>
        /// Inner impl to get files from drive, including folders and files.
        /// </summary>
        /// <param name="pathId"></param>
        /// <returns></returns>
        private void InnerGetFilesFromDrive(string pathId, List<IDriveFile> results)
        {
            try
            {
                Debug.WriteLine("GetFilesFromLocalDrive");
                if (pathId.StartsWith("This PC") || pathId.Equals("/"))
                {
                    results.AddRange(GetThisPCList());
                }
                else if (Directory.Exists(pathId))
                {
                    GetSonFoldersAndFiles(pathId, results);
                }
                Debug.WriteLine("GetFilesFromLocalDrive successfully");
            }
            catch (Exception e)
            {
                Debug.WriteLine(e.ToString());
            }
        }
        private List<IDriveFile> GetThisPCList()
        {
            List<IDriveFile> ret = new List<IDriveFile>();
            var drives = DriveInfo.GetDrives();

            foreach (var drive in drives)
            {
                if (drive.DriveType != System.IO.DriveType.Fixed
                    && drive.DriveType != System.IO.DriveType.Network)
                {
                    continue;
                }

                string flagLabel = string.Empty;
                if (drive.DriveType == System.IO.DriveType.Network)
                {
                    flagLabel = "Network";
                }
                else
                {
                    flagLabel = drive.VolumeLabel;
                }
                
                string driverName = drive.RootDirectory.FullName.Split(new char[] { '\\' }, StringSplitOptions.RemoveEmptyEntries)[0];
                // Here treat the drive as a folder
                DriveFolder localDrive = new DriveFolder();

                localDrive.Name = flagLabel + "(" + driverName + ")";
                localDrive.PathId = drive.RootDirectory.FullName;
                localDrive.DisplayPath = drive.RootDirectory.FullName;

                ret.Add(localDrive);
            }

            // add desktop         
            string path = Environment.GetFolderPath(System.Environment.SpecialFolder.Desktop);
            DriveFolder desktop = new DriveFolder();
            desktop.Name = "Desktop";
            desktop.PathId = path;
            desktop.DisplayPath = path;
            ret.Add(desktop);


            // add documents
            string documentPath = Environment.GetFolderPath(System.Environment.SpecialFolder.MyDocuments);
            DriveFolder document = new DriveFolder();
            document.Name = "Documents";
            document.PathId = documentPath;
            document.DisplayPath = documentPath;
            ret.Add(document);

            return ret;
        }
        private static void GetSonFoldersAndFiles(string directory, List<IDriveFile> collections)
        {
            List<IDriveFile> directoryList = new List<IDriveFile>();
            List<IDriveFile> fileList = new List<IDriveFile>();
            string[] resourceArray = Directory.GetFileSystemEntries(directory);
            foreach (var item in resourceArray)
            {
                try
                {
                    if (Directory.Exists(item))
                    {
                        // filter hidden folder
                        DirectoryInfo directoryInfo = new DirectoryInfo(item);
                        if (directoryInfo.Attributes.ToString().Contains(System.IO.FileAttributes.Hidden.ToString()))
                        {
                            continue;
                        }
                        DriveFolder folder = new DriveFolder();
                        folder.Name = directoryInfo.Name;
                        folder.PathId = directoryInfo.FullName;
                        folder.DisplayPath = directoryInfo.FullName;

                        directoryList.Add(folder);
                    }
                    else
                    {
                        // filter hidden file
                        FileInfo fileInfo = new FileInfo(item);
                        if (fileInfo.Attributes.ToString().Contains(System.IO.FileAttributes.Hidden.ToString()))
                        {
                            continue;
                        }
                        DriveFile file = new DriveFile();
                        file.Name = fileInfo.Name;
                        file.PathId = fileInfo.FullName;
                        file.DisplayPath = fileInfo.FullName;

                        fileList.Add(file);
                    }
                }
                catch (Exception ex)
                {
                    continue;
                }
            }
            // Add the folder first and then add the file
            collections.AddRange(directoryList);
            collections.AddRange(fileList);
        }
    }

    public class SelectedFolderItem : INotifyPropertyChanged
    {
        private IDriveFile folder;
        private bool isCanSelect;
        private Visibility showSlash;

        public SelectedFolderItem(IDriveFile _folder, bool _canSelect = true, Visibility _showSlash = Visibility.Visible)
        {
            folder = _folder;
            isCanSelect = _canSelect;
            showSlash = _showSlash;
        }

        public IDriveFile Folder { get => folder; }
        public bool IsCanSelect { get => isCanSelect; set { isCanSelect = value; OnPropertyChanged("IsCanSelect"); } }
        public Visibility ShowSlash { get => showSlash; set { showSlash = value; OnPropertyChanged("ShowSlash"); } }


        public event PropertyChangedEventHandler PropertyChanged;
        protected void OnPropertyChanged(string propertyName)
        {
            PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(propertyName));
        }
    }
    #endregion

    #region Convert
    public class EmptyVisibilityConverter : IValueConverter
    {
        public object Convert(object value, Type targetType, object parameter, CultureInfo culture)
        {
            int count = (int)value;

            if (count > 0)
            {
                return Visibility.Collapsed;
            }

            return Visibility.Visible;
        }

        public object ConvertBack(object value, Type targetType, object parameter, CultureInfo culture)
        {
            throw new NotImplementedException();
        }
    }
    #endregion

    /// <summary>
    /// LocalDrive.xaml  DataCommands
    /// </summary>
    public class LD_DataCommands
    {
        private static RoutedCommand browser;

        static LD_DataCommands()
        {
            browser = new RoutedCommand(
              "Browser", typeof(LD_DataCommands));
        }
        /// <summary>
        /// LocalDrive.xaml browser button command
        /// </summary>
        public static RoutedCommand Browser
        {
            get { return browser; }
        }
    }

    /// <summary>
    /// ViewModel for LocalDrive.xaml
    /// </summary>
    public class LocalDriveViewModel : INotifyPropertyChanged
    {
        #region DataBinding
        private ObservableCollection<SelectedFolderItem> selectedPaths = new ObservableCollection<SelectedFolderItem>();
        private ObservableCollection<IDriveFile> fileList = new ObservableCollection<IDriveFile>();

        /// <summary>
        /// (Internal binding) Record the selected folder path
        /// </summary>
        public ObservableCollection<SelectedFolderItem> SelectedPaths { get => selectedPaths; }

        /// <summary>
        /// (Internal binding to ListBox) File ListBox Itemsource. if ListBox have CheckBox, user can use FileList item IsChecked property to get selected files.
        /// if ListBox don't have CheckBox and SelectionMode is Single, user can use FileSelectViewModel.CurrentSelectFile property to get file.
        /// </summary>
        public ObservableCollection<IDriveFile> FileList { get => fileList; }

        /// <summary>
        /// LocalDrive repo, use to get local drive file
        /// </summary>
        public LocalDriveRepo LocalDriveRepo { get; }

        /// <summary>
        /// Used to record the selected folder in the file list
        /// </summary>
        public IDriveFile CurrentSelectFolder { get; set; }

        /// <summary>
        /// (Internal binding) Switch selected folder command
        /// </summary>
        public DelegateCommand SwitchSelectedFolderCommand { get; set; }



        public event PropertyChangedEventHandler PropertyChanged;
        protected void OnPropertyChanged(string propertyName)
        {
            PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(propertyName));
        }
        #endregion

        public LocalDriveViewModel()
        {
            LocalDriveRepo = new LocalDriveRepo();

            LocalDriveRepo.CurrentWorkingFolder = new DriveFolder() { PathId="/", DisplayPath = "/" };
            CurrentSelectFolder = LocalDriveRepo.CurrentWorkingFolder;

            // init file list
            var filePool = LocalDriveRepo.GetWorkingFolderFiles();
            var filePool2 = filePool.OrderBy(f => f.Name).OrderByDescending(f => f.IsFolder);
            SetFileList(filePool2.ToList());

            // init selected path list
            DriveFolder currentFolder = LocalDriveRepo.CurrentWorkingFolder;
            currentFolder.Name = "This PC";
            SelectedFolderItem folderItem = new SelectedFolderItem(currentFolder, true, Visibility.Collapsed);
            SelectedPaths.Add(folderItem);

            SwitchSelectedFolderCommand = new DelegateCommand(SwitchSelectedFolder);
        }

        #region Binding Command Dispatch
        private void SwitchSelectedFolder(object args)
        {
            SelectedFolderItem selectedItem = args as SelectedFolderItem;

            // update SelectedPaths list
            List<SelectedFolderItem> recordSelected = new List<SelectedFolderItem>();
            foreach (var item in SelectedPaths)
            {
                if (item.Folder.Equals(selectedItem.Folder))
                {
                    recordSelected.Add(item);
                    break;
                }
                else
                {
                    recordSelected.Add(item);
                }
            }

            SelectedPaths.Clear();
            foreach (var item in recordSelected)
            {
                SelectedPaths.Add(item);
            }

            CurrentSelectFolder = selectedItem.Folder;
            // update file list
            LocalDriveRepo.CurrentWorkingFolder = (DriveFolder)selectedItem.Folder;
            var filePool = LocalDriveRepo.GetWorkingFolderFiles();
            var filePool2 = filePool.OrderBy(f => f.Name).OrderByDescending(f => f.IsFolder);
            SetFileList(filePool2.ToList());

        }
        #endregion

        #region Event
        internal void FileListItem_MouseDoubleClick(object sender, MouseButtonEventArgs e)
        {
            ListBoxItem selectedItem = sender as ListBoxItem;
            IDriveFile selectedFile = (IDriveFile)selectedItem.Content;
            if (selectedFile.IsFolder)
            {
                CurrentSelectFolder = selectedFile;
                // record current working repo working folder
                LocalDriveRepo.CurrentWorkingFolder = (DriveFolder)selectedFile;

                // update file list
                var filePool = LocalDriveRepo.GetWorkingFolderFiles();
                var filePool2 = filePool.OrderBy(f => f.Name).OrderByDescending(f => f.IsFolder);
                SetFileList(filePool2.ToList());

                // add select path
                SelectedFolderItem selectedFolder = new SelectedFolderItem(LocalDriveRepo.CurrentWorkingFolder);
                SelectedPaths.Add(selectedFolder);
            }
        }

        internal void FileListItem_Selected(object sender, RoutedEventArgs e)
        {
            ListBoxItem selectedItem = sender as ListBoxItem;
            IDriveFile selectedFile = (IDriveFile)selectedItem.Content;
            if (selectedFile.IsFolder)
            {
                CurrentSelectFolder = selectedFile;
            }
        }
        #endregion

        private void SetFileList(IList<IDriveFile> files)
        {
            FileList.Clear();
            foreach (var item in files)
            {
                if (item.IsFolder)
                {
                    FileList.Add(item);
                }
            }
        }
    }

    /// <summary>
    /// Interaction logic for LocalDrive.xaml
    /// </summary>
    public partial class LocalDrive : UserControl
    {
        private LocalDriveViewModel viewModel;
        public LocalDrive()
        {
            InitializeComponent();

            this.DataContext = viewModel = new LocalDriveViewModel();
        }

        /// <summary>
        /// ViewModel for LocalDrive.xaml
        /// </summary>
        public LocalDriveViewModel ViewModel { get => viewModel; set => this.DataContext = viewModel = value; }

        private void FileListItem_MouseDoubleClick(object sender, MouseButtonEventArgs e)
        {
            ViewModel.FileListItem_MouseDoubleClick(sender, e);
        }

        private void FileListItem_Selected(object sender, RoutedEventArgs e)
        {
            ViewModel.FileListItem_Selected(sender, e);
        }

    }
}
