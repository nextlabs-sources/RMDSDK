using Microsoft.Win32;
using ServiceManager.rmservmgr.sdk;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace ServiceManager.rmservmgr.common.helper
{
    public class LoadAddInHelper
    {
        private static List<string> KeyNames = new List<string>();
        private static List<string> CurrentUserSubKeys = new List<string>();
        private static List<string> LocalMachineSubKeys = new List<string>();
        private static UIntPtr HKEY_CURRENT_USER = (UIntPtr)((long)0x80000001);
        private static UIntPtr HKEY_LOCAL_MACHINE = (UIntPtr)((long)0x80000002);

        static LoadAddInHelper()
        {
            KeyNames.Add(@"Excel\Addins\nxrmExcelAddIn");
            KeyNames.Add(@"PowerPoint\Addins\nxrmPowerPointAddIn");
            KeyNames.Add(@"Word\Addins\nxrmWordAddIn");

            //CurrentUser
            CurrentUserSubKeys.Add(@"Software\Microsoft\Office\");

            //LocalMachine
            LocalMachineSubKeys.Add(@"SOFTWARE\MICROSOFT\Office\"); //x64
            LocalMachineSubKeys.Add(@"SOFTWARE\Wow6432Node\Microsoft\Office\"); //x86

            //LocalMachineclickToRun
            LocalMachineSubKeys.Add(@"SOFTWARE\MICROSOFT\Office\ClickToRun\REGISTRY\MACHINE\SOFTWARE\Microsoft\Office\"); //x64
            LocalMachineSubKeys.Add(@"SOFTWARE\MICROSOFT\Office\ClickToRun\REGISTRY\MACHINE\SOFTWARE\Wow6432Node\Microsoft\Office\"); //x86
        }

        /// <summary>
        /// Detect office app if is installed in local machine, now only consider Office 2013 & Office 2016.
        /// </summary>
        /// <param name="version">returned the office version</param>
        public static bool IsOfficeInstalled(out EnumOfficeVer version)
        {
            bool ret = false;

            version = EnumOfficeVer.Unknown;

            try
            {
                // For 32-bit office
                RegistryKey baseKey32 = RegistryKey.OpenBaseKey(RegistryHive.LocalMachine, RegistryView.Registry32);
                RegistryKey subKey32_15 = baseKey32.OpenSubKey(@"SOFTWARE\Microsoft\Office\15.0\Common\InstallRoot", false); // Office 2013
                RegistryKey subKey32_16 = baseKey32.OpenSubKey(@"SOFTWARE\Microsoft\Office\16.0\Word\InstallRoot", false); // Office 2016

                // For 64-bit office
                RegistryKey baseKey64 = RegistryKey.OpenBaseKey(RegistryHive.LocalMachine, RegistryView.Registry64);
                RegistryKey subKey64_15 = baseKey64.OpenSubKey(@"SOFTWARE\Microsoft\Office\15.0\Common\InstallRoot", false); // Office 2013
                RegistryKey subKey64_16 = baseKey64.OpenSubKey(@"SOFTWARE\Microsoft\Office\16.0\Word\InstallRoot", false); // Office 2016

                if ((subKey32_16 != null && subKey32_16.GetValue("Path") != null)
                    || (subKey64_16 != null && subKey64_16.GetValue("Path") != null))
                {
                    version = EnumOfficeVer.Office_2016;
                    ret = true;
                }
                else if ((subKey32_15 != null && subKey32_15.GetValue("Path") != null)
                    || (subKey64_15 != null && subKey64_15.GetValue("Path") != null))
                {
                    version = EnumOfficeVer.Office_2013;
                    ret = true;
                }

            }
            catch (Exception e)
            {
                Console.WriteLine(" Exception in IsOfficeInstalled.");
            }

            return ret;
        }

        public static void ChangeRegeditOfOfficeAddin(Session session)
        {
            string name = "LoadBehavior";
            uint value = 3;

            foreach (string keyName in KeyNames)
            {
                foreach (string keyPath in CurrentUserSubKeys)
                {
                    bool rt = session.SDWL_Register_SetValue(HKEY_CURRENT_USER, keyPath + keyName, name, value);
                }

                foreach (string keyPath in LocalMachineSubKeys)
                {
                    bool rt = session.SDWL_Register_SetValue(HKEY_LOCAL_MACHINE, keyPath + keyName, name, value);
                }
            }
        }

        public enum EnumOfficeVer
        {
            Unknown = 0,
            Office_2013 = 1,
            Office_2016 = 2
        }

    }
}
