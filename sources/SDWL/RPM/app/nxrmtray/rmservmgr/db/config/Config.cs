using ServiceManager.rmservmgr.db.table;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace ServiceManager.rmservmgr.db.config
{
    class Config
    {
        public static readonly string Database_Name = "servmgr.db";
        public static readonly string ConnectionString = "Data Source=" + Database_Name;

        public static readonly string SQL_Create_Table_Server = ServerDao.SQL_Create_Table_Server;
        public static readonly string SQL_Create_Table_User = UserDao.SQL_Create_Table_User;
        public static readonly string SQL_Create_Table_RecentNotification = RecentNotificationDao.SQL_Create_Table_RecentNotification;
        public static readonly string SQL_Create_Table_RecentNotifyApp = RecentNotifyAppDao.SQL_Create_Table_RecentNotifyApp;
    }
}
