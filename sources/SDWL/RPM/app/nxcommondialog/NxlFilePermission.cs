using CommonDialog.sdk;
using nxcommondialog.helper;
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using WinFormControlLibrary;

namespace nxcommondialog
{
    class NxlFilePermission
    {
        private SdkHandler handler;
        public NxlFilePermission(SdkHandler sdkHandler)
        {
            this.handler = sdkHandler;
        }

        public void FileInfoShowDlg(string filePath, string dispname, string title)
        {
            string displayPath = dispname;
            if (string.IsNullOrEmpty(displayPath))
            {
                displayPath = filePath.EndsWith(".nxl") ? Path.GetFileName(filePath) : (Path.GetFileName(filePath) + ".nxl");
            }

            try
            {
                handler.GetRPMFileRights(filePath, out List<FileRights> rights, out WaterMarkInfo waterMark);
                Dictionary<string, List<string>> tags = handler.ReadFileTags(filePath);


                BuildFrmFileInfoData build = new BuildFrmFileInfoData(displayPath, tags, DataConvert.SdkRights2FrmRights(rights.ToArray()), waterMark.text);
                FrmFileInfo frmFileInfo = new FrmFileInfo(build.DataMode);
                frmFileInfo.DlgTitle = !string.IsNullOrEmpty(title) ? title : "NextLabs SkyDRM";
                frmFileInfo.ShowDialog();
            }
            catch (Exception e)
            {
                throw e;
            }
        }
    }

    class BuildFrmFileInfoData
    {
        private FileRightsInfoDataModel dataMode;

        public BuildFrmFileInfoData(string filePath, Dictionary<string, List<string>> tags, HashSet<Rights> filerRights, string warterMark)
        {
            dataMode = new FileRightsInfoDataModel()
            {
                FilePath = filePath,
                FileTags = tags,
                Filerights = filerRights,
                Wartemark = warterMark,
                IsModifyBtnVisible = false
            };
        }

        public FileRightsInfoDataModel DataMode { get => dataMode; }

    }
}

