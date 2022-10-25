using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.ComponentModel;
using System.Globalization;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Data;

namespace ServiceManager.rmservmgr.ui.windows.chooseServer.model
{
    public class ChooseServerModel : INotifyPropertyChanged
    {
        private ServiceManagerApp app = ServiceManagerApp.Singleton;

        // Property change notify
        public event PropertyChangedEventHandler PropertyChanged;
        protected void OnPropertyChanged(string propertyName)
        {
            PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(propertyName));
        }

        #region url list
        private ObservableCollection<UrlDataModel> urlList = new ObservableCollection<UrlDataModel>();
        public ObservableCollection<UrlDataModel> UrlList
        {
            get => urlList;
            set => urlList = value;
        }

        // Used for do search.
        private ObservableCollection<UrlDataModel> copyUrlList = new ObservableCollection<UrlDataModel>();

        private string url = "";
        public string URL
        {
            get => url; set => url = value;
        }

        #endregion // url list

        #region server model
        private ServerModel serverModel = ServerModel.Personal;
        public ServerModel ServerModel
        {
            get
            {
                return serverModel;
            }
            set
            {
                serverModel = value;
                OnPropertyChanged("ServerModel");
            }
        }
        #endregion // server model

        public ChooseServerModel()
        {
            // Save all router to this List
            List<string> AllList = new List<string>();

            // Get router from registry
            url = app.Config.CompanyRouter;  

            if (!string.IsNullOrEmpty(url))
            {
                string[] urlArray = url.Split(new char[] { ';', ',' }, StringSplitOptions.RemoveEmptyEntries);
                for (int i = 0; i < urlArray.Length; i++)
                {
                    if (string.IsNullOrEmpty(urlArray[i].Trim()))
                    {
                        continue;
                    }

                    //Do not add,if it already exists
                    if (!AllList.Contains(urlArray[i].Trim().ToLower()))
                    {
                        AllList.Add(urlArray[i].Trim().ToLower());
                    }
                }

            }

            // Get router from DB
            List<string> list = new List<string>();
            list = app.DBProvider.GetRouterUrl(); 

            for (int i = 0; i < list.Count; i++)
            {
                // Do not add,if it already exists
                if (!AllList.Contains(list[i].Trim().ToLower()))
                {
                    AllList.Add(list[i].Trim().ToLower());
                }
            }

            // Set ui display router List from AllList
            for (int i = 0; i < AllList.Count; i++)
            {
                UrlList.Add(new UrlDataModel(i, AllList[i]));
            }

            // Set search List from UrlList
            foreach (UrlDataModel one in UrlList)
            {
                copyUrlList.Add(one);
            }
        }

        public void Serach(string text, out bool isDropDownOpen)
        {
            isDropDownOpen = false;

            UrlList.Clear();
            string searchText = text.Trim().ToLower();

            if (string.IsNullOrEmpty(searchText))
            {
                foreach (UrlDataModel one in copyUrlList)
                {
                    UrlList.Add(one);
                    isDropDownOpen = true;
                }

                return;
            }

            foreach (UrlDataModel one in copyUrlList)
            {
                if (one.listUrl.ToLower().Contains(searchText))
                {
                    UrlList.Add(one);
                    isDropDownOpen = true;
                }
            }
        }

    }

    public enum ServerModel
    {
        Personal,
        Company
    }

    #region Converter
    public class ChooseServerForTextboxConverter : IValueConverter
    {
        public object Convert(object value, Type targetType, object parameter, CultureInfo culture)
        {
            ServerModel mode = (ServerModel)value;
            switch (mode)
            {
                case ServerModel.Personal:
                    return @"False";
                case ServerModel.Company:
                    return @"True";
                default:
                    return @"False";

            }
        }
        public object ConvertBack(object value, Type targetType, object parameter, CultureInfo culture)
        {
            throw new NotImplementedException();
        }
    }
    public class ChooseServerForTextboxBackgroundConverter : IValueConverter
    {
        public object Convert(object value, Type targetType, object parameter, CultureInfo culture)
        {
            ServerModel mode = (ServerModel)value;
            switch (mode)
            {
                case ServerModel.Personal:
                    return @"#A8F2F2F2";
                case ServerModel.Company:
                    return @"White";
                default:
                    return @"White";

            }
        }
        public object ConvertBack(object value, Type targetType, object parameter, CultureInfo culture)
        {
            throw new NotImplementedException();
        }
    }
    public class ChooseServerForTextblockConverter : IValueConverter
    {
        public object Convert(object value, Type targetType, object parameter, CultureInfo culture)
        {
            ServerModel mode = (ServerModel)value;
            ServiceManagerApp app = ServiceManagerApp.Singleton;

            switch (mode)
            {
                case ServerModel.Personal:
                    return app.Config.PersonRouter;
                case ServerModel.Company:
                    return @"example:  https://skydrm.microsoft.com";
                default:
                    return app.Config.PersonRouter;
            }

        }

        public object ConvertBack(object value, Type targetType, object parameter, CultureInfo culture)
        {
            throw new NotImplementedException();
        }
    }
    public class ChooseServerForTextblockLabelConverter : IValueConverter
    {
        public object Convert(object value, Type targetType, object parameter, CultureInfo culture)
        {
            ServerModel mode = (ServerModel)value;
            switch (mode)
            {
                case ServerModel.Personal:
                    return @"URL";
                case ServerModel.Company:
                    return @"Enter URL";
                default:
                    return @"URL";

            }
        }

        public object ConvertBack(object value, Type targetType, object parameter, CultureInfo culture)
        {
            throw new NotImplementedException();
        }
    }
    public class ChooseServerForCheckboxConverter : IValueConverter
    {
        public object Convert(object value, Type targetType, object parameter, CultureInfo culture)
        {
            ServerModel mode = (ServerModel)value;
            switch (mode)
            {
                case ServerModel.Personal:
                    return @"Collapsed";
                case ServerModel.Company:
                    return @"Visible";
                default:
                    return @"Collapsed";
            }
        }

        public object ConvertBack(object value, Type targetType, object parameter, CultureInfo culture)
        {
            throw new NotImplementedException();
        }
    }

    #endregion // Converter

}
