using Microsoft.Win32;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Diagnostics;

namespace ServiceManager.rmservmgr.common.helper
{
    class ClearRegistryFileRecord
    {
        public static void ClearAppHistoryDocs(string appName)
        {
            if (string.IsNullOrEmpty(appName))
            {
                return;
            }

            string name = appName.ToLower();
            try
            {
                try
                {
                    RegistryKey appOSRMXKey = Registry.LocalMachine.OpenSubKey(@"SOFTWARE\NextLabs\SkyDRM\OSRMX\whitelists\" + name, false);
                    string appCleanupCMD = (string)appOSRMXKey?.GetValue("cleanup", "");
                    if (appCleanupCMD != "")
                    {
                        ProcessStartInfo processStartInfo = new ProcessStartInfo(appCleanupCMD);
                        processStartInfo.UseShellExecute = false;
                        processStartInfo.WindowStyle = ProcessWindowStyle.Hidden;

                        Process process = new Process();
                        process.StartInfo = processStartInfo;
                        process.Start();
                    }
                } catch (Exception e) { }

                if (name == "veviewer.exe")
                {
                    ClearSAP();
                }
                else if (name == "acrord32.exe" || name == "acrobat.exe")
                {
                    ClearAdobe();
                }
                else if (name == "visview_ng.exe")
                {
                    ClearJT2Go();
                }

            }
            catch (Exception e)
            {
                ServiceManagerApp.Singleton.Log.Warn(e.ToString());
            }
        }


        private static void ClearAdobe()
        {
            RegistryKey aReaderKey = Registry.CurrentUser.OpenSubKey(@"SOFTWARE\Adobe\Acrobat Reader", true);
            string[] subKeyNames = aReaderKey?.GetSubKeyNames();
            foreach (string keyName in subKeyNames) // 11.0
            {
                string aGenalPath = keyName + @"\AVGeneral";
                RegistryKey avGeneralKey = aReaderKey.OpenSubKey(aGenalPath, true);
                avGeneralKey?.DeleteSubKeyTree("cRecentFiles", false);
                avGeneralKey?.Close();
            }
            aReaderKey?.Close();
        }

        private static void ClearSAP()
        {
            RegistryKey sapHistory = Registry.CurrentUser.OpenSubKey(@"SOFTWARE\SAP\SAP 3D Visual Enterprise Viewer", true);
            sapHistory?.DeleteSubKey("History", false);
            sapHistory?.Close();
        }

        //CurrentUser/Software/Siemens/JT2Go/13.0/JT2Go/C/Recent File List
        private static void ClearJT2Go()
        {
            RegistryKey smsKey = Registry.CurrentUser.OpenSubKey(@"SOFTWARE\Siemens", true);
            string[] jtKeyNames = smsKey?.GetSubKeyNames();
            foreach (string jtName in jtKeyNames) // JT2Go, JT2Go_Re...
            {
                RegistryKey jtKey = smsKey.OpenSubKey(jtName, true);
                string[] verKeyNames2 = jtKey?.GetSubKeyNames();
                foreach (string versionName in verKeyNames2) // 13.0
                {
                    string cPath= versionName + @"\JT2Go\C";
                    RegistryKey cKey = jtKey.OpenSubKey(cPath, true);
                    cKey?.DeleteSubKey("Recent File List", false);
                    cKey?.Close();
                }
                jtKey?.Close();
            }
            smsKey?.Close();
        }

    }
}
