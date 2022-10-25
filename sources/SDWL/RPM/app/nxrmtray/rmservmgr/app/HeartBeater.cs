using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading;
using System.Threading.Tasks;

namespace ServiceManager.rmservmgr.app
{
    class HeartBeater
    {
        ServiceManagerApp host;
        int count = 1;
        bool stop = false;
        Thread thisThread = null;

        public HeartBeater(ServiceManagerApp host)
        {
            this.host = host;
        }

        public bool Stop
        {
            get => stop;
            set
            {
                stop = value;
                if (thisThread != null) thisThread.Interrupt();
            }
        }

        public void WorkingBackground()
        {
            new Thread(HeartBeat) { Name = "UserHeartBeat", IsBackground = true }.Start();
        }

        private void HeartBeat()
        {
            thisThread = Thread.CurrentThread;
            host.Log.Info("====HeartBeat started!====");
            try
            {
                // each subtask can not throw out excpetion
                while (!stop)
                {
                    host.User.OnHeartBeat();

                    host.Dispatcher.Invoke(() =>
                    {
                        host.InvokeHeartBeatEvent();
                    });

                    // fix Bug 58750 - Save as central policy file to Desktop from project, cannot modify rights in Desktop
                    // In SDWL_User_GetListProjtects api will GetListProjectAdmins and update projectAdminsList.
                    host.Session.User.UpdateProjectInfo();

                    if (stop) break;

                    host.Log.Info($"====HearBeat {count++}t====, will sleep {host.User.HeartBeatIntervalSec}s");
                    Thread.Sleep(host.User.HeartBeatIntervalSec * 1000);
                }
            }
            catch (Exception e)
            {
                host.Log.Error(e);
            }
            host.Log.Info("====HeartBeat finished!====");
        }

    }
}
