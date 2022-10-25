using System;
using System.Collections.Generic;
using System.Drawing;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;

namespace nxcommondialog
{
    [ComVisible(true)]
    [Guid("6CB951A3-4FA2-4D8E-8C67-74BC8266AA97")]
    public interface ICommonDialog
    {
        /// <summary>
        /// Show the protect dialog
        /// </summary>
        /// <param name="filePath"> the normal file path.</param>
        /// <param name="iconName">the file type icon name and path.</param>
        /// <param name="dlgTitle">the dialog title content, the default is "NextLabs SkyDRM" if input para is empty.</param>
        /// <param name="actionBtnName">the action(positive) button content, the 
        /// default value is "Protect" if the input parameter is empty or null.</param>
        /// <param name="enabledProtectMethod">enable adhoc\central policy radio button or not.</param>
        /// <param name="jsonSelectedTags">returned selected tags for central policy protect; will return empty string for
        /// adhoc protect.</param>
        /// <param name="rights">returned selected rights for adhoc protect.</param>
        /// <param name="watermarkText">returned user defined watermark content for adhoc protect.</param>
        /// <param name="expiration">returned user defined expiration for adhoc protect.</param>
        /// <returns></returns>
        [DispId(1)]
        DialogResult ShowProtectDlg(
            string filePath,
            string iconName,
            string dlgTitle,
            string actionBtnName,
            NxlEnabledProtectMethod enabledProtectMethod,
            out string jsonSelectedTags,
            out long rights,
            out string watermarkText,
            out NxlExpiration expiration);

        /// <summary>
        /// Show checking protect dialog.
        /// </summary>
        /// <param name="filepath">the nxl file path, can not be empty or null</param>
        /// <param name="displayname">display name, if the para is empty, will extract file name and display it.</param>
        /// <param name="dlgTitle">the dialog title content, the default is "NextLabs SkyDRM" if input para is empty.</param>
        /// <returns></returns>
        [DispId(2)]
        DialogResult ShowFileInfoDlg(string filepath, string displayname, string dlgTitle);
    }
}
