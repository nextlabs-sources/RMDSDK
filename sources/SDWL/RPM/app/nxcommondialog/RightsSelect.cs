using nxcommondialog.helper;
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using WinFormControlLibrary;

namespace nxcommondialog
{
    class RightsSelect
    {
        private string plainFilePath;
        private FrmRightsSelect protectFrmRights;

        // central policy
        private string jsonSelectedtags;

        // adhoc
        private List<NxlFileRights> rights;
        private string watermarkText;
        private NxlExpiration expiration;

        // dialog result
        private DialogResult dialogResult;

        public RightsSelect(string filepath, RightsSelectDataModel dataModel, string title)
        {
            Init();

            plainFilePath = filepath;
            protectFrmRights = new FrmRightsSelect(dataModel);
            protectFrmRights.DlgTitle = !string.IsNullOrEmpty(title) ? title : "NextLabs SkyDRM";
            protectFrmRights.PositiveBtnEvent += ProtectFrmRights_PositiveBtnEvent;
            protectFrmRights.CancelBtnEvent += ProtectFrmRights_CancelBtnEvent;
        }

        public DialogResult ShowDialog(
            out string out_jsonSelTags,
            out List<NxlFileRights> out_rights,
            out string out_watermarkText,
            out NxlExpiration out_expiration,
            string actionBtnName = "Protect")
        {
            // Initial output parameters
            out_jsonSelTags = jsonSelectedtags;
            out_rights = rights;
            out_watermarkText = watermarkText;
            out_expiration = expiration;

            try
            {
                // Block here and then show a model dialog to wait for user to operate.
                protectFrmRights.ShowDialog();

                if(dialogResult == DialogResult.Positive)
                {
                    // out values.
                    out_jsonSelTags = jsonSelectedtags;
                    out_rights = rights;
                    out_watermarkText = watermarkText;
                    out_expiration = expiration;
                }
            }
            catch (Exception e)
            {
                Debug.WriteLine(e.Message);
            }

            return dialogResult;
        }

        private void Init()
        {
            jsonSelectedtags = "";
            rights = new List<NxlFileRights>();
            watermarkText = "";
            expiration = new NxlExpiration()
            {
                type = NxlExpiryType.NEVER_EXPIRE,
                Start = 0,
                End = 0
            };
            dialogResult = DialogResult.Negative;
        }

        private void ProtectFrmRights_PositiveBtnEvent(object sender, EventArgs e)
        {
            dialogResult = DialogResult.Positive;
            if (protectFrmRights.DataModel.AdhocRadioDefultChecked) // adhoc
            {
                rights = DataConvert.FrmRights2CommonDlgRights(protectFrmRights.DataModel.SelectedRights);

                if (rights.Contains(NxlFileRights.RIGHT_WATERMARK))
                {
                    watermarkText = protectFrmRights.DataModel.Watermark;
                }
                expiration = DataConvert.FrmExpt2CommonDlgExpt(protectFrmRights.DataModel.Expiry);
            }
            else
            {
                UserSelectTags tags = new UserSelectTags();
                foreach (var item in protectFrmRights.DataModel.SelectedTags)
                {
                    tags.AddTag(item.Key, item.Value);
                }

                jsonSelectedtags = tags.ToJsonString();
            }

            protectFrmRights.Close();
        }

        private void ProtectFrmRights_CancelBtnEvent(object sender, EventArgs e)
        {
            dialogResult = DialogResult.Negative;
            protectFrmRights.Close();
        }

    }

    class BuildFrmRightsData
    {
        private RightsSelectDataModel dataModel;
        public RightsSelectDataModel DataModel { get => dataModel; }

        // Used for protect normal file
        public BuildFrmRightsData(Bitmap fileIcon, string filePath, NxlEnabledProtectMethod enabledPM, string waterMark, WinFormControlLibrary.Expiration expiration,
            Classification[] classifications, string positiveBtnContent, string cancelBtnContent = "Cancel",
            bool infoTextVisible = true, bool skipBtnVisible = false, bool positiveBtnIsEnable = true)
        {
            bool adRdIsEnable = (enabledPM == NxlEnabledProtectMethod.Enabled_UserDefined ||
                enabledPM == NxlEnabledProtectMethod.Enabled_All);

            bool centralRdIsEnable = (enabledPM == NxlEnabledProtectMethod.Enabled_CompanyDefined ||
                enabledPM == NxlEnabledProtectMethod.Enabled_All);

            dataModel = new RightsSelectDataModel()
            {
                FileIcon = fileIcon,
                FilePath = filePath,
                AdhocRadioIsEnable = adRdIsEnable,
                CentralRadioIsEnable = centralRdIsEnable,
                Classifications = classifications,
                IsInfoTextVisible = infoTextVisible,
                IsSkipBtnVisible = skipBtnVisible,
                IsPositiveBtnIsEnable = positiveBtnIsEnable,
                PositiveBtnContent = positiveBtnContent,
                CancelBtnContent = cancelBtnContent
            };

            if (!centralRdIsEnable)
            {
                dataModel.AdhocRadioDefultChecked = true;
            }

            if (string.IsNullOrEmpty(positiveBtnContent))
            {
                dataModel.PositiveBtnContent = "Protect";
            }

            if (classifications.Length == 0)
            {
                dataModel.IsWarningVisible = true;
            }

            if (adRdIsEnable)
            {
                dataModel.Watermark = waterMark;
                dataModel.Expiry = expiration; 
            }
        }

    }
}
