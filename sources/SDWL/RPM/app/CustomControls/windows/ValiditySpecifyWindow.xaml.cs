using CustomControls.common.sharedResource;
using CustomControls.components.ValiditySpecify.model;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Shapes;

namespace CustomControls.windows
{
    /// <summary>
    /// Interaction logic for ValiditySpecifyWindow.xaml
    /// </summary>
    public partial class ValiditySpecifyWindow : Window
    {
        IExpiry expiry = new NeverExpireImpl();
        string validityContent = "";

        public ValiditySpecifyWindow(IExpiry expiry)
        {
            this.Resources.MergedDictionaries.Add(SharedDictionaryManager.StringResource);
            this.Resources.MergedDictionaries.Add(SharedDictionaryManager.UnifiedBtnStyle);

            InitializeComponent();

            this.ValidityComponent.Expiry = expiry;
        }

        public event EventHandler<NewValidationEventArgs> ValidationUpdated;

        private void Button_Cancel(object sender, RoutedEventArgs e)
        {
            this.Close();
        }

        private void Button_Select(object sender, RoutedEventArgs e)
        {
            ValidationUpdated?.Invoke(this, new NewValidationEventArgs(expiry, validityContent));
            this.Close();
        }

        private void ValidityComponent_ExpiryValueChanged(object sender, RoutedPropertyChangedEventArgs<ExpiryValueChangedEventArgs> e)
        {
            ExpiryValueChangedEventArgs value = e.NewValue;
            expiry = value.Expiry;
            validityContent = value.ExpiryDateValue;
        }
    }
    public class NewValidationEventArgs : EventArgs
    {
        private IExpiry expiry;
        private string validityContent;

        public NewValidationEventArgs(IExpiry expiry, string validityContent)
        {
            this.expiry = expiry;
            this.validityContent = validityContent;
        }

        public IExpiry Expiry
        {
            get { return expiry; }
            set { expiry = value; }
        }
        public string ValidityContent
        {
            get { return validityContent; }
            set { validityContent = value; }
        }

    }
}
