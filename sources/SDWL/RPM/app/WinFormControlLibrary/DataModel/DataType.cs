using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace WinFormControlLibrary
{
    public enum Rights
    {
        RIGHT_VIEW = 0,
        RIGHT_EDIT,
        RIGHT_PRINT,
        RIGHT_CLIPBOARD,
        RIGHT_SAVEAS,
        RIGHT_DECRYPT,
        RIGHT_SCREENCAPTURE,
        RIGHT_SEND,
        RIGHT_CLASSIFY,
        RIGHT_SHARE,
        RIGHT_DOWNLOAD,
        RIGHT_WATERMARK,
        RIGHT_VALIDITY
    }

    public enum ExpiryType
    {
        NEVER_EXPIRE = 0,
        RELATIVE_EXPIRE,
        ABSOLUTE_EXPIRE,
        RANGE_EXPIRE,
    }
    public struct Expiration
    {
        public ExpiryType type;
        public Int64 Start;
        public Int64 End;
    }

    /// <summary>
    /// Project Classification
    /// </summary>
    public struct Classification
    {
        public string name;
        public bool isMultiSelect;
        public bool isMandatory;

        /// <summary>
        /// string is  labels value, bool is defult selected
        /// </summary>
        public Dictionary<String, Boolean> labels;  // <LabelValue,isDefault>
    }
}
