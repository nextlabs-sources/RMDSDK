using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Linq;
using System.Runtime.CompilerServices;
using System.Text;
using System.Threading.Tasks;

namespace ServiceManager.rmservmgr.ui.windows.serviceManager.model
{
    public class Notifications : INotifyPropertyChanged
    {
        private int id;
        private string application;
        private string target;
        private string message;
        private int messageType;
        private string operation;
        private bool result;
        private int fileStatus;
        private DateTime dateTime;

        public int Id { get => id; set => id = value; }
        public string Application { get => application; set { application = value; OnPropertyChanged(); } }
        public string Target { get => target; set { target = value; OnPropertyChanged(); } }
        public string Message { get => message; set { message = value; OnPropertyChanged(); } }
        public int MessageType { get => messageType; set => messageType = value; }
        public string Operation { get => operation; set { operation = value; OnPropertyChanged(); } }
        public bool Result { get => result; set { result = value; OnPropertyChanged(); } }
        public int FileStatus { get => fileStatus; set { fileStatus = value; OnPropertyChanged(); } }
        public DateTime DateTime { get => dateTime; set { dateTime = value; OnPropertyChanged(); } }


        public event PropertyChangedEventHandler PropertyChanged;
        protected virtual void OnPropertyChanged([CallerMemberName] string propertyName = null)
        {
            PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(propertyName));
        }
    }
}
