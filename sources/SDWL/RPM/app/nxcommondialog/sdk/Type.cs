using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;
using Newtonsoft.Json;

namespace CommonDialog.sdk
{
    enum FileRights
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
        RIGHT_WATERMARK = 0x40000000
    }


    enum UserType
    {
        SKYDRM = 0,
        SMAL,
        GOOGLE,
        FACEBOOK,
        YAHOO
    }

    [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Unicode, Pack = 8)]
    struct WaterMarkInfo
    {
        [MarshalAs(UnmanagedType.LPWStr)]
        public string text;
        [MarshalAs(UnmanagedType.LPWStr)]
        public string fontName;
        [MarshalAs(UnmanagedType.LPWStr)]
        public string fontColor;
        public int fontSize;
        public int transparency;
        public int rotation;
        public int repeat;
    }

    enum ExpiryType
    {
        NEVER_EXPIRE = 0,
        RELATIVE_EXPIRE,
        ABSOLUTE_EXPIRE,
        RANGE_EXPIRE,
    }
    [StructLayout(LayoutKind.Sequential, Pack = 8)]
    struct Expiration
    {
        public ExpiryType type;
        public Int64 Start;
        public Int64 End;
    }

    [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Unicode, Pack = 8)]
    struct ActivityInfo
    {
        public string duid;
        public string email;
        public string operation;
        public string deviceType;
        public string deviceId;
        public string accessResult;
        public UInt64 accessTime;
    }

    [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Unicode, Pack = 8)]
    struct ProjectInfo
    {
        public int id;
        public string name;
        public string displayName;
        public string description;
        public int isOwner;
        public Int64 totalfiles;
        public string tenantId;
        public Int64 isAdhocEnabled;
    }

    [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Unicode, Pack = 8)]
    struct ProjectFileInfo
    {
        public string id;           //rms file id
        public string duid;
        public string displayPath;  // for UI ,path in UI is case-sensitive
        public string pathId;       // for code, path in code is NOT case-sensitive
        public string nxlName;
        public Int64 lastModifedTime;
        public Int64 creationTime;
        public Int64 fileSize;
        public Int32 isFolder;
        public Int32 ownerId;
        public string ownerDisplayName;
        public string ownerEmail;


    }
    enum ProjectFileDownloadType
    {
        Normal = 0,
        ForViewer = 1,
        ForOffline = 2
    }

    enum ProjectFilterType
    {
        All = 0,
        OwnedByMe = 1,
        OwnedByOther = 2
    }

    [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Unicode, Pack = 8)]
    struct ProjectClassification
    {
        public string name;
        public bool isMultiSelect;
        public bool isMandatory;
        public Dictionary<String, Boolean> labels;  // <LabelValue,isDefault>
    }



    [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Unicode, Pack = 8)]
    struct MyVaultFileInfo
    {
        public string pathId;
        public string displayPath;
        public string repoId;
        public string duid;
        public string nxlName;

        public Int64 lastModifiedTime;
        public Int64 creationTime;
        public Int64 sharedTime;
        public string sharedWithList;
        public Int64 size;

        public Int64 isDeleted;
        public Int64 isRevoked;
        public Int64 isShared;

        // misc.
        public string sourceRepoType;
        public string sourceFileDisplayPath;
        public string sourceFilePathId;
        public string sourceRepoName;
        public string sourceRepoId;
    }


    class UserSelectTags
    {
        // List<tag<tagName,tagvalues> >
        private List<KeyValuePair<string, List<string>>> tags;

        public UserSelectTags()
        {
            this.tags = new List<KeyValuePair<string, List<string>>>();
        }

        public void AddTag(string tagName, string tagValue)
        {
            var node = tags.Find((i) =>
            {
                if (i.Key.Equals(tagName))
                {
                    return true;
                }
                else
                {
                    return false;
                }

            });

            if (node.Key != null && node.Key.Equals(tagName))
            {
                if (!node.Value.Contains(tagValue))
                {
                    node.Value.Add(tagValue);
                }
            }
            else
            {
                var newNode = new KeyValuePair<string, List<string>>(tagName, new List<string>());
                newNode.Value.Add(tagValue);
                this.tags.Add(newNode);
            }
        }
        public void AddTag(string tagName, List<string> tagValues)
        {
            var node = tags.Find((i) =>
            {
                if (i.Key.Equals(tagName))
                {
                    return true;
                }
                else
                {
                    return false;
                }

            });
            if (node.Key != null && node.Key.Equals(tagName))
            {
                foreach (var i in tagValues)
                {
                    if (!node.Value.Contains(i))
                    {
                        node.Value.Add(i);
                    }
                }
            }
            else
            {
                var newNode = new KeyValuePair<string, List<string>>(tagName, new List<string>());
                newNode.Value.AddRange(tagValues);
                this.tags.Add(newNode);
            }

        }

        public bool IsEmpty()
        {
            return tags.Count == 0;
        }

        public string ToJsonString()
        {
            if (tags.Count == 0)
            {
                return "{}";
            }
            else
            {
                StringBuilder sb = new StringBuilder();
                StringWriter sw = new StringWriter(sb);
                using (JsonWriter writer = new JsonTextWriter(sw))
                {
                    writer.Formatting = Formatting.Indented;

                    writer.WriteStartObject();
                    {
                        foreach (var tag in tags)
                        {
                            writer.WritePropertyName(tag.Key);
                            writer.WriteStartArray();
                            {
                                foreach (var value in tag.Value)
                                {
                                    writer.WriteValue(value);
                                }
                            }
                            writer.WriteEndArray();
                        }
                    }
                    writer.WriteEndObject();
                }
                return sb.ToString();
            }
        }
    }

    enum DownlaodMyVaultFileType
    {
        Normal = 0,
        ForVeiwer = 1,
        ForOffline = 2
    }

    [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Unicode, Pack = 8)]
    struct SharedWithMeFileInfo
    {
        public string duid;
        public string nxlName;
        public string fileType;
        public Int64 size;
        public Int64 sharedDateJavaTimeMillis;
        public string sharedByWho;
        public string transactionId;
        public string transactionCode;
        public string sharedlinkUrl;
        public string rights;
        public string comments;
        public bool isOwner;
    }

    enum NxlOpLog
    {
        Protect = 1,
        Share,
        RemoveUser,
        View,
        Print,
        Download,
        Edit,
        Revoke,
        Decrypt,
        CopyContent,
        CaptureScreen,
        Classify,
        Reshare,
        Delete
    };


    // fetch as detailed as possible nxl related file infos
    // find a good way to work around the poor designed rmsdk  
    [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Unicode, Pack = 8)]
    struct NxlFileFingerPrint
    {
        public string name;
        public string localPath;
        public Int64 size;
        public Int64 created;
        public Int64 modified;
        public bool isOwner;

        public bool isFromMyVault;

        public bool isFromPorject;
        public bool isFromSystemBucket;
        public int projectId;

        public bool isByAdHoc;
        public bool isByCentrolPolicy;

        public Dictionary<string, List<string>> tags;
        public string rawtags;
        public Expiration expiration;
        public string adhocWatermark;
        public FileRights[] rights;

        public bool hasAdminRights;
        public string duid;
        public bool HasRight(FileRights r)
        {
            // Owner has all rigths
            if (isOwner && isByAdHoc && !isByCentrolPolicy)
            {
                return true;
            }

            if (rights == null)
            {
                return false;
            }
            foreach (var i in rights)
            {
                if (i == r)
                {
                    return true;
                }
            }
            return false;
        }


        public IList<string> Helper_GetRightsStr(bool bAddIfHasWatermark = true, bool bForceAddValidity = true)
        {
            var rt = new List<string>();
            foreach (FileRights f in rights)
            {
                switch (f)
                {
                    case FileRights.RIGHT_VIEW:
                        rt.Add("View");
                        break;
                    case FileRights.RIGHT_EDIT:
                        rt.Add("Edit");
                        break;
                    case FileRights.RIGHT_PRINT:
                        rt.Add("Print");
                        break;
                    case FileRights.RIGHT_CLIPBOARD:
                        rt.Add("Clipboard");
                        break;
                    case FileRights.RIGHT_SAVEAS:
                        rt.Add("SaveAs");
                        break;
                    case FileRights.RIGHT_DECRYPT:
                        rt.Add("Decrypt");
                        break;
                    case FileRights.RIGHT_SCREENCAPTURE:
                        rt.Add("ScreenCapture");
                        break;
                    case FileRights.RIGHT_SEND:
                        rt.Add("Send");
                        break;
                    case FileRights.RIGHT_CLASSIFY:
                        rt.Add("Classify");
                        break;
                    case FileRights.RIGHT_SHARE:
                        rt.Add("Share");
                        break;
                    case FileRights.RIGHT_DOWNLOAD:
                        // as PM required Windows platform must regard download as SaveAS
                        rt.Add("SaveAs");
                        break;
                }
            }


            if (bAddIfHasWatermark)
            {
                if (adhocWatermark.Length > 1)
                {
                    rt.Add("Watermark");
                }
            }
            if (bForceAddValidity)
            {
                //
                // comments, by current design, requreied to add the riths "Validity" compulsorily 
                //
                rt.Add("Validity");
            }

            return rt;
        }

        static private List<string> GenerateAllRights()
        {
            var rt = new List<string>();
            rt.Add("View");
            rt.Add("Edit");
            rt.Add("Print");
            rt.Add("Clipboard");
            rt.Add("SaveAs");
            rt.Add("Decrypt");
            rt.Add("ScreenCapture");
            rt.Add("Send");
            rt.Add("Classify");
            rt.Add("Share");
            rt.Add("Download");
            return rt;
        }


        static public bool IsSameTags(Dictionary<string, List<string>> l, Dictionary<string, List<string>> r)
        {
            if (l.Count != r.Count)
            {
                return false;
            }
            if (l.Count == 0)
            {
                return true;
            }

            // convert all string to lower case
            Dictionary<string, List<string>> ll = new Dictionary<string, List<string>>();
            foreach (var i in l)
            {
                var k = i.Key.ToLower();
                var v = new List<string>();
                foreach (var j in i.Value)
                {
                    v.Add(j.ToLower());
                }
                ll.Add(k, v);
            }

            Dictionary<string, List<string>> rr = new Dictionary<string, List<string>>();
            foreach (var i in r)
            {
                var k = i.Key.ToLower();
                var v = new List<string>();
                foreach (var j in i.Value)
                {
                    v.Add(j.ToLower());
                }
                rr.Add(k, v);
            }

            // keys are same
            var lk = ll.Keys;
            var rk = rr.Keys;

            foreach (var i in lk)
            {
                // each key in l is contained in r
                if (!rk.Contains(i))
                {
                    return false;
                }
                // check values
                var liv = ll[i];
                var riv = rr[i];
                if (liv.Count != riv.Count)
                {
                    return false;
                }
                // each liv's values is contained in riv
                foreach (var j in liv)
                {
                    if (!riv.Contains(j))
                    {
                        return false;
                    }
                }
            }
            return true;
        }

    }

    struct MyVaultMetaData
    {
        public string name;
        public string fileLink;

        public Int64 lastModified;
        public Int64 protectedOn;
        public Int64 sharedOn;
        public Int64 protectionType;

        public bool isShared;
        public bool isDeleted;
        public bool isRevoked;
        public bool isOwner;
        public bool isNxl;

        public List<string> recipents;
        public string pathDisplay;
        public string pathId;
        public Dictionary<string, List<string>> tags;
        public string rawTags;

        public Expiration expiration;
    }

    [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Unicode, Pack = 8)]
    struct InternalCentralRights
    {
        public Int64 rights;
        public IntPtr waterMarks;
        public Int64 wmSize;
    }
}
