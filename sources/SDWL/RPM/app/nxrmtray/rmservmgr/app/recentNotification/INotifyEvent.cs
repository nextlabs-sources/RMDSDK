using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace ServiceManager.rmservmgr.app.recentNotification
{
    public class NotifyEventArgs : EventArgs
    {
        public NotifyEventArgs(bool isUpdate, IRecentNotification notification)
        {
            this.IsUpdate = isUpdate;
            this.Notification = notification;
        }
        public bool IsUpdate { get; set; }
        public IRecentNotification Notification { get; set; }
    }

    public  interface INotifyEvent
    {
        event EventHandler<NotifyEventArgs> NotifyEvent;
    }
}
