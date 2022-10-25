using System;
using System.Collections.Generic;
using System.Data.SQLite;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace ServiceManager.rmservmgr.db.table
{
    public class Server
    {
        public int Id { get; set; }
        public string Url { get; set; }
        public string Tenant_id { get; set; }
        public string Router_url { get; set; }
        public string Last_access { get; set; }
        public string Access_count { get; set; }
        public string Last_logout { get; set; }

    }

    public class ServerDao
    {
        public static readonly string SQL_Create_Table_Server = @"
             CREATE TABLE IF NOT EXISTS Server(
                id                  integer         NOT NULL PRIMARY KEY AUTOINCREMENT UNIQUE , 
                router_url          varchar(255)    NOT NULL default '', 
                url                 varchar(255)    NOT NULL default '', 
                tenand_id           varchar(255)    NOT NULL default '', 
                last_access         DateTime        default current_timestamp,
                last_logout         DateTime        default current_timestamp ,
                access_count        integer         default 1,
                is_onpremise        integer         default 1,                       

                UNIQUE(router_url,tenand_id)
                );
        ";

        // Using router and teandId as union primary key insteading of url -- fix bug 52730
        public static KeyValuePair<String, SQLiteParameter[]> Upsert_SQL(string router, string url, string tenand, bool isOnPremise)
        {
            string sql = @"
                INSERT INTO 
                    Server( url, router_url, tenand_id, is_onpremise)
                    Values(@url,@router_url, @tenand_id, @is_onpremise)
                 ON CONFLICT(tenand_id,router_url)
                    DO UPDATE SET 
                        url = excluded.url,
                        access_count = access_count+1,
                        last_access  = current_timestamp,
                        last_logout  = current_timestamp;          
                -- find id--
                Select id from Server
                where tenand_id=@tenand_id and router_url=@router_url;   
                
            ";
            SQLiteParameter[] parameters = {
                new SQLiteParameter("@url",url),
                new SQLiteParameter("@router_url", router),
                new SQLiteParameter("@tenand_id", tenand),
                new SQLiteParameter("@is_onpremise", isOnPremise==true?1:0)
            };

            return new KeyValuePair<string, SQLiteParameter[]>(sql, parameters);

        }

        public static KeyValuePair<String, SQLiteParameter[]> Query_ID_SQL(string router, string tenant)
        {
            string sql = @"
                select id 
                from server
                where 
                    router_url=@router_url and 
                    tenand_id = @tenand_id
                ;
            ";
            SQLiteParameter[] parameters = {
                new SQLiteParameter("@router_url",router),
                new SQLiteParameter("@tenand_id",tenant)
            };
            return new KeyValuePair<string, SQLiteParameter[]>(sql, parameters);
        }


        public static KeyValuePair<String, SQLiteParameter[]> Update_LastLogout_SQL(int primary_key)
        {
            string sql = @"
                update server
                set  last_logout=current_timestamp
                where id=@id;
            ";
            SQLiteParameter[] parameters = {
                new SQLiteParameter("@id",primary_key)
            };
            return new KeyValuePair<string, SQLiteParameter[]>(sql, parameters);
        }

        // Only select 5 dataitem by desc
        public static KeyValuePair<String, SQLiteParameter[]> Query_Router_Url_SQL()
        {
            string sql = @"
            SELECT router_url 
            FROM server 
            order by[last_access] desc limit 0,5;";
            SQLiteParameter[] parameters = { };
            return new KeyValuePair<string, SQLiteParameter[]>(sql, parameters);
        }

        // Get Url
        public static KeyValuePair<String, SQLiteParameter[]> Query_Url_SQL(int primary_key)
        {
            string sql = @"
            SELECT url 
            FROM server 
            where id=@id;";
            SQLiteParameter[] parameters = {
             new SQLiteParameter("@id",primary_key)
            };
            return new KeyValuePair<string, SQLiteParameter[]>(sql, parameters);
        }

    }
}
