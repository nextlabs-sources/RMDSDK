using Microsoft.Win32;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;

namespace ServiceManager.rmservmgr.common.helper
{
    class GetBrowserDownloadPath
    {
        public static string GetChromeDownloadPath()
        {
            string downloads;
            SHGetKnownFolderPath(KnownFolder.Downloads, 0, IntPtr.Zero, out downloads);
            string defaultPath = downloads;

            string prefile = Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData) + @"\Google\Chrome\User Data\Default\Preferences";
            if (File.Exists(prefile))
            {
                string data = File.ReadAllText(prefile);
                JObject jo = JObject.Parse(data);
                if (jo.ContainsKey("download"))
                {
                    JToken downl = jo.GetValue("download");
                    string default_dir = downl["default_directory"]?.ToString();
                    if (!string.IsNullOrWhiteSpace(default_dir))
                    {
                        defaultPath = default_dir;
                    }
                }
            }

            return defaultPath;
        }

        public static string GetEdgeDownloadPath()
        {
            string downloads;
            SHGetKnownFolderPath(KnownFolder.Downloads, 0, IntPtr.Zero, out downloads);
            string defaultPath = downloads;

            string prefile = Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData) + @"\Microsoft\Edge\User Data\Default\Preferences";
            if (File.Exists(prefile))
            {
                string data = File.ReadAllText(prefile);
                JObject jo = JObject.Parse(data);
                if (jo.ContainsKey("download"))
                {
                    JToken downl = jo.GetValue("download");
                    string default_dir = downl["default_directory"]?.ToString();
                    if (!string.IsNullOrWhiteSpace(default_dir))
                    {
                        defaultPath = default_dir;
                    }
                }
            }

            return defaultPath;
        }

        public static string GetIEDownloadPath()
        {
            string downloads;
            SHGetKnownFolderPath(KnownFolder.Downloads, 0, IntPtr.Zero, out downloads);
            string defaultPath = downloads;

            // HKEY_CURRENT_USER\Software\Microsoft\Internet Explorer\Main
            RegistryKey ie = Registry.CurrentUser.CreateSubKey(@"Software\Microsoft\Internet Explorer\Main");
            defaultPath = (string)ie.GetValue("Default Download Directory", defaultPath);
            ie?.Close();

            return defaultPath;
        }

        public static string GetFireFoxDownloadPath()
        {
            string downloads;
            SHGetKnownFolderPath(KnownFolder.Downloads, 0, IntPtr.Zero, out downloads);
            string defaultPath = downloads;

            // C:\Users\wliang\AppData\Roaming\Mozilla\Firefox\Profiles\0m******.default\prefs.js
            string prefile = Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData) + @"\Mozilla\Firefox\Profiles";
            if (Directory.Exists(prefile))
            {
                string[] folders = Directory.GetDirectories(prefile);
                string defaul_folder = string.Empty;
                for (int i = 0; i < folders.Length; i++)
                {
                    if (folders[i].EndsWith(".default-release"))
                    {
                        defaul_folder = folders[i];
                        break;
                    }
                }

                string filePath = defaul_folder + @"\prefs.js";
                if (File.Exists(filePath))
                {
                    string data = File.ReadAllText(filePath); // user_pref("browser.download.dir", "C:\\Users\\wliang\\Downloads");
                    string label = "\"browser.download.dir\"";
                    int idex = data.IndexOf(label);
                    if (idex > 0)
                    {
                        string subdata = data.Substring(idex + label.Length + 3);
                        int fdex = subdata.IndexOf('"');
                        if (fdex > 0)
                        {
                            string path = subdata.Substring(0, fdex);
                            defaultPath = path.Replace("\\\\", "\\");
                        }
                    }
                }
            }

            return defaultPath;
        }

        /// <summary>
        /// Get Windows Download path
        /// </summary>
        public static class KnownFolder
        {
            public static readonly Guid Downloads = new Guid("374DE290-123F-4565-9164-39C4925E467B");
        }

        [DllImport("shell32.dll", CharSet = CharSet.Unicode)]
        static extern int SHGetKnownFolderPath([MarshalAs(UnmanagedType.LPStruct)] Guid rfid, uint dwFlags, IntPtr hToken, out string pszPath);

    }

}
