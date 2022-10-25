using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace ServiceManager.rmservmgr.common.filemonitor
{
    // This is the monitored object.
    public class MonitoredItem: IMonitoredItem
    {
        public uint Pid { get;}

        public string AppName { get; }

        public string FilePath { get; }

        public bool IsAppAllowEdit { get; set; }

        public bool IsFileAllowEdit { get; set; }

        // By compare, check file if is edited.
        public DateTime LastModified { get; set; }

        // Stop monitor when receive the event that app process exit.
        public bool IsStopMonitor { get; set; }

        public MonitoredItem(uint pid, string appName, string filePath, bool isAppAllowEdit, bool isFileAllowEdit)
        {
            this.Pid = pid;
            this.AppName = appName;
            this.FilePath = filePath;
            this.IsAppAllowEdit = isAppAllowEdit;
            this.IsFileAllowEdit = isFileAllowEdit;

            this.IsStopMonitor = false;
        }
    }
}
