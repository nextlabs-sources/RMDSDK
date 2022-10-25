using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace ServiceManager.rmservmgr.app.recentNotification
{
    public interface IRecentNotifications : INotifyEvent
    {
        List<IRecentNotification> List();
        List<IRecentNotifyApp> ListApp();
        void UpdateOrInsertMessage(string messageJson);
        void DeleteItem(int id);
        void DeleteAll();
    }
    public interface IRecentNotification
    {
        int Id { get; }
        string Application { get; set; }
        string Target { get; set; }
        string Message { get; set; }
        int MessageType { get; set; }
        string Operation { get; set; }
        bool Result { get; set; }
        int FileStatus { get; set; }
        DateTime DateTime { get; set; }
    }
    public interface IRecentNotifyApp
    {
        int Id { get; }
        string Application { get;}
        bool IsDisplay { get; set; }
    }
}
