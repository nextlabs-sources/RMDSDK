using System;
using System.Collections.Generic;
using System.Data.SQLite;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace ServiceManager.rmservmgr.db.table
{
    public class User
    {
        public int Id { get; set; }
        public int Server_id { get; set; }
        public string Name { get; set; }
        public string Email { get; set; }
        public string Pass_code { get; set; }
        public int Login_counts { get; set; }
        public DateTime Last_access { get; set; }
        public DateTime Last_logout { get; set; }
        public string Avartar { get; set; }
        public int Rms_user_id { get; set; }
        public int Rms_user_type { get; set; }
        public DateTime Rms_session_expiration { get; set; }
        public string Rms_nxl_watermark_setting { get; set; }
        public string Rms_nxl_expiration_setting { get; set; }
        public string Rms_user_json { get; set; }
        public string Rms_quota_setting_json { get; set; }
        public string User_preference_setting_json { get; set; }

        public User()
        {
        }

        public static User NewByReader(SQLiteDataReader reader)
        {
            return new User()
            {
                Id = int.Parse(reader["id"].ToString()),
                Name = reader["name"].ToString(),
                Email = reader["email"].ToString(),
                Pass_code = reader["pass_code"].ToString(),
                Login_counts = int.Parse(reader["login_counts"].ToString()),
                Last_access = DateTime.Parse(reader["last_access"].ToString()),
                Last_logout = DateTime.Parse(reader["last_logout"].ToString()),
                Rms_user_id = int.Parse(reader["rms_user_id"].ToString()),
                Rms_user_type = int.Parse(reader["rms_user_type"].ToString()),
                Rms_session_expiration = DateTime.Parse(reader["rms_session_expiration"].ToString()),
                Rms_nxl_watermark_setting = reader["rms_nxl_watermark_setting"].ToString(),
                Rms_nxl_expiration_setting = reader["rms_nxl_expiration_setting"].ToString(),
                Rms_user_json = reader["rms_user_json"].ToString(),
                Rms_quota_setting_json = reader["rms_quota_setting_json"].ToString(),
                User_preference_setting_json = reader["user_preference_setting_json"].ToString()
            };
        }

    }

    public class UserDao
    {
        public static readonly string SQL_Create_Table_User = @"
             CREATE TABLE IF NOT EXISTS User(
                  id                            integer         NOT NULL PRIMARY KEY AUTOINCREMENT UNIQUE, 
                  name                          varchar(255)    NOT NULL default '', 
                  email                         varchar(255)    NOT NULL default '', 
                  pass_code                     varchar(255)    NOT NULL default '', 
                  server_id                     integer         NOT NULL, 
                  login_counts                  integer         default 1,
                  last_access                   DateTime        default current_timestamp, 
                  last_logout                   DateTime        default current_timestamp, 
                  avartar                       text            default '',
                  rms_user_id                   integer         default 0,
                  rms_user_type                 integer         default 0, 
                  rms_session_expiration        DateTime        default current_timestamp,
                  rms_nxl_watermark_setting     varchar(255)    default '', 
                  rms_nxl_expiration_setting    varchar(255)    default '', 
                  rms_user_json                 text            default '',  
                  rms_quota_setting_json        text            default '',
                  user_preference_setting_json  text            default '',

                  unique(email,server_id),
                  foreign key(server_id) references Server(id) on delete cascade);
        ";

        static public KeyValuePair<String, SQLiteParameter[]> Upsert_SQL(
                int rms_user_id,
                string name,
                string email,
                string passcode,
                int server_id,
                int rms_user_type,
                string rms_user_raw_json,
                string rms_nxl_watermark_setting, string rms_nxl_expiration_setting,
                string rms_quota_setting_json, string user_preference_setting_json)
        {
            string sql = @"
                INSERT INTO
                    User( name,  email,  pass_code,  server_id,  rms_user_id,  rms_user_type,  
                            rms_user_json,rms_nxl_watermark_setting,
                            rms_nxl_expiration_setting,rms_quota_setting_json,
                            user_preference_setting_json)
                    VALUES (  @name, @email, @pass_code, @server_id, @rms_user_id, @rms_user_type, 
                            @rms_user_json,@rms_nxl_watermark_setting,
                            @rms_nxl_expiration_setting,@rms_quota_setting_json,
                            @user_preference_setting_json)
                ON CONFLICT(email,server_id)
                   DO UPDATE SET
                      name=excluded.name,
                      pass_code=excluded.pass_code, 
                      rms_user_id=excluded.rms_user_id,
                      rms_user_type=excluded.rms_user_type,
                      rms_user_json=excluded.rms_user_json,

                      login_counts=login_counts+1,
                      last_access=current_timestamp;
              
                -- find talbe_user id
                select id,login_counts from user 
                where  email=@email and server_id=@server_id and rms_user_id=@rms_user_id;
            ";
            SQLiteParameter[] parameters = {
                new SQLiteParameter("@name",name),
                new SQLiteParameter("@email",email),
                new SQLiteParameter("@pass_code",passcode),
                new SQLiteParameter("@server_id",server_id),
                new SQLiteParameter("@rms_user_id",rms_user_id),
                new SQLiteParameter("@rms_user_type",rms_user_type),
                new SQLiteParameter("@rms_user_json",rms_user_raw_json),
                new SQLiteParameter("@rms_nxl_watermark_setting",rms_nxl_watermark_setting),
                new SQLiteParameter("@rms_nxl_expiration_setting",rms_nxl_expiration_setting),
                new SQLiteParameter("@rms_quota_setting_json",rms_quota_setting_json),
                new SQLiteParameter("@user_preference_setting_json",user_preference_setting_json),

            };
            return new KeyValuePair<string, SQLiteParameter[]>(sql, parameters);
        }

        static public KeyValuePair<String, SQLiteParameter[]> Query_User(int primary_key)
        {
            string sql = @"
                SELECT 
                    *
                FROM 
                    User
                WHERE   
                    id = @primary_key;
            ";
            SQLiteParameter[] parameters = {
                new SQLiteParameter("@primary_key",primary_key)
            };
            return new KeyValuePair<string, SQLiteParameter[]>(sql, parameters);
        }


        static public KeyValuePair<String, SQLiteParameter[]> Update_Watermark(int primary_key, string watermark)
        {
            string sql = @"
                UPDATE 
                    User 
                SET 
                  rms_nxl_watermark_setting = @watermark
                WHERE 
                  id=@primary_key;
            ";
            SQLiteParameter[] parameters = {
                new SQLiteParameter("@primary_key",primary_key),
                new SQLiteParameter("@watermark",watermark),
            };
            return new KeyValuePair<string, SQLiteParameter[]>(sql, parameters);
        }

        static public KeyValuePair<String, SQLiteParameter[]> Update_Expiration(int primary_key, string expiration)
        {
            string sql = @"
                UPDATE 
                    User 
                SET 
                  rms_nxl_expiration_setting = @expiration
                WHERE 
                  id=@primary_key;
            ";
            SQLiteParameter[] parameters = {
                new SQLiteParameter("@primary_key",primary_key),
                new SQLiteParameter("@expiration",expiration),
            };
            return new KeyValuePair<string, SQLiteParameter[]>(sql, parameters);
        }

        static public KeyValuePair<String, SQLiteParameter[]> Update_Preference(int primary_key, string preference)
        {
            string sql = @"
                UPDATE 
                    User 
                SET 
                  user_preference_setting_json = @preference
                WHERE 
                  id=@primary_key;
            ";
            SQLiteParameter[] parameters = {
                new SQLiteParameter("@primary_key",primary_key),
                new SQLiteParameter("@preference",preference),
            };
            return new KeyValuePair<string, SQLiteParameter[]>(sql, parameters);
        }



        static public KeyValuePair<String, SQLiteParameter[]> Update_Auto_SQL(string email, int rms_user_id, int server_id)
        {
            string sql = @"
                UPDATE user 
                Set               
                    last_access=current_timestamp
                    where  email=@email and server_id=@server_id and rms_user_id=@rms_user_id; 
                -------
                -- find talbe_user id
                SELECT id,login_counts FROM user 
                WHERE  email=@email and server_id=@server_id and rms_user_id=@rms_user_id;
            ";
            SQLiteParameter[] parameters = {
                new SQLiteParameter("@email",email),
                new SQLiteParameter("@server_id",server_id),
                new SQLiteParameter("@rms_user_id",rms_user_id)
            };
            return new KeyValuePair<string, SQLiteParameter[]>(sql, parameters);
        }


        static public KeyValuePair<String, SQLiteParameter[]> Update_LastLogout_SQL(int primary_key)
        {
            string sql = @"
                update user
                Set 
                    last_logout=current_timestamp
                where
                    id=@id;
            ";
            SQLiteParameter[] parameters = {
                new SQLiteParameter("@id",primary_key)
            };
            return new KeyValuePair<string, SQLiteParameter[]>(sql, parameters);
        }


        static public KeyValuePair<String, SQLiteParameter[]> Update_Name_SQL(string new_name, int primary_key)
        {
            string sql = @"
                update user
                Set 
                    name=@new_name
                where
                    id=@id;
            ";
            SQLiteParameter[] parameters = {
                new SQLiteParameter("@new_name",new_name),
                new SQLiteParameter("@id",primary_key)
            };
            return new KeyValuePair<string, SQLiteParameter[]>(sql, parameters);
        }


        static public KeyValuePair<String, SQLiteParameter[]> Query_User_Id_SQL(string email, int Server_Primary_Key)
        {
            string sql = @"
                SELECT rms_user_id
                FROM 
                    user
                where
                   email=@email and  server_id=@server_id;
            ";
            SQLiteParameter[] parameters = {
                 new SQLiteParameter("@email",email),
                new SQLiteParameter("@server_id",Server_Primary_Key)
            };
            return new KeyValuePair<string, SQLiteParameter[]>(sql, parameters);
        }

        public static KeyValuePair<String, SQLiteParameter[]> Query_User_Login_Counts_SQL(string email, int Server_Primary_Key)
        {
            string sql = @"
                SELECT login_counts
                FROM 
                    user
                where
                   email=@email and  server_id=@server_id;
            ";
            SQLiteParameter[] parameters = {
                 new SQLiteParameter("@email",email),
                new SQLiteParameter("@server_id",Server_Primary_Key)
            };
            return new KeyValuePair<string, SQLiteParameter[]>(sql, parameters);
        }
    }
}
