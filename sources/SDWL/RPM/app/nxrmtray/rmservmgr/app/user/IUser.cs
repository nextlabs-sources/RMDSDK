using ServiceManager.rmservmgr.sdk;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace ServiceManager.rmservmgr.app.user
{
    /// <summary>
    ///  As current login User, to provider as much as possible info to Upper-level
    /// </summary>
    public interface IUser : IHeartBeat
    {
        string WorkingFolder { get; }

        // this is a tmp work around method, since many features need to get user's sdk folder
        string SDkWorkingFolder { get; }

        int RmsUserId { get; }

        string Name { get; }

        string Email { get; }

        UserType UserType { get; }

        bool LeaveCopy { get; set; }

        bool ShowNotifyWindow { get; set; }

        WaterMarkInfo Watermark { get; set; }

        Expiration Expiration { get; set; }

        /// <summary>
        /// Get WaterMark and Expiration from rms
        /// </summary>
        void GetDocumentPreference();

        /// <summary>
        /// Update User WaterMark and Expiration to rms
        /// </summary>
        void UpdateDocumentPreference();

        int HeartBeatIntervalSec { get; }
    }

    public struct Quota
    {
        public long usage;
        public long totalquota;
        public long vaultusage;
        public long vaultquota;
    }

}
