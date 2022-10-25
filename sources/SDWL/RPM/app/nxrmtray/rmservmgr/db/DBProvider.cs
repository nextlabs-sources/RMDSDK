using Newtonsoft.Json;
using ServiceManager.rmservmgr.db.config;
using ServiceManager.rmservmgr.db.table;
using System;
using System.Collections.Generic;
using System.Data.SQLite;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace ServiceManager.rmservmgr.db
{
    public class DBProvider
    {
        private ServiceManagerApp app = ServiceManagerApp.Singleton;
        string DataBasePath;
        string DataBaseConnectionString;

        int Server_Primary_Key;
        int User_Primary_Key;
        int User_Login_Counts = 0;

        private DbVersionControl versionControl = new DbVersionControl();

        #region Init
        public DBProvider(string DbPath)
        {

            DataBasePath = Path.Combine(DbPath, Config.Database_Name);
            // by omsond, normally, we should set busy_timeout as 10s,
            // but there must be some unoptimized code, so I set it as 60s
            DataBaseConnectionString =
                @"Data Source=" + DataBasePath + ";foreign_keys=true;busy_timeout=60000;";

            // detect db version & create db.
            versionControl.DetectVersion(DataBaseConnectionString);
        }
        #endregion

        #region User Session
        // for new user login
        public void OnUserLogin(int rms_user_id,
                                string name,
                                string email,
                                string passcode,
                                int rms_user_type,
                                string rms_user_raw_json)
        {

            string defaultWaterMark = JsonConvert.SerializeObject(new sdk.WaterMarkInfo() { text = "$(User)$(Break)$(Date)$(Time)" });
            string defaultExpiration = JsonConvert.SerializeObject(new sdk.Expiration());
            string defaultQuota = JsonConvert.SerializeObject(new app.user.Quota());
            string defaultPreference = JsonConvert.SerializeObject(new app.user.UserPreference()
            {
                heartBeatIntervalSec = sdk.Config.Deault_Heartbeat,
            });

            ExecuteReader(UserDao.Upsert_SQL(rms_user_id,
                    name, email, passcode, Server_Primary_Key,
                    rms_user_type, rms_user_raw_json, defaultWaterMark,
                    defaultExpiration, defaultQuota, defaultPreference), (reader) =>
                    {
                        app.Log.Info("***********get this user's primary key ***********");
                        // get this user's primary key
                        if (reader.Read())
                        {
                            User_Primary_Key = Int32.Parse(reader[0].ToString());
                            User_Login_Counts = Int32.Parse(reader[1].ToString());
                            app.Log.Info("***********User_Primary_Key :***********" + User_Primary_Key);
                            app.Log.Info("***********User_Login_Counts :***********" + User_Login_Counts);
                        }
                    });

        }

        // locate server_id by router     
        // set serverID and userID as this user
        // return rms_user_id
        public int OnUserRecovered(string email, string router, string tenant)
        {
            // locate server_id
            int rms_user_id = -1;
            ExecuteReader(ServerDao.Query_ID_SQL(router, tenant), (reader) =>
            {
                if (reader.Read())
                {
                    // set current server
                    Server_Primary_Key = Int32.Parse(reader[0].ToString());
                }
            });

            //get rms_user_id
            ExecuteReader(UserDao.Query_User_Id_SQL(email, Server_Primary_Key), (reader) =>
            {
                if (reader.Read())
                {
                    // set rms_user_id
                    rms_user_id = Int32.Parse(reader[0].ToString());
                }
            });
            // update user
            ExecuteReader(UserDao.Update_Auto_SQL(email, rms_user_id, Server_Primary_Key), (reader) =>
            {
                if (reader.Read())
                {
                    // set current user
                    User_Primary_Key = Int32.Parse(reader[0].ToString());
                    User_Login_Counts = Int32.Parse(reader[1].ToString());
                }

                // mProjectCache = new SkydrmLocal.rmc.database.memcache.project.ProjectCacheProxy(DataBaseConnectionString, User_Primary_Key);
            });

            //Project pre-load tasks must be started after project id map has been filled.
            //FillMapOfProjectId2PK(() =>
            //{
            //    mProjectCache.OnInitialize();
            //});

            return rms_user_id;
        }

        public User GetUser()
        {
            User u = null;
            ExecuteReader(UserDao.Query_User(User_Primary_Key), (reader) =>
            {
                app.Log.Info("***********User_Primary_Key :***********" + User_Primary_Key);
                app.Log.Info("***********reader.HasRows :***********" + reader.HasRows);
                if (reader.Read())
                {
                    app.Log.Info("***********reader.Read() :***********" + "true");
                    u = User.NewByReader(reader);
                }
            });
            if (u == null)
            {
                app.Log.Info("***********User is null**********");

                throw new Exception("Critical Error,Get user failed");
            }
            return u;
        }

        public void OnUserLogout()
        {
            try
            {
                ExecuteNonQuery(UserDao.Update_LastLogout_SQL(User_Primary_Key));
                ExecuteNonQuery(ServerDao.Update_LastLogout_SQL(Server_Primary_Key));

                this.User_Primary_Key = -1;
            }
            catch(Exception e)
            {
                app.Log.Error(e.ToString());
            }
        }

        public void UpdateUserName(string new_name)
        {
            try
            {
                ExecuteNonQuery(UserDao.Update_Name_SQL(new_name, User_Primary_Key));
            }
            catch (Exception)
            {
                throw;
            }
        }

        public void UpdateUserWaterMark(string watermark)
        {
            ExecuteNonQuery(UserDao.Update_Watermark(User_Primary_Key, watermark));
        }

        public void UpdateUserExpiration(string expiration)
        {
            ExecuteNonQuery(UserDao.Update_Expiration(User_Primary_Key, expiration));
        }

        public void UpdateUserPreference(string preference)
        {
            ExecuteNonQuery(UserDao.Update_Preference(User_Primary_Key, preference));
        }
        #endregion // end UserSession

        #region Server Session
        public void UpsertServer(string router, string url, string tenand, bool isOnPremise)
        {
            ExecuteReader(ServerDao.Upsert_SQL(router, url, tenand, isOnPremise), (reader) =>
            {
                // get this server item's primary key
                if (reader.Read())
                {
                    Server_Primary_Key = Int32.Parse(reader[0].ToString());
                    ServiceManagerApp.Singleton.Log.Info("***********Server_Primary_Key :***********" + Server_Primary_Key);
                }
            });
        }

        //only select 5 dataitem by desc
        //display to user
        public List<string> GetRouterUrl()
        {
            List<string> result = new List<string>();

            ExecuteReader(ServerDao.Query_Router_Url_SQL(), (reader) =>
            {
                while (reader.Read())
                {
                    string router_url = reader["router_url"].ToString();

                    result.Add(router_url);
                }
            });
            return result;
        }

        //display to user, Url is a combination of router and tenant.
        public string GetUrl()
        {
            string result = "http://www.skydrm.com/";

            ExecuteReader(ServerDao.Query_Url_SQL(Server_Primary_Key), (reader) =>
            {
                while (reader.Read())
                {
                    result = reader["url"].ToString();
                }
            });
            return result;
        }
        #endregion

        #region RecentNotification Session
        public RecentNotification InsertRecentNotification(string messageJson)
        {
            RecentNotification result = new RecentNotification();
            ExecuteReader(RecentNotificationDao.Insert_SQL(User_Primary_Key, messageJson), (reader)=>
            {
                while (reader.Read())
                {
                    result = RecentNotification.NewByReader(reader);
                }
            });
            return result;
        }
        public void UpdateRecentNotification(int id, string messageJson)
        {
            ExecuteNonQuery(RecentNotificationDao.Update_SQL(id, messageJson));
        }
        public void DeleteRecentNotification(int id)
        {
            ExecuteNonQuery(RecentNotificationDao.Delete_SQL(id));
        }
        public void DeleteAllRecentNotification()
        {
            ExecuteNonQuery(RecentNotificationDao.DeleteAll_SQL(User_Primary_Key));
        }

        public List<RecentNotification> QueryRecentNotification()
        {
            List<RecentNotification> result = new List<RecentNotification>();
            ExecuteReader(RecentNotificationDao.Query_SQL(User_Primary_Key), (reader) =>
            {
                while (reader.Read())
                {
                    result.Add(RecentNotification.NewByReader(reader));
                }
            });
            return result;
        }
        #endregion

        #region RecentNotifyApp Session
        public void UpsertRecentNotifyApp(string application)
        {
            ExecuteNonQuery(RecentNotifyAppDao.Upsert_SQL(User_Primary_Key, application));
        }
        public void UpdateRecentNotifyApp(int id, bool isDisplay)
        {
            ExecuteNonQuery(RecentNotifyAppDao.Update_SQL(id, isDisplay));
        }
        public void DeleteRecentNotifyApp(int id)
        {
            ExecuteNonQuery(RecentNotifyAppDao.Delete_SQL(id));
        }
        public List<RecentNotifyApp> QueryRecentNotifyApp()
        {
            List<RecentNotifyApp> result = new List<RecentNotifyApp>();
            ExecuteReader(RecentNotifyAppDao.Query_SQL(User_Primary_Key), (reader) =>
            {
                while (reader.Read())
                {
                    result.Add(RecentNotifyApp.NewByReader(reader));
                }
            });
            return result;
        }
        #endregion

        #region DbEngine
        // forward to SqiliteOpenHelper
        private int ExecuteNonQuery(KeyValuePair<String, SQLiteParameter[]> pair)
        {
            return ExecuteNonQuery(pair.Key, pair.Value);
        }

        private int ExecuteNonQuery(string sql, SQLiteParameter[] parameters)
        {
            return SqliteOpenHelper.ExecuteNonQuery(DataBaseConnectionString, sql, parameters);
        }

        private int ExecuteNonQueryBatch(KeyValuePair<string, SQLiteParameter[]>[] sqls)
        {
            return SqliteOpenHelper.ExecuteNonQueryBatch(DataBaseConnectionString, sqls);
        }

        private void ExecuteReader(KeyValuePair<String, SQLiteParameter[]> pair,
            Action<SQLiteDataReader> action)
        {
            ExecuteReader(pair.Key, pair.Value, action);
        }

        private void ExecuteReader(string sql, SQLiteParameter[] parameters
            , Action<SQLiteDataReader> action)
        {
            SqliteOpenHelper.ExecuteReader(DataBaseConnectionString, sql, parameters, action);
        }

        #endregion

    }
}
