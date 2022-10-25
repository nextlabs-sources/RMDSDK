using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Drawing.Imaging;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;
using System.Windows.Input;
using System.Windows.Media.Imaging;
using CustomControls.officeUserControl;
using CustomControls.components.CentralPolicy.model;
using CustomControls.components.DigitalRights.model;

namespace WinFormControlLibrary
{
    public partial class FrmRightsSelect : Form
    {
        private FileRightsSelectEx rightsSelectEx;

        private RightsSelectDataModel frmDataModel;
        public RightsSelectDataModel DataModel { get => frmDataModel; }

        private string dlgTitle = "NextLabs Rights Management";
        public string DlgTitle
        {
            set
            {
                dlgTitle = value;
                this.Text = dlgTitle;
            }
            get
            {
                return dlgTitle;
            }
        }

        private bool isValidTags { get; set; }

        /// <summary>
        /// When user click positive button, will trigger this event
        /// </summary>
        public event EventHandler PositiveBtnEvent;
        /// <summary>
        /// When user click skip button, will trigger this event
        /// </summary>
        public event EventHandler SkipBtnEvent;
        /// <summary>
        /// When user click cancel button, will trigger this event
        /// </summary>
        public event EventHandler CancelBtnEvent;


        public FrmRightsSelect(RightsSelectDataModel dataModel)
        {
            InitializeComponent();
            this.ControlBox = false;

            frmDataModel = dataModel;

            // build data by dataModel
            InitFileRightsSelectEx();
            elementHost.Child = rightsSelectEx;
            this.Controls.Add(elementHost);
        }

        private void InitFileRightsSelectEx()
        {
            rightsSelectEx = new FileRightsSelectEx();
            // Create bindings.
            CommandBinding binding;
            binding = new CommandBinding(SDR_DataCommands.ChangeWaterMark);
            binding.Executed += ChangeWarterMarkCommand;
            rightsSelectEx.CommandBindings.Add(binding);

            binding = new CommandBinding(SDR_DataCommands.ChangeExpiry);
            binding.Executed += ChangeExpiryCommand;
            rightsSelectEx.CommandBindings.Add(binding);

            binding = new CommandBinding(FRSEx_DataCommands.Skip);
            binding.Executed += SkipCommand_Executed;
            rightsSelectEx.CommandBindings.Add(binding);

            binding = new CommandBinding(FRSEx_DataCommands.Positive);
            binding.Executed += PositiveCommand;
            rightsSelectEx.CommandBindings.Add(binding);

            binding = new CommandBinding(FRSEx_DataCommands.Cancel);
            binding.Executed += CancelCommand_Executed;
            binding.CanExecute += CancelCommand_CanExecute;
            rightsSelectEx.CommandBindings.Add(binding);

            Bitmap bitmap = Properties.Resources.red_warning;
            // Set ViewModel
            rightsSelectEx.ViewMode = new FileRightsSelectExDataMode(rightsSelectEx)
            {
                Icon = DataConvertHelp.GDIToWpfBitmap(DataModel.FileIcon),
                FilePath = DataModel.FilePath,
                AdhocDesTextAlign = System.Windows.TextAlignment.Left,
                CpTextAlign = System.Windows.TextAlignment.Left,
                AdhocRadioIsEnable = DataModel.AdhocRadioIsEnable,
                CentralRadioIsEnable = DataModel.CentralRadioIsEnable,
                ProtectType = DataModel.AdhocRadioDefultChecked ? ProtectType.Adhoc : ProtectType.CentralPolicy,

                CpWarnDesVisible = DataModel.IsWarningVisible ? System.Windows.Visibility.Visible : System.Windows.Visibility.Collapsed,
                InfoTextVisible = DataModel.IsInfoTextVisible ? System.Windows.Visibility.Visible : System.Windows.Visibility.Hidden,
                SkipBtnVisible = DataModel.IsSkipBtnVisible ? System.Windows.Visibility.Visible : System.Windows.Visibility.Hidden,
                PositiveBtnIsEnable=DataModel.IsPositiveBtnIsEnable,
                PositiveBtnContent = DataModel.PositiveBtnContent,
                CancelBtnContent = DataModel.CancelBtnContent
            };
            // set adhoc page data
            rightsSelectEx.ViewMode.AdhocPage_ViewModel.Watermarkvalue = DataModel.Watermark;
            rightsSelectEx.ViewMode.AdhocPage_ViewModel.WaterMkTbMaxWidth = 200;
            rightsSelectEx.ViewMode.AdhocPage_ViewModel.ExpireDateValue = DataModel.ExpireDate;
            rightsSelectEx.ViewMode.AdhocPage_ViewModel.ExpireDateTbMaxWidth = 200;
            rightsSelectEx.ViewMode.AdhocPage_ViewModel.Expiry = DataConvertHelp.Expiration2ValiditySpecifyModel(DataModel.Expiry);
            rightsSelectEx.ViewMode.AdhocPage_ViewModel.Rights = DataConvertHelp.Rights2DigitalRights(DataModel.SelectedRights);

            //set central page data
            rightsSelectEx.ViewMode.CtP_Classifications = DataConvertHelp.Classifications2CentralPolicyModel(DataModel.Classifications);
            rightsSelectEx.ViewMode.OnClassificationChanged += (ss, ee) =>
            {
                Console.WriteLine("Invoke OnClassificationChanged");
                bool isValid = ee.NewValue.IsValid;
                if (isValid)
                {
                    DataModel.SelectedTags = ee.NewValue.KeyValues;
                }
                isValidTags = isValid;
            };
        }
        
        private void ChangeWarterMarkCommand(object sender, ExecutedRoutedEventArgs e)
        {
            try
            {
                FrmWaterMark frmWaterMark = new FrmWaterMark(rightsSelectEx.ViewMode.AdhocPage_ViewModel.Watermarkvalue, DlgTitle);
                frmWaterMark.WaterMarkChangedHandler += (ss, ee) =>
                {
                    rightsSelectEx.ViewMode.AdhocPage_ViewModel.Watermarkvalue = ee.Watermarkvalue;
                };
                frmWaterMark.ShowDialog();
            }
            catch (Exception msg)
            {
                Console.WriteLine("Error in EditWatermarkWindow:", msg);
            }
        }

        private void ChangeExpiryCommand(object sender, ExecutedRoutedEventArgs e)
        {
            try
            {
                FrmExpiry frmExpiry = new FrmExpiry(rightsSelectEx.ViewMode.AdhocPage_ViewModel.Expiry, DlgTitle);
                frmExpiry.ExpiryChangedHandler += (ss, ee) =>
                {
                    rightsSelectEx.ViewMode.AdhocPage_ViewModel.Expiry = ee.Expiry;
                    rightsSelectEx.ViewMode.AdhocPage_ViewModel.ExpireDateValue = ee.ExpiryDate;
                };
                frmExpiry.ShowDialog();
            }
            catch (Exception msg)
            {
                Console.WriteLine("Error in EditWatermarkWindow:", msg);
            }
        }

        private void PositiveCommand(object sender, ExecutedRoutedEventArgs e)
        {
            // update dataModel
            DataModel.AdhocRadioDefultChecked = rightsSelectEx.ViewMode.ProtectType == ProtectType.Adhoc ? true : false;

            if (DataModel.AdhocRadioDefultChecked)
            {
                DataModel.Watermark = rightsSelectEx.ViewMode.AdhocPage_ViewModel.Watermarkvalue;
                DataModel.ExpireDate = rightsSelectEx.ViewMode.AdhocPage_ViewModel.ExpireDateValue;
                DataModel.Expiry = DataConvertHelp.ValiditySpecifyModel2Expiration(rightsSelectEx.ViewMode.AdhocPage_ViewModel.Expiry);
                DataModel.SelectedRights = DataConvertHelp.DigitalRights2Rights(rightsSelectEx.ViewMode.AdhocPage_ViewModel.Rights);
            }
            else
            {
                DataModel.IsValidTags = isValidTags;
            }

            PositiveBtnEvent?.Invoke(sender, e);
        }
        public void OpenProgress()
        {
            rightsSelectEx.ViewMode.ProgressVisible = System.Windows.Visibility.Visible;
        }
        public void CloseProgress()
        {
            rightsSelectEx.ViewMode.ProgressVisible = System.Windows.Visibility.Collapsed;
        }

        private void SkipCommand_Executed(object sender, ExecutedRoutedEventArgs e)
        {
            SkipBtnEvent?.Invoke(sender, e);
        }
        private void CancelCommand_Executed(object sender, ExecutedRoutedEventArgs e)
        {
            CancelBtnEvent?.Invoke(sender, e);
        }
        private void CancelCommand_CanExecute(object sender, CanExecuteRoutedEventArgs e)
        {
            e.CanExecute = true;
        }

    }
 
}
