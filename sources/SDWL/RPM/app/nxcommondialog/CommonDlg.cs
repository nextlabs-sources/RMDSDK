using nxcommondialog.helper;
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Drawing;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;

namespace nxcommondialog
{
    [ComVisible(true)]
    [Guid("1FF5E368-8747-4BE0-B45F-96E91765EBD2")]
    [ClassInterface(ClassInterfaceType.None)]
    public class CommonDialog : ICommonDialog
    {
        public DialogResult ShowProtectDlg(string filePath,
            string iconName,
            string dlgTitle,
            string actionBtnName,
            NxlEnabledProtectMethod enabledProtectMethod,
            out string jsonSelectedTags,
            out long rights,
            out string watermarkText,
            out NxlExpiration expiration)
        {
            // Init output paras
            jsonSelectedTags = "";
            rights = 0;
            watermarkText = "";
            expiration = new NxlExpiration()
            {
                type = NxlExpiryType.NEVER_EXPIRE,
                Start = 0,
                End = 0
            };

            if (string.IsNullOrEmpty(filePath))
            {
                Trace.WriteLine(" -----> Error: the file path can not be empty.");
                return DialogResult.Error;
            }

            DialogResult res = DialogResult.None;
            try
            {
                #region Need to init blow by calling sdk.
                SdkHandler sdkHandler = new SdkHandler();
                string watermark = sdkHandler.RmsWaterMarkInfo.text;
                WinFormControlLibrary.Expiration exp = DataConvert.SdkExpt2ControlLibExpt(sdkHandler.RmsExpiration);
                WinFormControlLibrary.Classification[] classifications = DataConvert.SdkTag2FrmTag(sdkHandler.SysBucketClassifications);
                #endregion // Need to init blow by calling sdk.

                Bitmap ficon = CommonUtils.GetFileIcon(filePath, iconName);

                BuildFrmRightsData build = new BuildFrmRightsData(ficon, filePath, enabledProtectMethod, watermark, exp, classifications, actionBtnName);
                RightsSelect rs = new RightsSelect(filePath, build.DataModel, dlgTitle);

                string out_jsonTags = "";
                List<NxlFileRights> out_rights = new List<NxlFileRights>();
                string out_watermark = "";
                NxlExpiration out_exp;
                res = rs.ShowDialog(out out_jsonTags, out out_rights, out out_watermark, out out_exp, actionBtnName);

                jsonSelectedTags = out_jsonTags;
                rights = DataConvert.ListEnumRights2Long(out_rights);
                watermarkText = out_watermark;
                expiration = out_exp;
            }
            catch (Exception e)
            {
                Trace.WriteLine(e.Message); 
                return DialogResult.Error;
            }

            return res;
        }

        public DialogResult ShowFileInfoDlg(string filepath, string displayname, string dlgTitle)
        {
            if (string.IsNullOrEmpty(filepath))
            {
                Trace.WriteLine(" -----> Error: the file path can not be empty.");
                return DialogResult.Error;
            }

            DialogResult res = DialogResult.None;
            try
            {
                SdkHandler sdkHandler = new SdkHandler();
                NxlFilePermission filePermission = new NxlFilePermission(sdkHandler);
                filePermission.FileInfoShowDlg(filepath, displayname, dlgTitle);
                res = DialogResult.Close;
            }
            catch (Exception e)
            {
                Trace.WriteLine(e.Message);
                return DialogResult.Error;
            }

            return res;
        }

    }

    #region Data model that needed in exported interface
    public enum NxlEnabledProtectMethod
    {
        Enabled_CompanyDefined = 0,
        Enabled_UserDefined = 1,
        Enabled_All = 2
    }

    public enum NxlFileRights
    {
        RIGHT_VIEW = 0x1,
        RIGHT_EDIT = 0x2,
        RIGHT_PRINT = 0x4,
        RIGHT_CLIPBOARD = 0x8,
        RIGHT_SAVEAS = 0x10,
        RIGHT_DECRYPT = 0x20,
        RIGHT_SCREENCAPTURE = 0x40,
        RIGHT_SEND = 0x80,
        RIGHT_CLASSIFY = 0x100,
        RIGHT_SHARE = 0x200,
        RIGHT_DOWNLOAD = 0x400,
        RIGHT_WATERMARK = 0x40000000
    }

    public enum NxlExpiryType
    {
        NEVER_EXPIRE = 0,
        RELATIVE_EXPIRE,
        ABSOLUTE_EXPIRE,
        RANGE_EXPIRE,
    }
    public struct NxlExpiration
    {
        public NxlExpiryType type;
        public Int64 Start;
        public Int64 End;
    }

    public enum DialogResult
    {
        /// <summary>
        /// Nothing is returned from the dialog box. This means that the modal dialog continues running.
        /// </summary>
        None = 0,

        /// <summary>
        ///  The dialog box return value is positive content (Yes\OK\Protect).
        /// </summary>
        Positive = 1,

        /// <summary>
        ///  The dialog box return value is Negative content (No\Cancel).
        /// </summary>
        Negative = 2,

        /// <summary>
        /// The dialog box is closed by Close button or 'X' button.
        /// </summary>
        Close = 3,

        /// <summary>
        /// Failed to show dialog since some reasion such as invalid parameter, or other exception.
        /// </summary>
        Error = 4
    }
    #endregion // Data model that needed in exported interface.

}
