using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using Newtonsoft.Json;
using ServiceManager.resources.languages;
using ServiceManager.rmservmgr.sdk;

namespace ServiceManager.rmservmgr.app.user
{
    public struct UserPreference
    {
        public int heartBeatIntervalSec;
    }

    public class User : IUser
    {
        private ServiceManagerApp app = ServiceManagerApp.Singleton;
        private db.table.User raw;
        private UserPreference preference;

        private string rmsdk_home_path;
        private string RPM_folder_path;
        private string home_path;
        // work around for avoiding mutil threads call uplaod at onece        
        public bool closeDoor_UploadLog = false;

        public User(db.table.User raw)
        {
            this.raw = raw;
            this.preference = JsonConvert.DeserializeObject<UserPreference>(raw.User_preference_setting_json);

            // RPM path
            var path = app.Config.WorkingFolder;
            path += "\\home\\";
            var host = new Uri(app.Session.GetCurrentTenant().RMSURL).Host;
            path += host + "\\" + Email;
            // Make sure user working folder exist:
            Directory.CreateDirectory(path);
            home_path = path;
            RPM_folder_path = path + "\\RPM";


            // Retrive RMSdk's user folder;
            var sdkhome = app.Config.RmSdkFolder;
            var tenant = app.Session.GetCurrentTenant().Name;
            rmsdk_home_path = sdkhome + "\\" + tenant + "\\" + RmsUserId;

            GetDocumentPreference();

            closeDoor_UploadLog = false;
        }

        public string WorkingFolder => home_path;

        public string SDkWorkingFolder => rmsdk_home_path;

        public int RmsUserId => raw.Rms_user_id;

        public string Name => raw.Name;

        public string Email => raw.Email;

        public UserType UserType => (UserType)raw.Rms_user_type;

        public bool LeaveCopy { get => app.Config.LeaveCopy; set => app.Config.LeaveCopy = value; }

        public bool ShowNotifyWindow { get => app.Config.ShowNotifyWin; set => app.Config.ShowNotifyWin = value; }

        public WaterMarkInfo Watermark { get => GetWaterMark(); set => UpdateNxlWaterMark(value); }

        public Expiration Expiration { get => GetExpiration(); set => UpdateNxlExpiration(value); }

        public void GetDocumentPreference()
        {
            try
            {
                //invoke SDWLResult GetUserPreference
                Expiration eprn;
                string watermark;

                app.Session.User.GetPreference(out eprn,
                    out watermark);

                if (watermark != null)
                {
                    WaterMarkInfo waterMarkInfo = new WaterMarkInfo();
                    waterMarkInfo.text = watermark;

                    //set Watermark
                    Watermark = waterMarkInfo;
                }

                //set expiration
                Expiration = eprn;
            }
            catch (Exception msg)
            {
                app.Log.Error(msg.ToString());
            }
        }

        public void UpdateDocumentPreference()
        {
            try
            {
                //invoke SDWLResult UpdateUserPreference
                Expiration eprn;
                string watermark;

                eprn = Expiration;
                watermark = Watermark.text;

                app.Session.User.SetPreference(eprn, watermark);
            }
            catch (Exception msg)
            {
                app.Log.Error("Error in UpdateDocumentPreference:",msg);
            }
        }

        public int HeartBeatIntervalSec => GetHeartBeatIntervalSec();

        public void OnHeartBeat()
        {
            // sync project, policy bundle, user attributes
            WaterMarkInfo wmf;
            Int32 nHeartBeatFrequence;
            app.Session.User.SyncHeartBeatInfo(out wmf, out nHeartBeatFrequence);
            // Update new value into app level config.
            app.Config.HeartBeatIntervalSec = nHeartBeatFrequence;
            // returive user settings from rms by sdk
            GetDocumentPreference();
            // returive user name
            GetUserName();
        }

        #region User private Method
        private void GetUserName()
        {
            app.Session.User.UpdateUserInfo();
            string value = app.Session.User.Name;
            if (raw.Name != value)
            {
                raw.Name = value;
                // update db
                app.DBProvider.UpdateUserName(value);
            }
        }

        private void UpdateDbUserPrefence()
        {
            app.DBProvider.UpdateUserPreference(
                JsonConvert.SerializeObject(preference));
        }

        private WaterMarkInfo GetWaterMark()
        {
            var e = raw.Rms_nxl_watermark_setting;
            try
            {
                return JsonConvert.DeserializeObject<WaterMarkInfo>(e);
            }
            catch (Exception msg)
            {
                app.Log.Error("Error in DeserializeObject<WaterMarkInfo>:", msg);
            }

            // give a default value
            var rt = new WaterMarkInfo();
            rt.text = "$(User)$(Break)$(Date)$(Time)";
            raw.Rms_nxl_watermark_setting = JsonConvert.SerializeObject(rt);
            // update db;
            app.DBProvider.UpdateUserWaterMark(raw.Rms_nxl_watermark_setting);
            return rt;
        }

        private void UpdateNxlWaterMark(WaterMarkInfo s)
        {
            string json = JsonConvert.SerializeObject(s);
            if (raw.Rms_nxl_watermark_setting.Equals(json))
            {
                return;
            }
            // update cache
            raw.Rms_nxl_watermark_setting = json;
            // update db;
            app.DBProvider.UpdateUserWaterMark(json);
        }

        private Expiration GetExpiration()
        {
            var e = raw.Rms_nxl_expiration_setting;
            try
            {
                return JsonConvert.DeserializeObject<Expiration>(e);
            }
            catch (Exception msg)
            {
                app.Log.Error("Error in DeserializeObject<Expiration>:", msg);
            }

            // give a default value
            var rt = new Expiration();
            raw.Rms_nxl_expiration_setting = JsonConvert.SerializeObject(rt);
            // update db;
            app.DBProvider.UpdateUserExpiration(raw.Rms_nxl_expiration_setting);
            return rt;
        }

        private void UpdateNxlExpiration(Expiration e)
        {
            string json = JsonConvert.SerializeObject(e);
            if (raw.Rms_nxl_expiration_setting.Equals(json))
            {
                return;
            }
            // update cache
            raw.Rms_nxl_expiration_setting = json;
            // update db;
            app.DBProvider.UpdateUserExpiration(json);
        }

        private int GetHeartBeatIntervalSec()
        {
            //SkydrmLocalApp.Singleton.Config.GetRegistryLocalApp();
            var sec = app.Config.HeartBeatIntervalSec;

            //if HeartBeatIntervalSec registry modified by other software or user 
            if (sec != preference.heartBeatIntervalSec)
            {
                preference.heartBeatIntervalSec = sec;
                //update database
                UpdateDbUserPrefence();

                return preference.heartBeatIntervalSec;
            }
            return preference.heartBeatIntervalSec;
        }

        #endregion

    }

}
