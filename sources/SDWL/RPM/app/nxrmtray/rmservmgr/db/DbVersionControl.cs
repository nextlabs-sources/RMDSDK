using ServiceManager.rmservmgr.db.config;
using System;
using System.Collections.Generic;
using System.Data.SQLite;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace ServiceManager.rmservmgr.db
{
    public class DbVersionControl
    {
        private static readonly int db_version = 0;

        public void DetectVersion(string DataBaseConnectionString)
        {
            OnCreateDatabase(DataBaseConnectionString);

            // todo, db's Upgrade or Downgrade
        }

        public void OnCreateDatabase(string DataBaseConnectionString)
        {
            using (SQLiteConnection connection = new SQLiteConnection(DataBaseConnectionString))
            {
                connection.Open();

                using (SQLiteCommand command = new SQLiteCommand(connection))
                {
                    // table server
                    command.CommandText = Config.SQL_Create_Table_Server;
                    command.ExecuteNonQuery();

                    // table user
                    command.CommandText = Config.SQL_Create_Table_User;
                    command.ExecuteNonQuery();

                    // table RecentNotification
                    command.CommandText = Config.SQL_Create_Table_RecentNotification;
                    command.ExecuteNonQuery();

                    // table RecentNotifyApp
                    command.CommandText = Config.SQL_Create_Table_RecentNotifyApp;
                    command.ExecuteNonQuery();
                }
            }
        }

    }
}
