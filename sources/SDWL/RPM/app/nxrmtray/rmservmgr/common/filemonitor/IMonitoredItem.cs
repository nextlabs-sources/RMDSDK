using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace ServiceManager.rmservmgr.common.filemonitor
{
    public interface IMonitoredItem
    {
        uint Pid { get; }

        string AppName { get; }

        string FilePath { get; }

        bool IsAppAllowEdit { get; set; }

        bool IsFileAllowEdit { get; set; }

        // By compare, check file if is edited.
        DateTime LastModified { get; set; }

        // Stop monitor when receive the event that app process exit.
        bool IsStopMonitor { get; set; }
    }
}
