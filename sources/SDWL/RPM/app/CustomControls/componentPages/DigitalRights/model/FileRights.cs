using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace CustomControls.pages.DigitalRights.model
{
    public enum FileRights
    {
        RIGHT_VIEW = 0x1,
        RIGHT_EDIT = 0x2,
        RIGHT_PRINT = 0x4,
        RIGHT_CLIPBOARD = 0x8,
        RIGHT_SAVEAS = 0x10,
        RIGHT_DECRYPT = 0x20,
        RIGHT_SCREENCAPTURE = 0x40,
        RIGHT_SEND = 0x80,
        RIGHT_CLASSIFY = 0x100,
        RIGHT_SHARE = 0x200,
        RIGHT_DOWNLOAD = 0x400,
        RIGHT_VALIDITY = 0x800,
        RIGHT_WATERMARK = 0x1000
    }
}
