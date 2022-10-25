using ServiceManager.rmservmgr.common.helper;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace ServiceManager.resources.languages
{
   public class CultureStringInfo
    {
        /// <summary>
        /// Tray text
        /// </summary>
        public static string Tray_SkyDRM = CommonUtils.ApplicationFindResource("Tray_SkyDRM");
        public static string Tray_Text = CommonUtils.ApplicationFindResource("Tray_Text");
        public static string Tray_Text_User = CommonUtils.ApplicationFindResource("Tray_Text_User");
        public static string Tray_Text_Not_Sign_in = CommonUtils.ApplicationFindResource("Tray_Text_Not_Sign_in");

        /// <summary>
        /// Tray menu item
        /// </summary>
        public static string MenuItem_About = CommonUtils.ApplicationFindResource("Tray_MenuItem_About");
        public static string MenuItem_Login = CommonUtils.ApplicationFindResource("Tray_MenuItem_Login");
        public static string MenuItem_Logout = CommonUtils.ApplicationFindResource("Tray_MenuItem_Logout");
        public static string MenuItem_Exit = CommonUtils.ApplicationFindResource("Tray_MenuItem_Exit");

        ///<summary>
        /// ChooseServer Window
        ///<summary>
        public static string CheckUrl_Notify_UrlEmpty = CommonUtils.ApplicationFindResource("ChooseServerWin_CheckUrl_Notify_UrlEmpty");
        public static string CheckUrl_Notify_NetDisconnect = CommonUtils.ApplicationFindResource("ChooseServerWin_CheckUrl_Notify_NetDisconnect");
        public static string CheckUrl_Notify_UrlError = CommonUtils.ApplicationFindResource("ChooseServerWin_CheckUrl_Notify_UrlError");
        public static string CheckUrl_Notify_NetworkOrUrlError = CommonUtils.ApplicationFindResource("ChooseServerWin_CheckUrl_Notify_NetworkOrUrlError");

        /// <summary>
        /// ServiceManage Window
        /// </summary>
        public static string ServiceManageWin_Status_Online = CommonUtils.ApplicationFindResource("ServiceManageWin_Status_Online");
        public static string ServiceManageWin_Status_Offline = CommonUtils.ApplicationFindResource("ServiceManageWin_Status_Offline");

        public static string ServiceManageWin_All_Title = CommonUtils.ApplicationFindResource("ServiceManageWin_All_Title");

        public static string ServiceManageWin_All_CheckBox = CommonUtils.ApplicationFindResource("ServiceManageWin_All_CheckBox");
        public static string ServiceManageWin_All_Search_CheckBox = CommonUtils.ApplicationFindResource("ServiceManageWin_All_Search_CheckBox");

        public static string ServiceManageWin_One_Month = CommonUtils.ApplicationFindResource("ServiceManageWin_One_Month");
        public static string ServiceManageWin_One_Week = CommonUtils.ApplicationFindResource("ServiceManageWin_One_Week");
        public static string ServiceManageWin_Two_Weeks = CommonUtils.ApplicationFindResource("ServiceManageWin_Two_Weeks");
        public static string ServiceManageWin_Day_Ago = CommonUtils.ApplicationFindResource("ServiceManageWin_Day_Ago");
        public static string ServiceManageWin_Days_Ago = CommonUtils.ApplicationFindResource("ServiceManageWin_Days_Ago");
        public static string ServiceManageWin_Hour_Ago = CommonUtils.ApplicationFindResource("ServiceManageWin_Hour_Ago");
        public static string ServiceManageWin_Hours_Ago = CommonUtils.ApplicationFindResource("ServiceManageWin_Hours_Ago");
        public static string ServiceManageWin_Minute_Ago = CommonUtils.ApplicationFindResource("ServiceManageWin_Minute_Ago");
        public static string ServiceManageWin_Minutes_Ago = CommonUtils.ApplicationFindResource("ServiceManageWin_Minutes_Ago");
        public static string ServiceManageWin_Just_Now = CommonUtils.ApplicationFindResource("ServiceManageWin_Just_Now");

        /// <summary>
        /// File monitor
        /// </summary>
        public static string FileMonitor_Deny_Edit = CommonUtils.ApplicationFindResource("FileMonitor_Deny_Edit");

        /// <summary>
        /// Preference Window
        /// </summary>
        public static string PreferenceWin_SetRPMSystemFolder = CommonUtils.ApplicationFindResource("PreferenceWin_SetRPMSystemFolder");
        public static string PreferenceWin_SetRPMPathInvalid = CommonUtils.ApplicationFindResource("PreferenceWin_SetRPMPathInvalid");
        public static string PreferenceWin_SetRPMFailed = CommonUtils.ApplicationFindResource("PreferenceWin_SetRPMFailed");
        public static string PreferenceWin_GetRPMFailed = CommonUtils.ApplicationFindResource("PreferenceWin_GetRPMFailed");
        public static string PreferenceWin_ResetRPMFailed_For_Common = CommonUtils.ApplicationFindResource("PreferenceWin_ResetRPMFailed_For_Common");
        public static string PreferenceWin_Remove_RPMFailed = CommonUtils.ApplicationFindResource("PreferenceWin_Remove_RPMFailed");
        public static string PreferenceWin_MyFolder_Is_In_Use = CommonUtils.ApplicationFindResource("PreferenceWin_MyFolder_Is_In_Use");

    }
}
