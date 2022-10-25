using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace ServiceManager.rmservmgr.sdk
{
    class Config
    {
        //public const string DLL_NAME = @"D:\OyeProject\CSharp\rmd-windows\SkydrmLocal\Debug\sdkwrapper.dll";
        public const string DLL_NAME = "sdkwrapper.dll";

        public const string Default_Router = @"https://www.skydrm.com";
        public const string Default_Tenant = @"skydrm.com";

        public const int Deault_Heartbeat = 180;

        // Using local server
        //public const string Default_Router = @"https://rms-centos7303.qapf1.qalab01.nextlabs.com:8443";
        //public const string Default_Tenant = @"ee4eddd8-81fc-4488-a1a7-2a71505c44b2"; 

        public const uint RMS_ERROR_BASE = 0xF000;

        public const uint SDK_NETWORK_ERROR_BASE = 12007;
        public const uint SDK_NETWORK_ERROR_MAX = SDK_NETWORK_ERROR_BASE + 188;



        static public string GetSDKCommonError(uint ErrorCode)
        {
            switch (ErrorCode)
            {
                case 0:
                    return "SDWL_SUCCESS";
                case 3:
                    return "SDWL_PATH_NOT_FOUND";
                case 11:
                    return "SDWL_INVALID_JSON_FORMAT";
                case 13:
                    return "SDWL_INVALID_DATA";
                case 1603:
                    return "SDWL_CERT_NOT_INSTALLED";
                case 21:
                    return "SDWL_NOT_READY";
                case 8:
                    return "SDWL_NOT_ENOUGH_MEMORY";
                case 1168:
                    return "SDWL_NOT_FOUND";
                case 1359:
                    return "SDWL_INTERNAL_ERROR";
                case 1245:
                    return "SDWL_LOGIN_REQUIRED";
                case 5:
                    return "SDWL_ACCESS_DENIED";
                case 170:
                    return "SDWL_BUSY";
                case 183:
                    return "SDWL_ALREADY_EXISTS";
                default:
                    return "undefined or undocumented," + ErrorCode;
            }
        }

    }
}
