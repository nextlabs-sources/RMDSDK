using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace CustomControls.pages.Share
{
    public class ShareStatus
    {
        public static Int32 UNKNOWN = 0x00;
        public static Int32 EXPIRED = 0x01;
        public static Int32 SHARE_MY_VAULT_FILE_SUCCEEDED = 0x02;
        public static Int32 SHARE_PROJECT_FILE_SUCCEEDED = 0x04;
        public static Int32 RESHARE_SUCCEEDED = 0x08;
        public static Int32 IS_OWNER = 0x10;
        public static Int32 PROGRESS_BAR = 0x20;
        public static Int32 SHARE_BUTTON_ENABLED = 0x40;
        public static Int32 IS_FROM_MYVAULT = 0x80;
        public static Int32 IS_FROM_SHAREWITHME = 0x100;
        public static Int32 IS_FROM_PROJECT = 0x200;
        public static Int32 ERROR = 0x400;
    }
}
