using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace ServiceManager.rmservmgr.app.recentNotification
{
    public sealed class MyRecentNotifications : IRecentNotifications
    {
        private readonly ServiceManagerApp app = ServiceManagerApp.Singleton;
        private List<IRecentNotification> oldNotification = new List<IRecentNotification>();

        public event EventHandler<NotifyEventArgs> NotifyEvent;

        public List<IRecentNotification> List()
        {
            oldNotification.Clear();

            var list = app.DBProvider.QueryRecentNotification();
            if (list==null || list.Count == 0)
            {
                return oldNotification;
            }
            
            foreach (var item in list)
            {
                oldNotification.Add(new MyRecentNotification(item));
            }
            return oldNotification;
        }

        public List<IRecentNotifyApp> ListApp()
        {
            List<IRecentNotifyApp> notifyApps = new List<IRecentNotifyApp>();
            var list = app.DBProvider.QueryRecentNotifyApp();
            if (list == null || list.Count == 0)
            {
                return notifyApps;
            }

            foreach (var item in list)
            {
                notifyApps.Add(new MyRecentNotifyApp(item));
            }
            return notifyApps;
        }

        public void UpdateOrInsertMessage(string paraJson)
        {
            try
            {
                MessagePara mPara = ParseMessageData(paraJson, out bool parseResult);
                if (!parseResult)
                {
                    //app.ShowBalloonTip(paraJson);
                    return;
                }
                // fix bug 57570, 57598, 57600
                if (HandleJsonIsEmpty(mPara))
                {
                    return;
                }

                // insert to db
                var item = app.DBProvider.InsertRecentNotification(JsonConvert.SerializeObject(mPara));
                var mItem = new MyRecentNotification(item);
                oldNotification.Add(mItem);
                // notify
                NotifyEvent?.Invoke(this, new NotifyEventArgs(false, mItem));

                // insert application to RecentNotifyApp db
                app.DBProvider.UpsertRecentNotifyApp(mPara.Application);
            }
            catch (Exception e)
            {
                app.Log.Error(e);
            }
        }

        public void DeleteItem(int id)
        {
            // delete in RecentNotification item
            app.DBProvider.DeleteRecentNotification(id);
            oldNotification.RemoveAll(x => x.Id == id);

            // delete in RecentNotifyApp item
            //var listApp = ListApp();
            //foreach (var item in listApp)
            //{
            //    var result = oldNotification.Find(s => s.Application.Equals(item.Application));
            //    if (result == null)
            //    {
            //        app.DBProvider.DeleteRecentNotifyApp(item.Id);
            //    }
            //}
        }

        public void DeleteAll()
        {
            app.DBProvider.DeleteAllRecentNotification();
            oldNotification.Clear();
        }

        private MessagePara ParseMessageData(string value, out bool parseResult)
        {
            parseResult = true;
            MessagePara parameters = new MessagePara();
            try
            {
                JObject jo = (JObject)JsonConvert.DeserializeObject(value);

                if (jo.ContainsKey("application"))
                {
                    parameters.Application = jo["application"].ToString();
                }
                if (jo.ContainsKey("target"))
                {
                    parameters.Target = jo["target"].ToString();
                }
                if (jo.ContainsKey("message"))
                {
                    parameters.Message = jo["message"].ToString();
                }
                if (jo.ContainsKey("msgtype"))
                {
                    if (int.TryParse(jo["msgtype"].ToString(), out int result))
                    {
                        parameters.Msgtype = result;
                    }
                }
                if (jo.ContainsKey("operation"))
                {
                    parameters.Operation = jo["operation"].ToString();
                }
                if (jo.ContainsKey("result"))
                {
                    if (int.TryParse(jo["result"].ToString(), out int result))
                    {
                        if (result == 1)
                        {
                            parameters.Result = true;
                        }
                    }
                }
                if (jo.ContainsKey("fileStatus"))
                {
                    if (int.TryParse(jo["fileStatus"].ToString(), out int result))
                    {
                        parameters.FileStatus = result;
                    }
                }

            }
            catch (Exception e)
            {
                app.Log.Error(e);
                parseResult = false;
            }
            return parameters;
        }

        private bool HandleJsonIsEmpty(MessagePara para)
        {
            bool result = false;

            if (string.IsNullOrEmpty(para.Application)
                || string.IsNullOrEmpty(para.Target)
                || string.IsNullOrEmpty(para.Message))
            {
                result = true;

                if( !string.IsNullOrEmpty(para.Application) && 
                    string.IsNullOrEmpty(para.Target) && 
                    !string.IsNullOrEmpty(para.Message))
                {
                    app.ServiceManagerWin?.ShowNotifyWindow(para);
                }
            }

            return result;
        }
    }

    public sealed class MyRecentNotification : IRecentNotification
    {
        private db.table.RecentNotification raw;
        private MessagePara messagePara;

        public MyRecentNotification(db.table.RecentNotification raw)
        {
            this.raw = raw;
            this.messagePara = JsonConvert.DeserializeObject<MessagePara>(raw.MessageJson);
        }

        public int Id => raw.Id;
        public string Application { get => messagePara.Application; set => messagePara.Application = value; }
        public string Target { get => messagePara.Target; set => messagePara.Target = value; }
        public string Message { get => messagePara.Message; set => messagePara.Message = value; }
        public int MessageType { get => messagePara.Msgtype; set => messagePara.Msgtype = value; }
        public string Operation { get => messagePara.Operation; set => messagePara.Operation = value; }
        public bool Result { get => messagePara.Result; set => messagePara.Result = value; }
        public int FileStatus { get => messagePara.FileStatus; set => messagePara.FileStatus = value; }
        public DateTime DateTime { get => raw.Last_modified_time; set => raw.Last_modified_time = value; }
    }

    public sealed class MyRecentNotifyApp : IRecentNotifyApp
    {
        private readonly ServiceManagerApp app = ServiceManagerApp.Singleton;
        private db.table.RecentNotifyApp raw;

        public MyRecentNotifyApp(db.table.RecentNotifyApp raw)
        {
            this.raw = raw;
        }

        public int Id => raw.Id;
        public string Application { get => raw.Application; }
        public bool IsDisplay { get => raw.IsDisplay==1 ? true : false; set => UpdateIsDisplay(value); }

        private void UpdateIsDisplay(bool isDisplay)
        {
            app.DBProvider.UpdateRecentNotifyApp(Id, isDisplay);
            raw.IsDisplay = isDisplay ? 1 : 0;
        }

    }

    public struct MessagePara
    {
        public string Application { get; set; }
        public string Target { get; set; }
        public string Message { get; set; }
        public int Msgtype { get; set; }
        public string Operation { get; set; }
        public bool Result { get; set; }
        public int FileStatus { get; set; }
    }

    public enum FileStatus
    {
        UnKnow=0,
        Online,
        Offline,
        WaitingUpload,
    }

}
