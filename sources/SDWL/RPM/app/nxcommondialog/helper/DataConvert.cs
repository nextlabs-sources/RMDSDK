using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using WinFormControlLibrary;
using CommonDialog.sdk;
using SdkExpiration = CommonDialog.sdk.Expiration;
using SdkExpiryType = CommonDialog.sdk.ExpiryType;

namespace nxcommondialog.helper
{
    class DataConvert
    {
        /// <summary>
        /// WinFrmControlLibrary 'Rights' type convert to CommonDlg 'FileRights' type
        /// </summary>
        /// <param name="rights"></param>
        /// <returns></returns>
        public static List<NxlFileRights> FrmRights2CommonDlgRights(HashSet<Rights> rights)
        {
            List<NxlFileRights> fileRights = new List<NxlFileRights>();
            foreach (var item in rights)
            {
                switch (item)
                {
                    case Rights.RIGHT_VIEW:
                        fileRights.Add(NxlFileRights.RIGHT_VIEW);
                        break;
                    case Rights.RIGHT_EDIT:
                        fileRights.Add(NxlFileRights.RIGHT_EDIT);
                        break;
                    case Rights.RIGHT_PRINT:
                        fileRights.Add(NxlFileRights.RIGHT_PRINT);
                        break;
                    case Rights.RIGHT_CLIPBOARD:
                        fileRights.Add(NxlFileRights.RIGHT_CLIPBOARD);
                        break;
                    case Rights.RIGHT_SAVEAS:
                        fileRights.Add(NxlFileRights.RIGHT_DOWNLOAD);//PM required
                        break;
                    case Rights.RIGHT_DECRYPT:
                        fileRights.Add(NxlFileRights.RIGHT_DECRYPT);
                        break;
                    case Rights.RIGHT_SCREENCAPTURE:
                        fileRights.Add(NxlFileRights.RIGHT_SCREENCAPTURE);
                        break;
                    case Rights.RIGHT_SEND:
                        fileRights.Add(NxlFileRights.RIGHT_SEND);
                        break;
                    case Rights.RIGHT_CLASSIFY:
                        fileRights.Add(NxlFileRights.RIGHT_CLASSIFY);
                        break;
                    case Rights.RIGHT_SHARE:
                        fileRights.Add(NxlFileRights.RIGHT_SHARE);
                        break;
                    case Rights.RIGHT_DOWNLOAD:
                        fileRights.Add(NxlFileRights.RIGHT_DOWNLOAD);
                        break;
                    case Rights.RIGHT_WATERMARK:
                        fileRights.Add(NxlFileRights.RIGHT_WATERMARK);
                        break;
                    case Rights.RIGHT_VALIDITY:
                        break;
                    default:
                        break;
                }
            }
            return fileRights;
        }


        /// <summary>
        /// SDK 'FileRights' type convert to WinFrmControlLibrary 'Rights' type
        /// </summary>
        /// <param name="rights"></param>
        /// <returns></returns>
        public static HashSet<Rights> SdkRights2FrmRights(FileRights[] rights)
        {
            HashSet<Rights> fileRights = new HashSet<Rights>();
            fileRights.Add(Rights.RIGHT_VIEW);// defult rights

            foreach (var item in rights)
            {
                switch (item)
                {
                    case FileRights.RIGHT_VIEW:
                        fileRights.Add(Rights.RIGHT_VIEW);
                        break;
                    case FileRights.RIGHT_EDIT:
                        fileRights.Add(Rights.RIGHT_EDIT);
                        break;
                    case FileRights.RIGHT_PRINT:
                        fileRights.Add(Rights.RIGHT_PRINT);
                        break;
                    case FileRights.RIGHT_CLIPBOARD:
                        fileRights.Add(Rights.RIGHT_CLIPBOARD);
                        break;
                    case FileRights.RIGHT_SAVEAS:
                        fileRights.Add(Rights.RIGHT_SAVEAS);
                        break;
                    case FileRights.RIGHT_DECRYPT:
                        fileRights.Add(Rights.RIGHT_DECRYPT);
                        break;
                    case FileRights.RIGHT_SCREENCAPTURE:
                        fileRights.Add(Rights.RIGHT_SCREENCAPTURE);
                        break;
                    case FileRights.RIGHT_SEND:
                        fileRights.Add(Rights.RIGHT_SEND);
                        break;
                    case FileRights.RIGHT_CLASSIFY:
                        fileRights.Add(Rights.RIGHT_CLASSIFY);
                        break;
                    case FileRights.RIGHT_SHARE:
                        fileRights.Add(Rights.RIGHT_SHARE);
                        break;
                    case FileRights.RIGHT_DOWNLOAD:
                        fileRights.Add(Rights.RIGHT_SAVEAS);
                        break;
                    case FileRights.RIGHT_WATERMARK:
                        fileRights.Add(Rights.RIGHT_WATERMARK);
                        break;
                    //case FileRights.RIGHT_VALIDITY:
                    //    break;
                    default:
                        break;
                }
            }
            return fileRights;
        }

        /// <summary>
        /// WinFormControlLibrary 'Expiration' type convert to CommonDlg 'Expiration' type
        /// </summary>
        /// <param name="frmExpiration"></param>
        /// <returns></returns>
        public static NxlExpiration FrmExpt2CommonDlgExpt(WinFormControlLibrary.Expiration frmExpiration)
        {
            NxlExpiration expiry = new NxlExpiration();
            switch (frmExpiration.type)
            {
                case WinFormControlLibrary.ExpiryType.NEVER_EXPIRE:
                    expiry.type = NxlExpiryType.NEVER_EXPIRE;
                    break;
                case WinFormControlLibrary.ExpiryType.RELATIVE_EXPIRE:
                    expiry.type = NxlExpiryType.RELATIVE_EXPIRE;
                    break;
                case WinFormControlLibrary.ExpiryType.ABSOLUTE_EXPIRE:
                    expiry.type = NxlExpiryType.ABSOLUTE_EXPIRE;
                    break;
                case WinFormControlLibrary.ExpiryType.RANGE_EXPIRE:
                    expiry.type = NxlExpiryType.RANGE_EXPIRE;
                    break;
                default:
                    break;
            }
            expiry.Start = frmExpiration.Start;
            expiry.End = frmExpiration.End;

            return expiry;
        }


        /// <summary>
        /// Sdk 'Expiration' type convert to WinFormControlLibrary 'Expiration' type
        /// </summary>
        /// <param name="sdkExpiration"></param>
        /// <returns></returns>
        public static WinFormControlLibrary.Expiration SdkExpt2ControlLibExpt(SdkExpiration sdkExpiration)
        {
            WinFormControlLibrary.Expiration expiry = new WinFormControlLibrary.Expiration();
            switch (sdkExpiration.type)
            {
                case SdkExpiryType.NEVER_EXPIRE:
                    expiry.type = WinFormControlLibrary.ExpiryType.NEVER_EXPIRE;
                    break;
                case SdkExpiryType.RELATIVE_EXPIRE:
                    expiry.type = WinFormControlLibrary.ExpiryType.RELATIVE_EXPIRE;
                    break;
                case SdkExpiryType.ABSOLUTE_EXPIRE:
                    expiry.type = WinFormControlLibrary.ExpiryType.ABSOLUTE_EXPIRE;
                    break;
                case SdkExpiryType.RANGE_EXPIRE:
                    expiry.type = WinFormControlLibrary.ExpiryType.RANGE_EXPIRE;
                    break;
                default:
                    break;
            }
            expiry.Start = sdkExpiration.Start;
            expiry.End = sdkExpiration.End;

            return expiry;
        }


        /// <summary>
        /// SDK 'ProjectClassification' type convert to WinFrmControlLibrary 'Classification' type
        /// </summary>
        /// <param name="sdkTag"></param>
        /// <returns></returns>
        public static Classification[] SdkTag2FrmTag(ProjectClassification[] sdkTag)
        {
            if (sdkTag == null || sdkTag.Length == 0)
            {
                return new Classification[0];
            }

            Classification[] tags = new Classification[sdkTag.Length];
            for (int i = 0; i < sdkTag.Length; i++)
            {
                tags[i].name = sdkTag[i].name;
                tags[i].isMultiSelect = sdkTag[i].isMultiSelect;
                tags[i].isMandatory = sdkTag[i].isMandatory;
                tags[i].labels = sdkTag[i].labels;
            }
            return tags;
        }

        /// <summary>
        /// Convert 'List enum' type rights to 'long'
        /// </summary>
        /// <param name="lists"></param>
        /// <returns></returns>
        public static long ListEnumRights2Long(List<NxlFileRights> lists)
        {
            long ret = 0;

            foreach(NxlFileRights one in lists)
            {
                long r = (long)one;
                ret |= r;
            }

            return ret;
        }

    }
}
