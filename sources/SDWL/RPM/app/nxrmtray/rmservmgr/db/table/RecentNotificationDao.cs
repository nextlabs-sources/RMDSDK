using System;
using System.Collections.Generic;
using System.Data.SQLite;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace ServiceManager.rmservmgr.db.table
{
    public class RecentNotification
    {
        private int id;
        private int user_table_pk;
        private string messageJson;
        private DateTime last_modified_time = DateTime.Now;
        private string reserved1;
        private string reserved2;
        private string reserved3;

        public RecentNotification()
        {}

        public int Id { get => id; }
        public int User_Table_Pk { get => user_table_pk; }
        public string MessageJson { get => messageJson; }
        public DateTime Last_modified_time { get => last_modified_time; set => last_modified_time = value; }
        public string Reserved1 { get => reserved1; }
        public string Reserved2 { get => reserved2; }
        public string Reserved3 { get => reserved3; }

        public static RecentNotification NewByReader(SQLiteDataReader reader)
        {
            var rt = new RecentNotification()
            {
                id = Int32.Parse(reader["id"].ToString()),
                user_table_pk = Int32.Parse(reader["user_table_pk"].ToString()),
                messageJson = reader["messageJson"].ToString(),
                last_modified_time = DateTime.Parse(reader["last_modified_time"].ToString()),
                reserved1 = "",
                reserved2 = "",
                reserved3 = "",
            };
            //DateTime.TryParse(reader["last_modified_time"].ToString(), out rt.last_modified_time);
            return rt;
        }
    }

    public class RecentNotificationDao
    {
        public static readonly string SQL_Create_Table_RecentNotification = @"
                CREATE TABLE IF NOT EXISTS RecentNotification (
                   id                  integer          NOT NULL PRIMARY KEY AUTOINCREMENT UNIQUE , 
                   user_table_pk       integer          default 0,         
                   messageJson              text     NOT NULL default '', 
                   last_modified_time  DateTime         NOT NULL default (datetime('now','localtime')),
                   reserved1                 text      DEFAULT '',
                   reserved2                 text      DEFAULT '',
                   reserved3                 text      DEFAULT '',
                       
                   foreign key(user_table_pk) references User(id) on delete cascade);
        ";

        public static KeyValuePair<String, SQLiteParameter[]> Insert_SQL(int user_table_pk, string messageJson)
        {
            string sql = @"
                        INSERT INTO  
                                RecentNotification(user_table_pk,messageJson)
                        VALUES 
                                (@user_table_pk,@messageJson);

                        SELECT * FROM
                                RecentNotification
                        WHERE
                                user_table_pk =@user_table_pk AND messageJson = @messageJson;
                ";

            SQLiteParameter[] parameters = {
                   new SQLiteParameter("@user_table_pk" , user_table_pk),
                   new SQLiteParameter("@messageJson" , messageJson)
            };

            return new KeyValuePair<string, SQLiteParameter[]>(sql, parameters);
        }

        public static KeyValuePair<String, SQLiteParameter[]> Update_SQL(int id, string messageJson)
        {

            string sql = @"
                   UPDATE RecentNotification 
                        SET 
                            messageJson=@messageJson,
                            last_modified_time=datetime('now','localtime')
                                         
                        WHERE
                            id = @id;
                ";

            SQLiteParameter[] parameters = {
                   new SQLiteParameter("@id" , id),
                   new SQLiteParameter("@messageJson" , messageJson)
            };

            return new KeyValuePair<string, SQLiteParameter[]>(sql, parameters);
        }

        public static KeyValuePair<String, SQLiteParameter[]> Delete_SQL(int id)
        {

            string sql = @"
                   DELETE FROM
                        RecentNotification 
                   WHERE
                        id = @id;
                ";

            SQLiteParameter[] parameters = {
                   new SQLiteParameter("@id" , id)
            };

            return new KeyValuePair<string, SQLiteParameter[]>(sql, parameters);
        }

        public static KeyValuePair<String, SQLiteParameter[]> DeleteAll_SQL(int user_table_pk)
        {

            string sql = @"
                   DELETE FROM
                        RecentNotification 
                   WHERE
                        user_table_pk=@user_table_pk ;
                ";

            SQLiteParameter[] parameters = {
                   new SQLiteParameter("@user_table_pk" , user_table_pk)
            };

            return new KeyValuePair<string, SQLiteParameter[]>(sql, parameters);
        }

        public static KeyValuePair<String, SQLiteParameter[]> Query_SQL(int user_table_pk)
        {
            string sql = @"
              SELECT   
                *
            FROM
                RecentNotification
            WHERE
                 user_table_pk=@user_table_pk 
            AND 
              ( julianday('now','localtime') - julianday(last_modified_time) <= 30)
            ;";

            SQLiteParameter[] parameters = {
                new SQLiteParameter("@user_table_pk",user_table_pk)
            };

            return new KeyValuePair<string, SQLiteParameter[]>(sql, parameters);
        }
    }
}
