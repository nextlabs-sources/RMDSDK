using System;
using System.Collections.Generic;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace WinFormControlLibrary
{
    /// <summary>
    /// FrmRightsSelect.cs DataModel
    /// </summary>
    public class RightsSelectDataModel
    {
        private Bitmap fileIcon= Properties.Resources.red_warning;
        private string filePath="";

        private bool adhocRadioIsEnable = true;
        private bool centralRadioIsEnable = true;
        // defult checked raidoButton, true is adhoc radio, false is central radio
        private bool adhocRadioDefultChecked = false;
        
        // Adhoc Page dataModel
        private string watermark = "$(Date)$(Time)$(Break)$(User)";
        // ui display standard time
        private string expireDate = "Never expire";
        private Expiration expiry=new Expiration() { type=ExpiryType.NEVER_EXPIRE, Start=0, End =0};
        private HashSet<Rights> selectedRights=new HashSet<Rights>() { Rights.RIGHT_VIEW, Rights.RIGHT_VALIDITY };

        // CentralPolicy page dataModel
        private Classification[] classifications = new Classification[0];
        private Dictionary<string, List<string>> selectedTags = new Dictionary<string, List<string>>();

        private bool isWarningVisible = false;
        private bool isValidTags = false;

        private bool isInfoTextVisible = false;
        private bool isSkipBtnVisible = false;
        private bool isPositiveBtnIsEnable = true;
        private string positiveBtnContent = "OK";
        private string cancelBtnContent = "Cancel";

        /// <summary>
        /// File icon, defult icon is "red_warning.png".
        /// </summary>
        public Bitmap FileIcon { get => fileIcon; set => fileIcon = value; }

        /// <summary>
        /// File Path,defult value is "".
        /// </summary>
        public string FilePath { get => filePath; set => filePath = value; }

        /// <summary>
        /// Adhoc RadioButton isEnable, defult value is true.
        /// </summary>
        public bool AdhocRadioIsEnable { get => adhocRadioIsEnable; set => adhocRadioIsEnable = value; }

        /// <summary>
        /// CentralPolicy RadioButton isEnable, defult value is true.
        /// </summary>
        public bool CentralRadioIsEnable { get => centralRadioIsEnable; set => centralRadioIsEnable = value; }

        /// <summary>
        /// RadioButton is Checked the first time open FrmRightsSelect.cs, true is adhoc radio, false is centralPolicy radio. defult value is false.
        /// </summary>
        public bool AdhocRadioDefultChecked { get => adhocRadioDefultChecked; set => adhocRadioDefultChecked = value; }

        /// <summary>
        /// Adhoc page watermark value,defult value is  "$(Date)$(Time)$(Break)$(User)"
        /// </summary>
        public string Watermark { get => watermark; set => watermark = value; }

        /// <summary>
        /// Adhoc page expireDate value,defult value is  "Never expire"
        /// </summary>
        public string ExpireDate { get => expireDate; set => expireDate = value; }

        /// <summary>
        /// Adhoc page Expiry value,defult value is "type=ExpiryType.NEVER_EXPIRE, Start=0, End =0", and change this will update ExpireDate property.
        /// </summary>
        public Expiration Expiry { get => expiry; set => expiry = value; }

        /// <summary>
        /// Adhoc page selected rights, defult add "RIGHT_VIEW" and "RIGHT_VALIDITY"
        /// </summary>
        public HashSet<Rights> SelectedRights { get => selectedRights; set => selectedRights = value; }

        /// <summary>
        /// CentralPolicy page tags, use for init UI, defult value is empty.
        /// </summary>
        public Classification[] Classifications { get => classifications; set => classifications = value; }

        /// <summary>
        /// CentralPolicy page selected tags
        /// </summary>
        public Dictionary<string, List<string>> SelectedTags { get => selectedTags; set => selectedTags = value; }

        /// <summary>
        /// Warning  describe visibility, false is Collapsed, true is Visible, defult value is false
        /// </summary>
        public bool IsWarningVisible { get => isWarningVisible; set => isWarningVisible = value; }

        /// <summary>
        /// Judge select tags is valid.
        /// </summary>
        public bool IsValidTags { get => isValidTags; set => isValidTags = value; }

        /// <summary>
        /// Button prompt message bar visibility, false is Hidden, true is Visible, defult value is false
        /// </summary>
        public bool IsInfoTextVisible { get => isInfoTextVisible; set => isInfoTextVisible = value; }

        /// <summary>
        /// Skip button isVisible, false is Hidden, true is Visible, defult value is false
        /// </summary>
        public bool IsSkipBtnVisible { get => isSkipBtnVisible; set => isSkipBtnVisible = value; }

        /// <summary>
        /// Positive Buttonn IsEnable, defult value is true
        /// </summary>
        public bool IsPositiveBtnIsEnable { get => isPositiveBtnIsEnable; set => isPositiveBtnIsEnable = value; }

        /// <summary>
        /// Positive Buttonn Content, defult value is "OK"
        /// </summary>
        public string PositiveBtnContent { get => positiveBtnContent; set => positiveBtnContent = value; }

        /// <summary>
        /// Cancel Button Content, defult value is "Cancel"
        /// </summary>
        public string CancelBtnContent { get => cancelBtnContent; set => cancelBtnContent = value; }
        
    }
}
