using CustomControls.officeUserControl;
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

namespace WinFormControlLibrary
{
    public partial class FrmFileInfo : Form
    {
        private FileInfoEx fileInfoEx;

        private FileRightsInfoDataModel frmDataModel;
        internal FileRightsInfoDataModel DataModel { get => frmDataModel; }

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

        /// <summary>
        /// When user click modify button, will trigger this event
        /// </summary>
        public event EventHandler ModifyBtnEvent;

        public FrmFileInfo(FileRightsInfoDataModel dataModel)
        {
            InitializeComponent();

            this.frmDataModel = dataModel;

            InitFileInfoEx();
            this.elementHost.Child = fileInfoEx;
            this.Controls.Add(elementHost);
        }
    
        private void InitFileInfoEx()
        {
            fileInfoEx = new FileInfoEx();

            // Create bindings.
            CommandBinding binding;
            binding = new CommandBinding(FInfoEx_DataCommands.Modify);
            binding.Executed += PositiveCommand;
            fileInfoEx.CommandBindings.Add(binding);

            binding = new CommandBinding(FInfoEx_DataCommands.Cancel);
            binding.Executed += CancelCommand;
            fileInfoEx.CommandBindings.Add(binding);

            // init ViewModel
            fileInfoEx.ViewModel = new FileInfoExViewModel(fileInfoEx)
            {
                Icon = DataConvertHelp.GDIToWpfBitmap(DataModel.FileIcon),
                FilePath = DataModel.FilePath,

                TagViewMaxWidth=200,
                CentralTag = DataModel.FileTags,

                ModifyBtnVisible =DataModel.IsModifyBtnVisible? System.Windows.Visibility.Visible: System.Windows.Visibility.Collapsed
            };

            if (DataModel.FileTags.Count == 0)
            {
                fileInfoEx.ViewModel.CentralSpVisible = System.Windows.Visibility.Collapsed;
            }

            if (string.IsNullOrEmpty(DataModel.Wartemark))
            {
                fileInfoEx.ViewModel.RightsDisplayViewModel.WaterPanlVisibility = System.Windows.Visibility.Hidden;
            }
            else
            {
                fileInfoEx.ViewModel.RightsDisplayViewModel.WaterPanlVisibility = System.Windows.Visibility.Visible;
                fileInfoEx.ViewModel.RightsDisplayViewModel.WatermarkValue = DataModel.Wartemark;
                DataModel.Filerights.Add(Rights.RIGHT_WATERMARK);
            }

            fileInfoEx.ViewModel.RightsDisplayViewModel.RightsList = DataConvertHelp.Rights2RightsDisplayModel(DataModel.Filerights);
            fileInfoEx.ViewModel.RightsDisplayViewModel.RightsColumn = 4;

            fileInfoEx.ViewModel.RightsDisplayViewModel.ValidityPanlVisibility = System.Windows.Visibility.Hidden;
        }

        private void PositiveCommand(object sender, ExecutedRoutedEventArgs e)
        {
            ModifyBtnEvent?.Invoke(sender, e);
            this.Close();
        }
        private void CancelCommand(object sender, ExecutedRoutedEventArgs e)
        {
            this.Close();
        }

    }
}
