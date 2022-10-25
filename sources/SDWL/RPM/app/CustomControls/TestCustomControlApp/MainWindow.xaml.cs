using CustomControls;
using CustomControls.components;
using CustomControls.components.CentralPolicy.model;
using CustomControls.components.DigitalRights.model;
using CustomControls.pages.Preference;
using CustomControls.pages.Share;
using CustomControls.windows;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;

namespace TestCustomControlApp
{
    /// <summary>
    /// Interaction logic for MainWindow.xaml
    /// </summary>
    public partial class MainWindow : Window
    {
        private FileRightsSelectPage control;
        public MainWindow()
        {
            InitializeComponent();

            InitFileFileRightsSelectUCUserControl();
        }

        #region Test FileRightsSelectUserControl

        private bool isValid = false;
        private Dictionary<string, List<string>> tags = new Dictionary<string, List<string>>();

        private void InitFileFileRightsSelectUCUserControl()
        {
            CommandBinding binding0;
            binding0 = new CommandBinding(CapD_DataCommands.Change);
            binding0.Executed += ChangeWarterMarkCommand;
            this.CommandBindings.Add(binding0);

            // Create bindings.
            CommandBinding binding;
            //binding = new CommandBinding(SDR_DataCommands.ChangeWaterMark);
            //binding.Executed += ChangeWarterMarkCommand;
            //Frs_UC.CommandBindings.Add(binding);

            //binding = new CommandBinding(SDR_DataCommands.ChangeExpiry);
            //binding.Executed += ChangeExpiryCommand;
            //Frs_UC.CommandBindings.Add(binding);

            binding = new CommandBinding(FRS_DataCommands.Positive);
            binding.Executed += PositiveCommand;
            Frs_UC.CommandBindings.Add(binding);

            binding = new CommandBinding(FRS_DataCommands.Cancel);
            binding.Executed += CancelCommand_Executed;
            binding.CanExecute += CancelCommand_CanExecute;
            Frs_UC.CommandBindings.Add(binding);

            // Set ViewModel
            Frs_UC.ViewMode = new FileRightsSelectViewMode(Frs_UC)
            {
                SavePathStpVisibility = Visibility.Visible,
                AdhocRadioIsEnable = true,
                PositiveBtnContent = "Classify"
            };
            // set adhoc page data
            Frs_UC.ViewMode.AdhocPage_ViewModel.Watermarkvalue = "test1456";
            Frs_UC.ViewMode.AdhocPage_ViewModel.WaterMkTbMaxWidth = 350;

            //set central page data
            Frs_UC.ViewMode.CtP_Classifications = GetProjectClassification();
            Frs_UC.ViewMode.OnClassificationChanged += (ss, ee) =>
            {
                Console.WriteLine("Invoke OnClassificationChanged in UserControl");
                isValid = ee.NewValue.IsValid;
                tags = ee.NewValue.KeyValues;
            };

            Frs_UC.ViewMode.ProtectType = ProtectType.Adhoc;
        }

        private List<Project> GetProjects()
        {
            List<Project> list = new List<Project>();
            for (int i = 0; i < 10; i++)
            {
                Project project = new Project()
                { Id = i, Name = "WaylonProject", CreateTime = DateTime.Now, FileCount = 10, IsOwner = true, InvitedBy = "waylon" };
                list.Add(project);
            }
            for (int i = 10; i < 20; i++)
            {
                Project project = new Project()
                { Id = i, Name = "WaylonProject2", CreateTime = DateTime.Now, FileCount = 0, IsOwner = false, InvitedBy = "waylon2" };
                list.Add(project);
            }
            return list;
        }

        /// <summary>
        /// directly listen central policy page SelectClassificationChanged routed event.
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void Window_SelectClassificationChanged(object sender, RoutedPropertyChangedEventArgs<SelectClassificationEventArgs> e)
        {
            Console.WriteLine("Invoke OnClassificationChanged in Window");
            isValid = e.NewValue.IsValid;
            tags = e.NewValue.KeyValues;
        }

        private Classification[] GetProjectClassification()
        {
            Classification[] classification = new Classification[2];
            classification[0].name = "test1";
            classification[0].isMandatory = true;
            classification[0].isMultiSelect = false;
            classification[0].labels = new Dictionary<string, bool>();
            classification[0].labels.Add("AAA1", false);
            classification[0].labels.Add("BBB1", true);

            classification[1].name = "test2";
            classification[1].isMandatory = false;
            classification[1].isMultiSelect = true;
            classification[1].labels = new Dictionary<string, bool>();
            classification[1].labels.Add("CCC2", true);
            classification[1].labels.Add("DDD2", false);

            return classification;
        }

        private void ChangeWarterMarkCommand(object sender, ExecutedRoutedEventArgs e)
        {
            try
            {
                EditWatermarkWindow editWatermarkWindow = new EditWatermarkWindow(Frs_UC.ViewMode.AdhocPage_ViewModel.Watermarkvalue);
                editWatermarkWindow.WatermarkHandler += (ss, ee) =>
                {
                    Frs_UC.ViewMode.AdhocPage_ViewModel.Watermarkvalue = ee.Watermarkvalue;
                };
                editWatermarkWindow.ShowDialog();
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
                ValiditySpecifyWindow validitySpecifyWindow = new ValiditySpecifyWindow(Frs_UC.ViewMode.AdhocPage_ViewModel.Expiry);
                validitySpecifyWindow.ValidationUpdated += (ss, ee) =>
                {
                    Frs_UC.ViewMode.AdhocPage_ViewModel.Expiry = ee.Expiry;
                    Frs_UC.ViewMode.AdhocPage_ViewModel.ExpireDateValue = ee.ValidityContent;
                };
                validitySpecifyWindow.ShowDialog();
            }
            catch (Exception msg)
            {
                Console.WriteLine("Error in EditWatermarkWindow:", msg);
            }
        }

        private void PositiveCommand(object sender, ExecutedRoutedEventArgs e)
        {
            Frs_UC.ViewMode.ProtectType = ProtectType.CentralPolicy;
        }
        private void CancelCommand_Executed(object sender, ExecutedRoutedEventArgs e)
        {
            this.Close();
        }
        private void CancelCommand_CanExecute(object sender, CanExecuteRoutedEventArgs e)
        {
            e.CanExecute = true;
        }


        #endregion

        private void Frs_UC_RaidoBtnChecked(object sender, RoutedEventArgs e)
        {

        }
    }
}
