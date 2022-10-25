using System;
using System.Collections.Generic;
using System.Data.SQLite;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace ServiceManager.rmservmgr.db.table
{
    public class RecentNotifyApp
    {
        private int id;
        private int user_table_pk;
        private string application;
        private int isDisplay;
        private string reserved1;
        private string reserved2;
        private string reserved3;

        public int Id { get => id; }
        public int User_table_pk { get => user_table_pk; }
        public string Application { get => application; }
        public int IsDisplay { get => isDisplay; set => isDisplay = value; }
        public string Reserved1 { get => reserved1; }
        public string Reserved2 { get => reserved2; }
        public string Reserved3 { get => reserved3; }

        public static RecentNotifyApp NewByReader(SQLiteDataReader reader)
        {
            var rt = new RecentNotifyApp()
            {
                id = Int32.Parse(reader["id"].ToString()),
                user_table_pk = Int32.Parse(reader["user_table_pk"].ToString()),
                application = reader["application"].ToString(),
                isDisplay = Int32.Parse(reader["isDisplay"].ToString()),
                reserved1 = "",
                reserved2 = "",
                reserved3 = "",
            };
            //DateTime.TryParse(reader["last_modified_time"].ToString(), out rt.last_modified_time);
            return rt;
        }
    }

    public class RecentNotifyAppDao
    {
        public static readonly string SQL_Create_Table_RecentNotifyApp = @"
                CREATE TABLE IF NOT EXISTS RecentNotifyApp (
                   id                  integer          NOT NULL PRIMARY KEY AUTOINCREMENT UNIQUE , 
                   user_table_pk       integer          default 0,         
                   application              text     NOT NULL default '' COLLATE NOCASE, 
                   isDisplay        integer          default 1,
                   reserved1                 text      DEFAULT '',
                   reserved2                 text      DEFAULT '',
                   reserved3                 text      DEFAULT '',
                       
                   UNIQUE(user_table_pk,application)
                   foreign key(user_table_pk) references User(id) on delete cascade);
        ";

        public static KeyValuePair<String, SQLiteParameter[]> Upsert_SQL(int user_table_pk, string application)
        {
            string sql = @"
                        INSERT INTO  
                                RecentNotifyApp(user_table_pk,application)
                        VALUES 
                                (@user_table_pk,@application)
                        ON CONFLICT(user_table_pk,application)
                          DO UPDATE SET
                             reserved1='';
                ;";

            SQLiteParameter[] parameters = {
                   new SQLiteParameter("@user_table_pk" , user_table_pk),
                   new SQLiteParameter("@application" , application)
            };

            return new KeyValuePair<string, SQLiteParameter[]>(sql, parameters);
        }

        public static KeyValuePair<String, SQLiteParameter[]> Update_SQL(int id, bool isDisplay)
        {
            string sql = @"
                   UPDATE RecentNotifyApp 
                        SET 
                            isDisplay=@isDisplay
                        WHERE
                            id = @id;
                ";

            SQLiteParameter[] parameters = {
                   new SQLiteParameter("@id" , id),
                   new SQLiteParameter("@isDisplay" , isDisplay ? 1:0)
            };

            return new KeyValuePair<string, SQLiteParameter[]>(sql, parameters);
        }

        public static KeyValuePair<String, SQLiteParameter[]> Delete_SQL(int id)
        {

            string sql = @"
                   DELETE FROM
                        RecentNotifyApp 
                   WHERE
                        id = @id;
                ";

            SQLiteParameter[] parameters = {
                   new SQLiteParameter("@id" , id)
            };

            return new KeyValuePair<string, SQLiteParameter[]>(sql, parameters);
        }

        public static KeyValuePair<String, SQLiteParameter[]> Query_SQL(int user_table_pk)
        {
            string sql = @"
              SELECT   
                *
            FROM
                RecentNotifyApp
            WHERE
                 user_table_pk=@user_table_pk;
                ";

            SQLiteParameter[] parameters = {
                new SQLiteParameter("@user_table_pk",user_table_pk)
            };

            return new KeyValuePair<string, SQLiteParameter[]>(sql, parameters);
        }

    }

}
