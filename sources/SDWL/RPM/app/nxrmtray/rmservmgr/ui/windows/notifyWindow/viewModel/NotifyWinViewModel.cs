using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Linq;
using System.Runtime.CompilerServices;
using System.Text;
using System.Threading.Tasks;

namespace ServiceManager.rmservmgr.ui.windows.notifyWindow.viewModel
{
    class NotifyWinViewModel : INotifyPropertyChanged
    {
        #region DataBinding
        private string application;
        private string target;
        private string message;
        private bool result;
        private int fileStatus;

        public string Application { get => application; set { application = value; OnPropertyChanged(); } }
        public string Target { get => target; set { target = value; OnPropertyChanged(); } }
        public string Message { get => message; set { message = value; OnPropertyChanged(); } }
        public bool Result { get => result; set { result = value; OnPropertyChanged(); } }
        public int FileStatus { get => fileStatus; set { fileStatus = value; OnPropertyChanged(); } }

        public event PropertyChangedEventHandler PropertyChanged;
        protected void OnPropertyChanged([CallerMemberName] string propertyName = null)
        {
            PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(propertyName));
        }
        #endregion

    }
}
