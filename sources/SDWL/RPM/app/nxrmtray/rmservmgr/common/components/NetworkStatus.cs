using System;
using System.Collections.Generic;
using System.Linq;
using System.Net.NetworkInformation;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;

namespace ServiceManager.rmservmgr.common.components
{
    /// <summary>
    /// Provides notification of status changes related to Internet-specific network
    /// adapters on this machine.  All other adpaters such as tunneling and loopbacks
    /// are ignored.  Only connected IP adapters are considered.
    /// </summary>
    public static class NetworkStatus
    {
        #region Actively invoke to judge: method 1
        [DllImport("wininet.dll")]
        // Declare outer function
        private extern static bool InternetGetConnectedState(ref int dwFlag, int dwReserved);

        /// <summary>
        /// Actively call this method to judge the network if is avaiable
        /// </summary>
        public static bool IsConnectInternet()
        {
            int dwFlag = 0;
            return InternetGetConnectedState(ref dwFlag, 0);
        }
        #endregion // Actively invoke to judge.



        #region Set network status changed listener to judge: method 2
        // Since we'll likely invoke the IsAvailable property very frequently, that should
        // be very efficient.  So we wire up handlers for both NetworkAvailabilityChanged
        // and NetworkAddressChanged and capture the state in the local isAvailable variable. 
        private static bool isAvailable;
        private static NetworkStatusChangedHandler hander;

        static NetworkStatus()
        {
            isAvailable = IsNetworkAvailable();
        }

        public static bool IsAvailable
        {
            get { return isAvailable; }
        }


        /// <summary>
        /// This event is fired when the overall Internet connectivity changes.  All
        /// non-Internet adpaters are ignored.  If at least one valid Internet connection
        /// is "up" then we consider the Internet "available". 
        /// </summary>
        public static event NetworkStatusChangedHandler AvailabilityChanged
        {
            [MethodImpl(MethodImplOptions.Synchronized)]
            add
            {
                if (hander == null)
                {
                    NetworkChange.NetworkAvailabilityChanged
                        += new NetworkAvailabilityChangedEventHandler(DoNetworkAvailabilityChanged);
                    NetworkChange.NetworkAddressChanged
                        += new NetworkAddressChangedEventHandler(DoNetworkAddressChanged);
                }

                hander = (NetworkStatusChangedHandler)Delegate.Combine(hander, value);
            }

            [MethodImpl(MethodImplOptions.Synchronized)]
            remove
            {
                hander = (NetworkStatusChangedHandler)Delegate.Combine(hander, value);
                if (hander == null)
                {
                    NetworkChange.NetworkAddressChanged
                        -= new NetworkAddressChangedEventHandler(DoNetworkAddressChanged);

                    NetworkChange.NetworkAvailabilityChanged
                        -= new NetworkAvailabilityChangedEventHandler(DoNetworkAvailabilityChanged);
                }
            }
        }


        /// <summary>
        /// Evaluate the online network adapters to determine if at least one of them
        /// is capable of connecting to the Internet.
        /// </summary>
        private static bool IsNetworkAvailable()
        {
            // only recognizes changes related to Internet adapters
            if (NetworkInterface.GetIsNetworkAvailable())
            {
                // however, this will include all adapters
                NetworkInterface[] interfaces = NetworkInterface.GetAllNetworkInterfaces();
                foreach (NetworkInterface face in interfaces)
                {
                    // filter so we see only Internet adapters
                    if (face.OperationalStatus == OperationalStatus.Up && !face.Description.Contains("VMware Virtual Ethernet Adapter"))
                    {
                        if ((face.NetworkInterfaceType != NetworkInterfaceType.Tunnel) &&
                            (face.NetworkInterfaceType != NetworkInterfaceType.Loopback))
                        {
                            IPv4InterfaceStatistics statistics = face.GetIPv4Statistics();

                            // all testing seems to prove that once an interface comes online
                            // it has already accrued statistics for both received and sent...

                            if ((statistics.BytesReceived > 0) &&
                                (statistics.BytesSent > 0))
                            {
                                return true;
                            }
                        }
                    }
                }
            }

            return false;
        }

        private static void DoNetworkAddressChanged(object sender, EventArgs e)
        {
            SigalAvailabilityChange(sender);
        }

        private static void DoNetworkAvailabilityChanged(object sender, NetworkAvailabilityEventArgs e)
        {
            SigalAvailabilityChange(sender);
        }

        private static void SigalAvailabilityChange(object sender)
        {
            bool change = IsNetworkAvailable();

            if (change != isAvailable)
            {
                isAvailable = change;
                hander?.Invoke(sender, new NetworkStatusChangedArgs(isAvailable));
            }
        }

        #endregion // Set network status changed listener to judge


        /// <summary>
        /// Define the network status changed Args
        /// </summary>
        public class NetworkStatusChangedArgs : EventArgs
        {
            private bool isAvailable;

            public NetworkStatusChangedArgs(bool isAvaiable)
            {
                this.isAvailable = isAvaiable;
            }

            public bool IsAvailable
            {
                get { return isAvailable; }
            }
        }


        /// <summary>
        /// Define the method signature for the network status changes.
        /// </summary>
        public delegate void NetworkStatusChangedHandler(object sender, NetworkStatusChangedArgs e);
    }
}
