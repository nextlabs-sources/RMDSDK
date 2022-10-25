using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Linq;
using System.Text;

namespace CustomControls.components.ValiditySpecify.model
{
    public class ValiditySpecifyConfig : INotifyPropertyChanged
    {
        private string validityDateValue;
        private string validityCountDaysValue;
        private ExpiryMode expiryMode = ExpiryMode.NEVER_EXPIRE;

        public ValiditySpecifyConfig(ValiditySpecify host)
        {
            validityDateValue = host.TryFindResource("ValidityCom_Never_Description0").ToString();
        }

        public ExpiryMode ExpiryMode
        {
            get
            {
                return expiryMode;
            }
            set
            {
                expiryMode = value;
                OnPropertyChanged("ExpiryMode");
            }
        }

        public string ValidityCountDaysValue
        {
            get
            {
                return validityCountDaysValue;
            }
            set
            {
                validityCountDaysValue = value;
                OnPropertyChanged("ValidityCountDaysValue");
            }
        }

        public string ValidityDateValue
        {
            get
            {
                return validityDateValue;
            }
            set
            {
                validityDateValue = value;
                OnPropertyChanged("ValidityDateValue");
            }
        }



        public event PropertyChangedEventHandler PropertyChanged;

        protected void OnPropertyChanged(string propertyName)
        {
            PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(propertyName));
        }
    }

}
