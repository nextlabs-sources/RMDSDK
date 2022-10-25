using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;
using System.Windows.Input;
using CustomControls.components.ValiditySpecify.model;
using CustomControls.officeUserControl;

namespace WinFormControlLibrary
{
    public partial class FrmExpiry : Form
    {
        public string DlgTitle { get; set; }
        private ValiditySpecifyEx validitySpecifyEx;
        private IExpiry expiry = new NeverExpireImpl();
        private string expiryDate;

        public event EventHandler<ExpiryChangedEventArgs> ExpiryChangedHandler;

        public FrmExpiry(IExpiry expiry, string title)
        {
            DlgTitle = title;
            InitializeComponent();

            InitValiditySpecifyEx(expiry);
            this.elementHost.Child = validitySpecifyEx;
            this.Controls.Add(elementHost);
        }

        private void InitValiditySpecifyEx(IExpiry value)
        {
            validitySpecifyEx = new ValiditySpecifyEx();

            // Create bindings.
            CommandBinding binding;
            binding = new CommandBinding(VSEx_DataCommands.Positive);
            binding.Executed += PositiveCommand;
            validitySpecifyEx.CommandBindings.Add(binding);

            binding = new CommandBinding(VSEx_DataCommands.Cancel);
            binding.Executed += CancelCommand;
            validitySpecifyEx.CommandBindings.Add(binding);

            // init ViewModel
            validitySpecifyEx.ViewModel = new ValiditySpecifyExDataModel()
            {
                Expiry = value
            };

            validitySpecifyEx.ViewModel.OnExpiryValueChanged += (ss, ee) =>
            {
                expiry = ee.NewValue.Expiry;
                expiryDate =ee.NewValue.ExpiryDateValue;
            };
        }

        private void PositiveCommand(object sender, ExecutedRoutedEventArgs e)
        {
            ExpiryChangedHandler?.Invoke(this, new ExpiryChangedEventArgs(expiry, expiryDate));
            this.Close();
        }
        private void CancelCommand(object sender, ExecutedRoutedEventArgs e)
        {
            this.Close();
        }

    }

    public class ExpiryChangedEventArgs : EventArgs
    {
        private IExpiry expiry;
        private string expiryDate;

        public ExpiryChangedEventArgs(IExpiry expiry, string date)
        {
            this.expiry = expiry;
            this.expiryDate = date;
        }

        public IExpiry Expiry
        {
            get { return expiry; }
            set { expiry = value; }
        }
        public string ExpiryDate
        {
            get { return expiryDate; }
            set { expiryDate = value; }
        }
    }

}
