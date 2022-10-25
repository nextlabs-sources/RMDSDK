using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace CommonDialog.sdk.helper
{
    /// <summary>
    /// Note: now hard code the string first in order to impl the sdk share link conviniently.
    /// </summary>
    class CultureStringInfo
    {
        public static string Exception_Sdk_General = "There is an internal system error, contact your system administrator.";
        public static string Exception_Sdk_Network_IO = "Can't connect to the network.";
        public static string Exception_Sdk_Insufficient_Rights = "You have no permission to access the file.";
        public static string Exception_Sdk_Rest_General = "There is an internal system error, contact your system administrator.";
        public static string Exception_Sdk_Rest_400_InvalidParam = "Invalid request, contact your system administrator.";
        public static string Exception_Sdk_Rest_401_Authentication_Failed = "Your session has expired or become invalid, please login again.";
        public static string Exception_Sdk_Rest_403_AccessForbidden = "You are not authorized to perform the operation.";
        public static string Exception_Sdk_Rest_404_NotFound = "Unable to perform your request, resource not found.";
        public static string Exception_Sdk_Rest_500_ServerInternal = "There is an internal system error, contact your system administrator.";
        public static string Exception_Sdk_Rest_6001_StorageExceeded = "Your have exceeded the maximum storage limit.";
        //for myvault 
        public static string Exception_Sdk_Rest_MyVault_304_RevokedFile = "Permission to access the file has been revoked.";
        public static string Exception_Sdk_Rest_MyVault_4003_ExpiredFile = "Permission to access the file has expired.";
        public static string Exception_Sdk_Rest_MyVault_5001_InvalidNxl = "Invalid NXL file.";
        public static string Exception_Sdk_Rest_MyVault_5002_InvalidRepoMetadata = "Invalid metadata.";
        public static string Exception_Sdk_Rest_MyVault_5003_InvalidFileName = "Invalid file name.";
        public static string Exception_Sdk_Rest_MyVault_5004_InvalidFileExtension = "Invalid file extension.";
        public static string Exception_Sdk_Rest_MyVault_5005_InvalidFileExtension = "Invalid file.";
    }
}
