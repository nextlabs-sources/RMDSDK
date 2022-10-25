using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;

namespace CustomControls.windows.fileInfo.helper
{
    public enum ExpiryType
    {
        NEVER_EXPIRE = 0,
        RELATIVE_EXPIRE,
        ABSOLUTE_EXPIRE,
        RANGE_EXPIRE,
    }

    [StructLayout(LayoutKind.Sequential, Pack = 8)]
    public struct Expiration
    {
        public ExpiryType type;
        public Int64 Start;
        public Int64 End;
    }

}
