using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using CommonDialog.sdk;
using CommonDialog.sdk.helper;
using nxcommondialog.helper;

namespace nxcommondialog
{
   class SdkHandler
    {
        private Session session;
        public Session Rmsdk { get => session; }
        public bool SysBucketIsEnableAdhoc { get; private set; }
        public ProjectClassification[] SysBucketClassifications { get; private set; }
        public WaterMarkInfo RmsWaterMarkInfo { get; private set; }
        public Expiration RmsExpiration { get; private set; }
        public bool bRecoverSucceed { get; set; }

        public SdkHandler()
        {
            try
            {
                SdkLibInit();

                bRecoverSucceed = RecoverSession();
                if (bRecoverSucceed)
                {
                    InitSystemBucket();
                    GetDocumentPreference();
                }
                else
                {
                    Trace.WriteLine(" -----> Error: Failed to get current logged in user.");
                }
            }
            catch (Exception e)
            {
                throw e;
            }
        }

        private void SdkLibInit()
        {
            try
            {
                Apis.SdkLibInit();
            }
            catch (Exception e)
            {
                Trace.WriteLine(" -----> Error: Failed to initialize sdk.");
                throw e;
            }
        }
        private void SdkLibCleanup()
        {
            Apis.SdkLibCleanup();
        }

        private bool RecoverSession()
        {
            return Apis.GetCurrentLoggedInUser(out session);
        }

        private void InitSystemBucket()
        {
            string sbTenant = Rmsdk.User.GetSystemProjectTenantId();
            SysBucketClassifications = Rmsdk.User.GetProjectClassification(sbTenant);
            //SysBucketIsEnableAdhoc = Rmsdk.User.IsEnabledAdhocForSystemBucket();
        }

        private void GetDocumentPreference()
        {
            try
            {
                //invoke SDWLResult GetUserPreference
                
                Expiration eprn;
                string watermark;

                Rmsdk.User.GetPreference(out eprn,
                    out watermark);

                if (watermark != null)
                {
                    WaterMarkInfo waterMarkInfo = new WaterMarkInfo();
                    waterMarkInfo.text = watermark;

                    //set Watermark
                    RmsWaterMarkInfo = waterMarkInfo;
                }

                //set expiration
                RmsExpiration = eprn;
            }
            catch (Exception e)
            {
                throw e;
            }
        }


        #region Get RPMFileRights
        public void GetRPMFileRights(string plainFilePath, out List<FileRights> rights, out WaterMarkInfo watermark)
        {
            Rmsdk.RPMGetFileRights(plainFilePath, out rights, out watermark);
        }

        public Dictionary<string, List<string>> ReadFileTags(string plainFilePath)
        {
            string tags = Rmsdk.RPMReadFileTags(plainFilePath);
            return Utils.ParseClassificationTag(tags);
        }
        #endregion

    }
}
