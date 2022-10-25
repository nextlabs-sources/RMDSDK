using CustomControls.pages.DigitalRights.model;
using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.ComponentModel;
using System.Linq;
using System.Text;
using System.Windows;
using System.Windows.Media.Imaging;

namespace CustomControls.components.RightsDisplay.model
{
    ///  <summary>
    /// ViewModel for RightsStackPanle.xaml 
    /// </summary>
    public class RightsStPanViewModel : INotifyPropertyChanged
    {
        private RightsStackPanle host;
        private HashSet<FileRights> fileRights = new HashSet<FileRights>();
        private ObservableCollection<RightsItem> rightsList = new ObservableCollection<RightsItem>();
        private int rightsColumn = 8;
        private string waterLabel;
        private string validityLabel;
        private string watermarkValue;
        private string validityValue;
        private Visibility waterPanlVisibility;
        private Visibility validityPanlVisibility;

        public RightsStPanViewModel(RightsStackPanle host)
        {
            this.host = host;
            waterLabel = this.host.TryFindResource("Label_WaterMark").ToString();
            validityLabel = this.host.TryFindResource("Label_Validity").ToString();
        }

        /// <summary>
        /// User can use 'FileRights' type set display rights list
        /// </summary>
        public HashSet<FileRights> FileRights { get=> fileRights; set { fileRights = value; SetRightsItemList(value); } }
        private void SetRightsItemList(HashSet<FileRights> fileRights)
        {
            ObservableCollection<RightsItem> rightsItems = new ObservableCollection<RightsItem>();
            foreach (var item in fileRights)
            {
                switch (item)
                {
                    case pages.DigitalRights.model.FileRights.RIGHT_VIEW:
                        rightsItems.Add(new RightsItem(new BitmapImage(new Uri("/CustomControls;component/resources/icons/icon_rights_view.png", UriKind.Relative)),
                            host.TryFindResource("RightsItem_View").ToString()));
                        break;
                    case pages.DigitalRights.model.FileRights.RIGHT_EDIT:
                        rightsItems.Add(new RightsItem(new BitmapImage(new Uri("/CustomControls;component/resources/icons/icon_rights_edit.png", UriKind.Relative)),
                           host.TryFindResource("RightsItem_Edit").ToString()));
                        break;
                    case pages.DigitalRights.model.FileRights.RIGHT_PRINT:
                        rightsItems.Add(new RightsItem(new BitmapImage(new Uri("/CustomControls;component/resources/icons/icon_rights_print.png", UriKind.Relative)),
                           host.TryFindResource("RightsItem_Print").ToString()));
                        break;
                    case pages.DigitalRights.model.FileRights.RIGHT_CLIPBOARD:
                        break;
                    case pages.DigitalRights.model.FileRights.RIGHT_SAVEAS:
                        rightsItems.Add(new RightsItem(new BitmapImage(new Uri("/CustomControls;component/resources/icons/icon_rights_save_as.png", UriKind.Relative)),
                          host.TryFindResource("RightsItem_SaveAs").ToString()));
                        break;
                    case pages.DigitalRights.model.FileRights.RIGHT_DECRYPT:
                        rightsItems.Add(new RightsItem(new BitmapImage(new Uri("/CustomControls;component/resources/icons/icon_rights_extract.png", UriKind.Relative)),
                           host.TryFindResource("RightsItem_Extract").ToString()));
                        break;
                    case pages.DigitalRights.model.FileRights.RIGHT_SCREENCAPTURE:
                        break;
                    case pages.DigitalRights.model.FileRights.RIGHT_SEND:
                        break;
                    case pages.DigitalRights.model.FileRights.RIGHT_CLASSIFY:
                        break;
                    case pages.DigitalRights.model.FileRights.RIGHT_SHARE:
                        rightsItems.Add(new RightsItem(new BitmapImage(new Uri("/CustomControls;component/resources/icons/icon_rights_share.png", UriKind.Relative)),
                           host.TryFindResource("RightsItem_Share").ToString()));
                        break;
                    case pages.DigitalRights.model.FileRights.RIGHT_DOWNLOAD:
                        break;
                    case pages.DigitalRights.model.FileRights.RIGHT_VALIDITY:
                        rightsItems.Add(new RightsItem(new BitmapImage(new Uri("/CustomControls;component/resources/icons/icon_rights_validity.png", UriKind.Relative)),
                           host.TryFindResource("RightsItem_Validity").ToString()));
                        break;
                    case pages.DigitalRights.model.FileRights.RIGHT_WATERMARK:
                        rightsItems.Add(new RightsItem(new BitmapImage(new Uri("/CustomControls;component/resources/icons/icon_rights_watermark.png", UriKind.Relative)),
                           host.TryFindResource("RightsItem_Watermark").ToString()));
                        break;
                }
            }
            RightsList = rightsItems;
        }

        /// <summary>
        /// User can use 'RightsItem' type set display rights list
        /// </summary>
        public ObservableCollection<RightsItem> RightsList { get => rightsList; set { rightsList = value; SetRightsColumn(value.Count); OnPropertyChanged("RightsList");} }
        private void SetRightsColumn(int colum)
        {
            RightsColumn = colum;
        }
        /// <summary>
        /// Icon column, when set 'RightsList' property will set this. 
        /// User also can set this property, but must be set after set 'FileRights' or 'RightsList'.
        /// </summary>
        public int RightsColumn { get => rightsColumn; set { rightsColumn = value; OnPropertyChanged("RightsColumn"); } }
        /// <summary>
        /// WaterMark label, defult value is "Watermark:".
        /// </summary>
        public string WaterLabel { get => waterLabel; set { waterLabel = value; OnPropertyChanged("WaterLabel"); } }
        /// <summary>
        /// Validity Label, defult value is "Validity:".
        /// </summary>
        public string ValidityLabel { get => validityLabel; set { validityLabel = value; OnPropertyChanged("ValidityLabel"); } }
        /// <summary>
        /// Wartermark value
        /// </summary>
        public string WatermarkValue { get => watermarkValue; set { watermarkValue = value; OnPropertyChanged("WatermarkValue"); } }
        /// <summary>
        /// Expiry date value
        /// </summary>
        public string ValidityValue { get => validityValue; set { validityValue = value; OnPropertyChanged("ValidityValue"); } }
        /// <summary>
        /// Watermark stackPanel visibility
        /// </summary>
        public Visibility WaterPanlVisibility { get => waterPanlVisibility; set { waterPanlVisibility = value; OnPropertyChanged("WaterPanlVisibility"); } }
        /// <summary>
        /// Expiry date stackPanel visibility
        /// </summary>
        public Visibility ValidityPanlVisibility { get => validityPanlVisibility; set { validityPanlVisibility = value; OnPropertyChanged("ValidityPanlVisibility"); } }

        

        public event PropertyChangedEventHandler PropertyChanged;
        protected void OnPropertyChanged(string propertyName)
        {
            PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(propertyName));
        }

    }
}
