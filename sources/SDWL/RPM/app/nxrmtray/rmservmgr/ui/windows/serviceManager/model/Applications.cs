using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Linq;
using System.Runtime.CompilerServices;
using System.Text;
using System.Threading.Tasks;

namespace ServiceManager.rmservmgr.ui.windows.serviceManager.model
{
    public class Applications : INotifyPropertyChanged
    {
        public int Id { get; set; }
        public string Application { get; set; }

        private bool isChecked;
        public bool IsChecked { get => isChecked; set { isChecked = value; OnPropertyChanged(); } }

        public event PropertyChangedEventHandler PropertyChanged;
        protected virtual void OnPropertyChanged([CallerMemberName] string propertyName = null)
        {
            PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(propertyName));
        }
    }
}
