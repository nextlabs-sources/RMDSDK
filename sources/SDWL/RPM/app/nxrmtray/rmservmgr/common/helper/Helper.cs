using CustomControls.common.helper;
using CustomControls.componentPages.Preference;
using CustomControls.components.ValiditySpecify.model;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using ServiceManager.rmservmgr.app.recentNotification;
using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Media.Imaging;

namespace ServiceManager.rmservmgr.common.helper
{
    // Used to wrapper common win32 api
    public class Win32Common
    {
        /// <summary>
        /// Specifies a unique application-defined Application User Model ID (AppUserModelID),
        /// that identifies the current process to the taskbar. 
        /// This identifier allows an application to group its associated processes and windows under a single taskbar button.
        /// </summary>
        /// <param name="AppID"></param>
        [DllImport("shell32.dll", SetLastError = true)]
        public static extern void SetCurrentProcessExplicitAppUserModelID([MarshalAs(UnmanagedType.LPWStr)] string AppID);

        // rmc-sdk required set cookies when login
        [DllImport("wininet.dll", CharSet = CharSet.Auto, SetLastError = true)]
        public static extern bool InternetSetCookie(string lpszUrlName, string lpszCookieName, string lpszCookieData);

        [DllImport("User32.dll", CharSet = CharSet.Unicode)]
        public static extern uint RegisterWindowMessage(string Message);

    }

    // Used to some common utils
    public class CommonUtils
    {
        public static string ApplicationFindResource(string key)
        {
            if (string.IsNullOrEmpty(key))
            {
                return string.Empty;
            }
            try
            {
                string ResourceString = ServiceManagerApp.Singleton.FindResource(key).ToString();
                return ResourceString;
            }
            catch (Exception e)
            {
                ServiceManagerApp.Singleton.Log.Error(e.Message);
                return string.Empty;
            }

        }

        public static string ConvertNameToAvatarText(string name, string rule)
        {
            name = name.Trim();
            if (string.IsNullOrEmpty(name))
            {
                return "";
            }
            string letter = "";
            if (string.Equals(" ", rule))
            {
                rule = "\\s+";
            }
            String[] split = System.Text.RegularExpressions.Regex.Split(name, rule);
            if (split.Length > 1)
            {

                letter = string.Concat(letter, split[0].Substring(0, 1).ToUpper());
                //fix bug 51464
                //letter = string.Concat(letter," ");
                letter = string.Concat(letter, split[split.Length - 1].Substring(0, 1).ToUpper());
            }
            else
            {
                letter = name.Substring(0, 1).ToUpper();
            }
            return letter;
        }

        public static string SelectionBackgroundColor(string name)
        {
            if (string.IsNullOrEmpty(name))
            {
                return "#9D9FA2";
            }
            switch (name.Substring(0, 1).ToUpper())
            {
                case "A":
                    return "#DD212B";
                case "B":
                    return "#FDCB8A";
                case "C":
                    return "#98C44A";
                case "D":
                    return "#1A5279";
                case "E":
                    return "#EF6645";
                case "F":
                    return "#72CAC1";
                case "G":
                    return "#B7DCAF";
                case "H":
                    return "#705A9E";
                case "I":
                    return "#FCDA04";
                case "J":
                    return "#ED1D7C";
                case "K":
                    return "#F7AAA5";
                case "L":
                    return "#4AB9E6";
                case "M":
                    return "#603A18";
                case "N":
                    return "#88B8BC";
                case "O":
                    return "#ECA81E";
                case "P":
                    return "#DAACD0";
                case "Q":
                    return "#6D6E73";
                case "R":
                    return "#9D9FA2";
                case "S":
                    return "#B5E3EE";
                case "T":
                    return "#90633D";
                case "U":
                    return "#BDAE9E";
                case "V":
                    return "#C8B58E";
                case "W":
                    return "#F8BDD2";
                case "X":
                    return "#FED968";
                case "Y":
                    return "#F69679";
                case "Z":
                    return "#EE6769";
                case "0":
                    return "#D3E050";
                case "1":
                    return "#D8EBD5";
                case "2":
                    return "#F27EA9";
                case "3":
                    return "#1782C0";
                case "4":
                    return "#CDECF9";
                case "5":
                    return "#FDE9E6";
                case "6":
                    return "#FCED95";
                case "7":
                    return "#F99D21";
                case "8":
                    return "#F9A85D";
                case "9":
                    return "#BCE2D7";
                default:
                    return "#333333";
            }
        }

        public static string SelectionTextColor(string name)
        {
            if (string.IsNullOrEmpty(name))
            {
                return "#ffffff";
            }
            switch (name.Substring(0, 1).ToUpper())
            {
                case "A":
                    return "#ffffff";
                case "B":
                    return "#8F9394";
                case "C":
                    return "#ffffff";
                case "D":
                    return "#ffffff";
                case "E":
                    return "#ffffff";
                case "F":
                    return "#ffffff";
                case "G":
                    return "#8F9394";
                case "H":
                    return "#ffffff";
                case "I":
                    return "#8F9394";
                case "J":
                    return "#ffffff";
                case "K":
                    return "#ffffff";
                case "L":
                    return "#ffffff";
                case "M":
                    return "#ffffff";
                case "N":
                    return "#ffffff";
                case "O":
                    return "#ffffff";
                case "P":
                    return "#ffffff";
                case "Q":
                    return "#ffffff";
                case "R":
                    return "#ffffff";
                case "S":
                    return "#ffffff";
                case "T":
                    return "#ffffff";
                case "U":
                    return "ffffff";
                case "V":
                    return "#ffffff";
                case "W":
                    return "#ffffff";
                case "X":
                    return "#8F9394";
                case "Y":
                    return "#ffffff";
                case "Z":
                    return "#ffffff";
                case "0":
                    return "#8F9394";
                case "1":
                    return "#8F9394";
                case "2":
                    return "#ffffff";
                case "3":
                    return "#ffffff";
                case "4":
                    return "#8F9394";
                case "5":
                    return "#8F9394";
                case "6":
                    return "#8F9394";
                case "7":
                    return "#ffffff";
                case "8":
                    return "#ffffff";
                case "9":
                    return "#8F9394";
                default:
                    return "#ffffff";
            }
        }

        public static bool FileStatusEnumTryParse(int fileStatus, out FileStatus fileStatusEnum)
        {
            bool result = false;
            fileStatusEnum = FileStatus.UnKnow;

            if (Enum.IsDefined(typeof(FileStatus), fileStatus))
            {
                fileStatusEnum = (FileStatus)fileStatus;
                result = true;
            }
            return result;
        }

        /// <summary>
        /// Used to judge using the new file type icon .png format
        /// </summary>
        /// <param name="fileType"></param>
        /// <returns></returns>
        public static bool IsSupportFileTypeEx(string fileType)
        {
            bool result = false;

            if (string.Equals(fileType, "3dxml", StringComparison.CurrentCultureIgnoreCase)
                    || string.Equals(fileType, "bmp", StringComparison.CurrentCultureIgnoreCase)
                    || string.Equals(fileType, "c", StringComparison.CurrentCultureIgnoreCase)
                    || string.Equals(fileType, "catpart", StringComparison.CurrentCultureIgnoreCase)
                    || string.Equals(fileType, "catshape", StringComparison.CurrentCultureIgnoreCase)
                    || string.Equals(fileType, "cgr", StringComparison.CurrentCultureIgnoreCase)
                    || string.Equals(fileType, "cpp", StringComparison.CurrentCultureIgnoreCase)
                    || string.Equals(fileType, "csv", StringComparison.CurrentCultureIgnoreCase)
                    || string.Equals(fileType, "doc", StringComparison.CurrentCultureIgnoreCase)
                    || string.Equals(fileType, "docm", StringComparison.CurrentCultureIgnoreCase)
                    || string.Equals(fileType, "docx", StringComparison.CurrentCultureIgnoreCase)
                    || string.Equals(fileType, "dotx", StringComparison.CurrentCultureIgnoreCase)
                    || string.Equals(fileType, "dwg", StringComparison.CurrentCultureIgnoreCase)
                    || string.Equals(fileType, "dxf", StringComparison.CurrentCultureIgnoreCase)
                    || string.Equals(fileType, "err", StringComparison.CurrentCultureIgnoreCase)
                    || string.Equals(fileType, "exe", StringComparison.CurrentCultureIgnoreCase)
                    || string.Equals(fileType, "ext", StringComparison.CurrentCultureIgnoreCase)
                    || string.Equals(fileType, "file", StringComparison.CurrentCultureIgnoreCase)
                    || string.Equals(fileType, "gdoc", StringComparison.CurrentCultureIgnoreCase)
                    || string.Equals(fileType, "gdra", StringComparison.CurrentCultureIgnoreCase)
                    || string.Equals(fileType, "gif", StringComparison.CurrentCultureIgnoreCase)
                    || string.Equals(fileType, "gshe", StringComparison.CurrentCultureIgnoreCase)
                    || string.Equals(fileType, "gsli", StringComparison.CurrentCultureIgnoreCase)
                    || string.Equals(fileType, "h", StringComparison.CurrentCultureIgnoreCase)
                    || string.Equals(fileType, "hsf", StringComparison.CurrentCultureIgnoreCase)
                    || string.Equals(fileType, "htm", StringComparison.CurrentCultureIgnoreCase)
                    || string.Equals(fileType, "html", StringComparison.CurrentCultureIgnoreCase)
                    || string.Equals(fileType, "hwf", StringComparison.CurrentCultureIgnoreCase)
                    || string.Equals(fileType, "iges", StringComparison.CurrentCultureIgnoreCase)
                    || string.Equals(fileType, "igs", StringComparison.CurrentCultureIgnoreCase)
                    || string.Equals(fileType, "ipt", StringComparison.CurrentCultureIgnoreCase)
                    || string.Equals(fileType, "java", StringComparison.CurrentCultureIgnoreCase)
                    || string.Equals(fileType, "jpg", StringComparison.CurrentCultureIgnoreCase)
                    || string.Equals(fileType, "js", StringComparison.CurrentCultureIgnoreCase)
                    || string.Equals(fileType, "json", StringComparison.CurrentCultureIgnoreCase)
                    || string.Equals(fileType, "jt", StringComparison.CurrentCultureIgnoreCase)
                    || string.Equals(fileType, "key", StringComparison.CurrentCultureIgnoreCase)
                    || string.Equals(fileType, "log", StringComparison.CurrentCultureIgnoreCase)
                    || string.Equals(fileType, "m", StringComparison.CurrentCultureIgnoreCase)
                    || string.Equals(fileType, "md", StringComparison.CurrentCultureIgnoreCase)
                    || string.Equals(fileType, "model", StringComparison.CurrentCultureIgnoreCase)
                    || string.Equals(fileType, "mov", StringComparison.CurrentCultureIgnoreCase)
                    || string.Equals(fileType, "mp3", StringComparison.CurrentCultureIgnoreCase)
                    || string.Equals(fileType, "mp4", StringComparison.CurrentCultureIgnoreCase)
                    || string.Equals(fileType, "numb", StringComparison.CurrentCultureIgnoreCase)
                    || string.Equals(fileType, "page", StringComparison.CurrentCultureIgnoreCase)
                    || string.Equals(fileType, "par", StringComparison.CurrentCultureIgnoreCase)
                    || string.Equals(fileType, "pdf", StringComparison.CurrentCultureIgnoreCase)
                    || string.Equals(fileType, "png", StringComparison.CurrentCultureIgnoreCase)
                    || string.Equals(fileType, "potm", StringComparison.CurrentCultureIgnoreCase)
                    || string.Equals(fileType, "potx", StringComparison.CurrentCultureIgnoreCase)
                    || string.Equals(fileType, "ppt", StringComparison.CurrentCultureIgnoreCase)
                    || string.Equals(fileType, "pptx", StringComparison.CurrentCultureIgnoreCase)
                    || string.Equals(fileType, "properties", StringComparison.CurrentCultureIgnoreCase)
                    || string.Equals(fileType, "prt", StringComparison.CurrentCultureIgnoreCase)
                    || string.Equals(fileType, "psm", StringComparison.CurrentCultureIgnoreCase)
                    || string.Equals(fileType, "py", StringComparison.CurrentCultureIgnoreCase)
                    || string.Equals(fileType, "rft", StringComparison.CurrentCultureIgnoreCase)
                    || string.Equals(fileType, "rh", StringComparison.CurrentCultureIgnoreCase)
                    || string.Equals(fileType, "rtf", StringComparison.CurrentCultureIgnoreCase)
                    || string.Equals(fileType, "sldasm", StringComparison.CurrentCultureIgnoreCase)
                    || string.Equals(fileType, "sldprt", StringComparison.CurrentCultureIgnoreCase)
                    || string.Equals(fileType, "sql", StringComparison.CurrentCultureIgnoreCase)
                    || string.Equals(fileType, "step", StringComparison.CurrentCultureIgnoreCase)
                    || string.Equals(fileType, "stl", StringComparison.CurrentCultureIgnoreCase)
                    || string.Equals(fileType, "stp", StringComparison.CurrentCultureIgnoreCase)
                    || string.Equals(fileType, "swift", StringComparison.CurrentCultureIgnoreCase)
                    || string.Equals(fileType, "tif", StringComparison.CurrentCultureIgnoreCase)
                    || string.Equals(fileType, "tiff", StringComparison.CurrentCultureIgnoreCase)
                    || string.Equals(fileType, "txt", StringComparison.CurrentCultureIgnoreCase)
                    || string.Equals(fileType, "vb", StringComparison.CurrentCultureIgnoreCase)
                    || string.Equals(fileType, "vds", StringComparison.CurrentCultureIgnoreCase)
                    || string.Equals(fileType, "vsd", StringComparison.CurrentCultureIgnoreCase)
                    || string.Equals(fileType, "vsdx", StringComparison.CurrentCultureIgnoreCase)
                    || string.Equals(fileType, "x_b", StringComparison.CurrentCultureIgnoreCase)
                    || string.Equals(fileType, "x_t", StringComparison.CurrentCultureIgnoreCase)
                    || string.Equals(fileType, "xls", StringComparison.CurrentCultureIgnoreCase)
                    || string.Equals(fileType, "xlsb", StringComparison.CurrentCultureIgnoreCase)
                    || string.Equals(fileType, "xlsm", StringComparison.CurrentCultureIgnoreCase)
                    || string.Equals(fileType, "xlsx", StringComparison.CurrentCultureIgnoreCase)
                    || string.Equals(fileType, "xlt", StringComparison.CurrentCultureIgnoreCase)
                    || string.Equals(fileType, "xltm", StringComparison.CurrentCultureIgnoreCase)
                    || string.Equals(fileType, "xltx", StringComparison.CurrentCultureIgnoreCase)
                    || string.Equals(fileType, "xml", StringComparison.CurrentCultureIgnoreCase)
                    || string.Equals(fileType, "xmt_txt", StringComparison.CurrentCultureIgnoreCase)
                    || string.Equals(fileType, "zip", StringComparison.CurrentCultureIgnoreCase)
                    )
            {
                result = true;
            }
            return result;
        }

        #region Sdk Expiration and UI Expiration interconvert
        public static void SdkExpiration2ValiditySpecifyModel(sdk.Expiration expiration, out IExpiry expiry)
        {
            expiry = new NeverExpireImpl(); ;
            switch (expiration.type)
            {
                case sdk.ExpiryType.NEVER_EXPIRE:
                    expiry = new NeverExpireImpl();
                    break;
                case sdk.ExpiryType.RELATIVE_EXPIRE:
                    int years = (int)(expiration.Start >> 32);
                    int months = (int)expiration.Start;
                    int weeks = (int)(expiration.End >> 32);
                    int days = (int)expiration.End;
                    expiry = new RelativeImpl(years, months, weeks, days);
                    break;
                case sdk.ExpiryType.ABSOLUTE_EXPIRE:
                    expiry = new AbsoluteImpl(expiration.End);
                    break;
                case sdk.ExpiryType.RANGE_EXPIRE:
                    expiry = new RangeImpl(expiration.Start, expiration.End);
                    break;
            }

        }

        public static void ValiditySpecifyModel2SdkExpiration(out sdk.Expiration expiration, IExpiry expiry)
        {
            expiration = new sdk.Expiration();

            int exType = expiry.GetOpetion();
            //Get current year,month,day.
            int year = DateTime.Now.Year;
            int month = DateTime.Now.Month;
            int day = DateTime.Now.Day;
            DateTime dateStart = new DateTime(year, month, day, 0, 0, 0);
            switch (exType)
            {
                case 0:
                    INeverExpire neverExpire = (INeverExpire)expiry;
                    expiration.type = sdk.ExpiryType.NEVER_EXPIRE;
                    break;
                case 1:
                    IRelative relative = (IRelative)expiry;
                    int years = relative.GetYears();
                    int months = relative.GetMonths();
                    int weeks = relative.GetWeeks();
                    int days = relative.GetDays();
                    Console.WriteLine("years:{0}-months:{1}-weeks:{2}-days{3}", years, months, weeks, days);

                    expiration.type = sdk.ExpiryType.RELATIVE_EXPIRE;

                    expiration.Start = ((long)years << 32) + months;
                    expiration.End = ((long)weeks << 32) + days;

                    break;
                case 2:
                    IAbsolute absolute = (IAbsolute)expiry;
                    long endAbsDate = absolute.EndDate();
                    Console.WriteLine("absEndDate:{0}", endAbsDate);

                    expiration.type = sdk.ExpiryType.ABSOLUTE_EXPIRE;
                    expiration.Start = DateTimeHelper.DateTimeToTimestamp(dateStart);
                    expiration.End = endAbsDate;
                    break;
                case 3:
                    IRange range = (IRange)expiry;
                    long startDate = range.StartDate();
                    long endDate = range.EndDate();
                    Console.WriteLine("StartDate:{0},EndDate{1}", startDate, endDate);

                    expiration.type = sdk.ExpiryType.RANGE_EXPIRE;
                    expiration.Start = startDate;
                    expiration.End = endDate;
                    break;
            }

        }
        #endregion

        #region CustomControl data type convert
        /// <summary>
        /// SDK data type
        /// convert to
        /// CustomControls.componentPages.Preference.FolderItem type List
        /// <param name="rpmPaths"></param>
        /// <returns></returns>
        public static ObservableCollection<FolderItem> SdkRpmPaths2CusCtrFolderItem(List<string> rpmPaths)
        {
            ObservableCollection<FolderItem> folderItems = new ObservableCollection<FolderItem>();
            foreach (var item in rpmPaths)
            {
                FolderItem folderItem = new FolderItem()
                {
                    Icon = new BitmapImage(new Uri(@"/resources/icons/folder.png", UriKind.Relative)),
                    FolderName = string.IsNullOrWhiteSpace(Path.GetFileName(item)) ? item : Path.GetFileName(item),
                    FolderPath = item
                };
                folderItems.Add(folderItem);
            }
            return folderItems;
        }
        #endregion

    }

    // Used to popup dialogs
    public class DialogHelper
    {
        public static void ShowDlg_InitComponentFailed()
        {
            MessageBox.Show("Failed to launch application. Please contact your system administrator for further help.",
                      "SkyDRM DESKTOP",
                      MessageBoxButton.OK,
                      MessageBoxImage.Error);
        }
    }

    // Used to handle string ops.
    public class StringHelper
    {
        public static bool IsValidJsonStr_Fast(string jsonStr)
        {
            if (jsonStr == null)
            {
                return false;
            }
            if (jsonStr.Length < 2)
            {
                return false;
            }

            if (!jsonStr.StartsWith("{"))
            {
                return false;
            }

            if (!jsonStr.EndsWith("}"))
            {
                return false;
            }

            return true;
        }
    }

    public class FilterSystemPath
    {
        public static bool IsSystemFolderPath(string path)
        {
            bool result = false;
            foreach (var item in Enum.GetValues(typeof(Environment.SpecialFolder)))
            {
                bool isSucceed = Enum.TryParse(item.ToString(), out Environment.SpecialFolder value);
                if (isSucceed)
                {
                    result = path.Equals(Environment.GetFolderPath(value),StringComparison.OrdinalIgnoreCase);
                    if (result)
                    {
                        break;
                    }
                }
            }
            return result;
        }
        public static bool IsSpecialFolderPath(string path)
        {
            bool result = false;
            if (path.StartsWith(Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData) + @"\Nextlabs\SkyDRM",StringComparison.OrdinalIgnoreCase)
                || path.StartsWith(Environment.GetFolderPath(Environment.SpecialFolder.Windows),StringComparison.OrdinalIgnoreCase)
                || path.StartsWith(Environment.GetFolderPath(Environment.SpecialFolder.SystemX86),StringComparison.OrdinalIgnoreCase))
            {
                result = true;
            }
            return result;
        }

    }

}
