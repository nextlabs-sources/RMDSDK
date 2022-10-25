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
using CustomControls.officeUserControl;

namespace WinFormControlLibrary
{
    public partial class FrmWaterMark : Form
    {
        public string DlgTitle { set; get; }

        private EditWatermarkEx waterMarkEx;
        private bool isValid = false;
        private string waterMark;

        public event EventHandler<WarteMarkEventArgs> WaterMarkChangedHandler;

        public FrmWaterMark(string watermark, string dlgtitle)
        {
            DlgTitle = dlgtitle;
            InitializeComponent();

            InitEditWaterMarkEx(watermark);
            this.elementHost.Child = waterMarkEx;
            this.Controls.Add(elementHost);
        }

        private void InitEditWaterMarkEx(string watermark)
        {
            waterMarkEx = new EditWatermarkEx();

            // Create bindings.
            CommandBinding binding;
            binding = new CommandBinding(EditWEx_DataCommands.Positive);
            binding.Executed += PositiveCommand;
            waterMarkEx.CommandBindings.Add(binding);

            binding = new CommandBinding(EditWEx_DataCommands.Cancel);
            binding.Executed += CancelCommand;
            waterMarkEx.CommandBindings.Add(binding);

            // init ViewModel
            waterMarkEx.ViewModel = new EditWarterMarkExDataModel()
            {
                WarterMark= watermark
            };

            waterMarkEx.ViewModel.OnWarterMarkChanged += (ss, ee) =>
            {
                Console.WriteLine("Invoke OnWarterMarkChanged");

                isValid = ee.NewValue.IsValid;
                if (isValid)
                { waterMarkEx.ViewModel.IsEnableSaveBtn = true; }
                else
                { waterMarkEx.ViewModel.IsEnableSaveBtn = false; return; }

                waterMark = ee.NewValue.WarterMarkValue;
            };
        }

        private void PositiveCommand(object sender, ExecutedRoutedEventArgs e)
        {
            WaterMarkChangedHandler?.Invoke(this, new WarteMarkEventArgs(waterMark));
            this.Close();
        }

        private void CancelCommand(object sender, ExecutedRoutedEventArgs e)
        {
            this.Close();
        }

    }

    public class WarteMarkEventArgs : EventArgs
    {
        private string watermarkvalue;
        public WarteMarkEventArgs(string value)
        {
            this.watermarkvalue = value;
        }
        public string Watermarkvalue
        {
            get { return watermarkvalue; }
            set { watermarkvalue = value; }
        }
    }

}
