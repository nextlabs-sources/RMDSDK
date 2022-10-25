using System;
using System.Collections.Generic;
using System.Data;
using System.Data.Common;
using System.Data.SQLite;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace ServiceManager.rmservmgr.db
{
    class SqliteOpenHelper
    {
        public static void ExecuteReader(
            string connectionString,
            KeyValuePair<String, SQLiteParameter[]> pair,
            Action<SQLiteDataReader> action
            )
        {
            ExecuteReader(connectionString, pair.Key, pair.Value, action);
        }

        public static void ExecuteReader(string connectionString,
            string sql,
            SQLiteParameter[] parameters,
            Action<SQLiteDataReader> action)
        {
            try
            {
                using (SQLiteConnection connection = new SQLiteConnection(connectionString))
                {
                    connection.Open();
                    using (var transaction = connection.BeginTransaction(IsolationLevel.Serializable))
                    {
                        using (SQLiteCommand command = new SQLiteCommand(sql, connection))
                        {
                            if (parameters != null)
                            {
                                command.Parameters.AddRange(parameters);
                            }
                            using (var reader = command.ExecuteReader())
                            {
                                action?.Invoke(reader);
                            }
                        }
                        transaction.Commit();
                    }
                }
            }
            catch (Exception e)
            {
                Console.WriteLine("SQLiteException in ExecuteReader");
                ServiceManagerApp.Singleton.Log.Error("SQLiteException in ExecuteReader", e);
            }
        }


        public static int ExecuteNonQuery(string connectionString, KeyValuePair<String, SQLiteParameter[]> pair)
        {
            return ExecuteNonQuery(connectionString, pair.Key, pair.Value);
        }

        public static int ExecuteNonQuery(string connectionString, string sql, SQLiteParameter[] parameters)
        {
            int affectedRows = 0;

            try
            {
                using (SQLiteConnection connection = new SQLiteConnection(connectionString))
                {
                    connection.Open();
                    using (var transaction = connection.BeginTransaction())
                    {
                        using (SQLiteCommand command = new SQLiteCommand(connection))
                        {
                            command.CommandText = sql;
                            if (parameters != null)
                            {
                                command.Parameters.AddRange(parameters);
                            }
                            affectedRows = command.ExecuteNonQuery();
                        }
                        transaction.Commit();
                    }
                }
            }
            catch (Exception e)
            {
                Console.WriteLine("SQLiteException in ExecuteNonQuery: " + e.Message);
                ServiceManagerApp.Singleton.Log.Error("SQLiteException in ExecuteNonQuery: " + e);
            }

            return affectedRows;
        }

        public static int ExecuteNonQueryBatch(string connectionString, KeyValuePair<string, SQLiteParameter[]>[] sqls)
        {
            int affectedRows = 0;

            try
            {
                using (var connection = new SQLiteConnection(connectionString))
                {
                    connection.Open();
                    using (DbTransaction transaction = connection.BeginTransaction())
                    {
                        using (SQLiteCommand command = new SQLiteCommand(connection))
                        {
                            foreach (var i in sqls)
                            {
                                command.CommandText = i.Key;
                                if (i.Value != null)
                                {
                                    command.Parameters.AddRange(i.Value);
                                }
                                affectedRows += command.ExecuteNonQuery();
                            }
                        }
                        transaction.Commit();
                    }
                }
            }
            catch (Exception e)
            {
                Console.WriteLine("SQLiteException in ExecuteNonQueryBatch: " + e.Message);
                ServiceManagerApp.Singleton.Log.Error("SQLiteException in ExecuteNonQueryBatch: " + e);
            }

            return affectedRows;
        }

        public static int GetVersion(string connectionString)
        {
            int version = -1;
            using (SQLiteConnection connection = new SQLiteConnection(connectionString))
            {
                connection.Open();
                using (SQLiteCommand command = new SQLiteCommand(connection))
                {
                    command.CommandText = "PRAGMA user_version";
                    using (var reader = command.ExecuteReader())
                    {
                        if (reader.Read())
                        {
                            version = reader.GetInt32(0);
                            Console.WriteLine("PRAGMA user_version is {0}", version);
                        }
                    }
                }
            }
            return version;
        }

        public static void SetVersion(string connectionString, int version)
        {
            using (SQLiteConnection connection = new SQLiteConnection(connectionString))
            {
                connection.Open();
                using (SQLiteCommand command = new SQLiteCommand(connection))
                {
                    command.CommandText = "PRAGMA user_version = " + version;
                    command.ExecuteNonQuery();
                }
            }
        }

        public static bool IsDbFileExists(string dbPath)
        {
            return File.Exists(dbPath);
        }

        public static bool CheckCloumnExist(SQLiteCommand command, string tableName, string columnName)
        {
            command.CommandText = string.Format("SELECT * FROM {0} LIMIT 0", tableName);
            bool hascol = false;
            using (SQLiteDataReader reader = command.ExecuteReader())
            {
                var nameVaultCollection = reader.GetValues();
                var keyCollection = nameVaultCollection.Keys;
                foreach (var key in keyCollection)
                {
                    if (key.Equals(columnName))
                    {
                        hascol = true;
                    }
                }
            }
            return hascol;
        }
    }
}
