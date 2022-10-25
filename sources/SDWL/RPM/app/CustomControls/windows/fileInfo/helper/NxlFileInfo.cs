using CustomControls.pages.DigitalRights.model;
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

    public class NXlFileInfo
    {
        public string Name { get; }

        public long Size { get; }

        public bool IsOwner { get; }

        public bool IsByAdHoc { get; }

        public bool IsByCentrolPolicy { get; }

        public DateTime LastModified { get; }  // windows-UTC

        public string LocalDiskPath { get; }  // file stored at local disk

        public string RmsRemotePath { get; }  // file in RMS remote, like /aaa/bbb/ccc/ddd.txt.nxl

        public bool IsCreatedLocal { get; }   // ture for local added file, flase RMS remote file 

        public string[] Emails { get; }     // shared to who, or who share this file to you

        public FileRights[] Rights { get; }

        public string WaterMark { get; }

        public Expiration Expiration { get; }

        public Dictionary<string, List<string>> Tags { get; }

        public string RawTags { get; }
    }
}
