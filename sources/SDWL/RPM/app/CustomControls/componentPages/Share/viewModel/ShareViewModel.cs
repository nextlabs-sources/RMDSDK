using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.ComponentModel;
using System.Linq;
using System.Text;
using System.Windows;

namespace CustomControls.pages.Share
{
    public class ShareViewModel : INotifyPropertyChanged
    {
        private Int32 status;
        private string fileName;
        private string watermark;
        private string validity;
        private string operationTitle;
        private bool isOwner = false;
        private ObservableCollection<RightsItem> rights = new ObservableCollection<RightsItem>();
        private ObservableCollection<EmailItem> emails = new ObservableCollection<EmailItem>();

        //manage email List, whether its valid or not
        private List<string> sharedEmailLists = new List<string>();
        private List<string> dirtyEmailLists = new List<string>();

        public Int32 Status
        {
            get
            {
                return status;
            }
            set
            {
                status = value;
                OnPropertyChanged("Status");
            }
        }

        public string FileName
        {
            get
            {
                return fileName;
            }
            set
            {
                fileName = value;
                OnPropertyChanged("FileName");
            }
        }

        public string Watermark
        {
            get
            {
                return watermark;
            }
            set
            {
                watermark = value;
                OnPropertyChanged("WatermarkValue");
            }
        }

        public string Validity
        {
            get
            {
                return validity;
            }
            set
            {
                validity = value;
                OnPropertyChanged("ValidityValue");
            }
        }

        public string OperationTitle
        {
            get
            {
                return operationTitle;
            }
            set
            {
                operationTitle = value;
                OnPropertyChanged("OperationTitle");
            }
        }

        public bool IsOwner
        {
            get
            {
                return isOwner;
            }
            set
            {
                isOwner = value;
                OnPropertyChanged("IsOwner");
            }
        }

        public ObservableCollection<RightsItem> Rights
        {
            get
            {
                return rights;
            }
            set
            {
                rights = value;
                OnPropertyChanged("Rights");
            }
        }

        public ObservableCollection<EmailItem> Emails
        {
            get
            {
                return emails;
            }
            set
            {
                emails = value;
                OnPropertyChanged("Emails");
            }
        }

        public event PropertyChangedEventHandler PropertyChanged;

        protected void OnPropertyChanged(string propertyName)
        {
            PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(propertyName));
        }

        public ShareViewModel(string fileName, string watermark,string validity, IList<string> rights,bool isOwner, bool expired, bool isFromMyVault, bool isFromPorject)
        {
            this.FileName = fileName;
            this.Watermark = watermark;
            this.Validity = validity;
            this.OperationTitle = "Share a protected file";
            this.IsOwner = isOwner;
            AddRightsItem(rights, ref this.rights);

            Status = Status | ShareStatus.SHARE_BUTTON_ENABLED;
            if (expired)
            {
                Status = Status | ShareStatus.EXPIRED;
            }

            if (isFromMyVault)
            {
                if (isOwner)
                {
                    // From My Vault
                    Status = Status | ShareStatus.IS_FROM_MYVAULT;
                }
                else
                {
                    // From Share With Me
                    Status = Status | ShareStatus.IS_FROM_SHAREWITHME;
                }
            }
            else if (isFromPorject)
            {
                // From Share With Me
                Status = Status | ShareStatus.IS_FROM_PROJECT;
            }

        }

        private string ApplicationFindResource(string key)
        {
            Application application = Application.Current;
            if (string.IsNullOrEmpty(key))
            {
                return string.Empty;
            }
            try
            {
                string ResourceString = application.FindResource(key).ToString();
                return ResourceString;
            }
            catch (Exception)
            {
                return string.Empty;
            }
        }

        public void AddRightsItem(IList<string> rights, ref ObservableCollection<RightsItem> rightsItems, bool isAddValidity = true)
        {
            rightsItems.Add(new RightsItem(@"/CustomControls;component/resources/icons/icon_rights_view.png", ApplicationFindResource("SelectRights_View")));
            if (rights != null && rights.Count != 0)
            {
                //In order to keep the rihts display order use the method below traversal list manually instead of 
                //using foreach loop.
                if (rights.Contains("Edit"))
                {
                    rightsItems.Add(new RightsItem(@"/CustomControls;component/resources/icons/icon_rights_edit.png", ApplicationFindResource("SelectRights_Edit")));
                }
                if (rights.Contains("Print"))
                {
                    rightsItems.Add(new RightsItem(@"/CustomControls;component/resources/icons/icon_rights_print.png", ApplicationFindResource("SelectRights_Print")));
                }
                if (rights.Contains("Share"))
                {
                    rightsItems.Add(new RightsItem(@"/CustomControls;component/resources/icons/icon_rights_share.png", ApplicationFindResource("SelectRights_Share")));
                }
                if (rights.Contains("SaveAs"))
                {
                    rightsItems.Add(new RightsItem(@"/CustomControls;component/resources/icons/icon_rights_save_as.png", ApplicationFindResource("SelectRights_SaveAs")));
                }
                // Fix bug 54210
                if (rights.Contains("Decrypt"))
                {
                    rightsItems.Add(new RightsItem(@"/CustomControls;component/resources/icons/icon_rights_extract.png", ApplicationFindResource("SelectRights_Extract")));
                }
                if (rights.Contains("Watermark"))
                {
                    rightsItems.Add(new RightsItem(@"/CustomControls;component/resources/icons/icon_rights_watermark.png", ApplicationFindResource("SelectRights_Watermark")));
                }
            }
            if (isAddValidity)
            {
                rightsItems.Add(new RightsItem(@"/CustomControls;component/resources/icons/icon_rights_validity.png", ApplicationFindResource("SelectRights_Validity")));
            }
        }


    }
}
