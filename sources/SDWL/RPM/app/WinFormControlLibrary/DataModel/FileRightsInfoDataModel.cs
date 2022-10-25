using System;
using System.Collections.Generic;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace WinFormControlLibrary
{
    /// <summary>
    /// FrmFileInfo.cs DataModel
    /// </summary>
    public class FileRightsInfoDataModel
    {
        private Bitmap fileIcon = Properties.Resources.nxl;
        private string filePath = "";

        private bool cpStackPanelVisible = true;
        private Dictionary<string, List<string>> fileTags = new Dictionary<string, List<string>>();
        private HashSet<Rights> filerights = new HashSet<Rights>();
        private string wartemark = "";

        private bool isModifyBtnVisible = false;

        /// <summary>
        /// File icon, defult icon is "nxl.png".
        /// </summary>
        public Bitmap FileIcon { get => fileIcon; set => fileIcon = value; }

        /// <summary>
        /// File Path,defult value is "".
        /// </summary>
        public string FilePath { get => filePath; set => filePath = value; }

        /// <summary>
        /// Central Policy StackPanel visible, false is Collapsed, true is Visible, defult value is true
        /// </summary>
        public bool CpStackPanelVisible { get => cpStackPanelVisible; set => cpStackPanelVisible = value; }

        /// <summary>
        /// File tags
        /// </summary>
        public Dictionary<string, List<string>> FileTags { get => fileTags; set => fileTags = value; }

        /// <summary>
        /// File rights
        /// </summary>
        public HashSet<Rights> Filerights { get => filerights; set => filerights = value; }

        /// <summary>
        /// WarterMark value
        /// </summary>
        public string Wartemark { get => wartemark; set => wartemark = value; }

        /// <summary>
        /// Modify rights button isVisible, defult value is false
        /// </summary>
        public bool IsModifyBtnVisible { get => isModifyBtnVisible; set => isModifyBtnVisible = value; }
        
    }
}
