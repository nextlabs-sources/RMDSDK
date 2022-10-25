using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Text.RegularExpressions;
using System.Threading.Tasks;
using Newtonsoft.Json;

namespace CommonDialog.sdk.helper
{
    class Utils
    {
        // like log-2019-01-24-07-04-28.txt
        // pattern-match "-2019-01-24-07-04-28" replaced with latest lcoal timestamp
        public static string TIMESTAMP_PATTERN = @"-\d{4}-\d{2}-\d{2}-\d{2}-\d{2}-\d{2}";
        public static bool Replace(string inputStr, out string outputStr, string pattern, RegexOptions regexOptions)
        {
            bool result = false;
            outputStr = inputStr;
            try
            {
                if (!string.IsNullOrEmpty(pattern))
                {
                    Regex reg = new Regex(pattern, regexOptions);
                    string newString = string.Empty;
                    if (reg.IsMatch(inputStr))
                    {
                        outputStr = reg.Replace(inputStr, newString);
                        result = true;
                    }
                }
            }
            catch (Exception)
            {
            }
            return result;
        }

        public static bool IsValidJsonStr_Fast(string jsonStr)
        {
            if (jsonStr == null)
            {
                return false;
            }
            if (jsonStr.Length < 2)
            {
                return false;
            }

            if (!jsonStr.StartsWith("{"))
            {
                return false;
            }

            if (!jsonStr.EndsWith("}"))
            {
                return false;
            }

            return true;
        }

        static public List<FileRights> ParseRights(Int64 v)
        {
            var rt = new List<FileRights>();

            if ((v & (Int64)FileRights.RIGHT_VIEW) > 0)
            {
                rt.Add(FileRights.RIGHT_VIEW);
            }
            if ((v & (Int64)FileRights.RIGHT_EDIT) > 0)
            {
                rt.Add(FileRights.RIGHT_EDIT);
            }
            if ((v & (Int64)FileRights.RIGHT_PRINT) > 0)
            {
                rt.Add(FileRights.RIGHT_PRINT);
            }
            if ((v & (Int64)FileRights.RIGHT_CLIPBOARD) > 0)
            {
                rt.Add(FileRights.RIGHT_CLIPBOARD);
            }
            if ((v & (Int64)FileRights.RIGHT_SAVEAS) > 0)
            {
                rt.Add(FileRights.RIGHT_SAVEAS);
            }
            if ((v & (Int64)FileRights.RIGHT_DECRYPT) > 0)
            {
                rt.Add(FileRights.RIGHT_DECRYPT);
            }
            if ((v & (Int64)FileRights.RIGHT_SCREENCAPTURE) > 0)
            {
                rt.Add(FileRights.RIGHT_SCREENCAPTURE);
            }
            if ((v & (Int64)FileRights.RIGHT_SEND) > 0)
            {
                rt.Add(FileRights.RIGHT_SEND);
            }
            if ((v & (Int64)FileRights.RIGHT_CLASSIFY) > 0)
            {
                rt.Add(FileRights.RIGHT_CLASSIFY);
            }
            if ((v & (Int64)FileRights.RIGHT_SHARE) > 0)
            {
                rt.Add(FileRights.RIGHT_SHARE);
            }
            if ((v & (Int64)FileRights.RIGHT_DOWNLOAD) > 0)
            {
                rt.Add(FileRights.RIGHT_DOWNLOAD);
            }
            if ((v & (Int64)FileRights.RIGHT_WATERMARK) > 0)
            {
                rt.Add(FileRights.RIGHT_WATERMARK);
            }

            return rt;
        }

        static public Dictionary<string, List<string>> ParseClassificationTag(string value)
        {
            var rt = new Dictionary<string, List<string>>();
            if (value.Length == 0)
            {
                return rt;
            }
            if (!Utils.IsValidJsonStr_Fast(value))
            {
                return rt;
            }

            return JsonConvert.DeserializeObject<Dictionary<string, List<string>>>(value);
        }

        static public NxlFileFingerPrint Convert(User.InternalFingerPrint fp)
        {
            NxlFileFingerPrint v = new NxlFileFingerPrint()
            {
                name = fp.name,
                created = fp.created,
                modified = fp.modified,
                localPath = fp.localPath,
                size = fp.size,
                isOwner = fp.isOwner == 1 ? true : false,
                isFromMyVault = fp.isFromMyVault == 1 ? true : false,
                isFromPorject = fp.isFromPorject == 1 ? true : false,
                isFromSystemBucket = fp.isFromSystemBucket == 1 ? true : false,
                projectId = (int)fp.projectId,
                isByAdHoc = fp.isByAdHoc == 1 ? true : false,
                isByCentrolPolicy = fp.isByCentrolPolicy == 1 ? true : false,
                tags = ParseClassificationTag(fp.tags),
                rawtags = fp.tags,
                expiration = fp.expiration,
                adhocWatermark = fp.adhocWatermark,
                rights = ParseRights(fp.rights).ToArray(),
                hasAdminRights = fp.hasAdminRights == 1 ? true : false,
                duid = fp.duid
            };
            return v;
        }

        public static MyVaultMetaData Convert(User.InternalMyVaultMetaData md)
        {
            MyVaultMetaData metaData = new MyVaultMetaData
            {
                name = md.name,
                fileLink = md.fileLink,

                lastModified = md.lastModified,
                protectedOn = md.protectedOn,
                sharedOn = md.sharedOn,
                protectionType = md.protectionType,

                isShared = md.isShared == 1 ? true : false,
                isDeleted = md.isDeleted == 1 ? true : false,
                isRevoked = md.isRevoked == 1 ? true : false,
                isOwner = md.isOwner == 1 ? true : false,
                isNxl = md.isNxl == 1 ? true : false,

                recipents = ParseRecipents(md.recipents),
                pathDisplay = md.pathDisplay,
                pathId = md.pathId,
                tags = ParseClassificationTag(md.tags),
                rawTags = md.tags,
                expiration = md.expiration
            };
            return metaData;
        }

        public static List<string> ParseRecipents(string recipents)
        {
            List<string> ret = new List<string>();

            if (!string.IsNullOrEmpty(recipents))
            {
                foreach (var i in recipents.Split(new char[] { ';' }))
                {
                    if (!string.IsNullOrEmpty(i))
                    {
                        ret.Add(i);
                    }
                }
            }

            return ret;
        }
    }
}
