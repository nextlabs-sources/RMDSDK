using CustomControls.common.sharedResource;
using CustomControls.components.DigitalRights.model;
using System;
using System.Windows;
using System.Windows.Controls;

namespace CustomControls.components.DigitalRights
{
    /// <summary>
    /// Interaction logic for SelectDigitalRights.xaml
    /// </summary>
    public partial class SelectDigitalRights : UserControl
    {
        private DigitalRightsViewModel viewModel;

        public SelectDigitalRights()
        {
            this.Resources.MergedDictionaries.Add(SharedDictionaryManager.StringResource);
            this.Resources.MergedDictionaries.Add(SharedDictionaryManager.UnifiedCheckBoxStyle);
            this.Resources.MergedDictionaries.Add(SharedDictionaryManager.UnifiedBtnStyle);

            InitializeComponent();

            this.DataContext = viewModel = new DigitalRightsViewModel(this);
        }

        /// <summary>
        ///  ViewModel for SelectDigitalRights.xaml
        /// </summary>
        public DigitalRightsViewModel ViewModel { get => viewModel; set => this.DataContext = viewModel = value; }

        /// <summary>
        /// This is the callback of rights checkbox checked or unchecked.  
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void CheckBox_RightsChecked(object sender, RoutedEventArgs e)
        {
            CheckBox rightsCheckBox = sender as CheckBox;
            if (rightsCheckBox != null && rightsCheckBox.Name != null)
            {
                Console.WriteLine($"CustomControl CheckBox_RightsChecked: Name({rightsCheckBox.Name}),IsChecked({rightsCheckBox.IsChecked})");
                switch (rightsCheckBox.Name.ToString())
                {
                    case "Edit":
                        FillRights(FileRights.RIGHT_EDIT);
                        break;
                    case "Print":
                        FillRights(FileRights.RIGHT_PRINT);
                        break;
                    case "Share":
                        FillRights(FileRights.RIGHT_SHARE);
                        break;
                    case "SaveAs":
                        FillRights(FileRights.RIGHT_SAVEAS);
                        break;
                    case "Watermark":
                        viewModel.WarterMarkCheckStatus = (bool)rightsCheckBox.IsChecked ? CheckStatus.CHECKED : CheckStatus.UNCHECKED;
                        FillRights(FileRights.RIGHT_WATERMARK);
                        break;
                    case "Decrypt":
                        FillRights(FileRights.RIGHT_DECRYPT);
                        break;
                }
            }
        }
        private void FillRights(FileRights rightsItem)
        {
            if (viewModel.Rights.Contains(rightsItem))
            {
                viewModel.Rights.Remove(rightsItem);
            }
            else
            {
                viewModel.Rights.Add(rightsItem);
            }
        }

        private void OnExpanded(object sender, RoutedEventArgs e)
        {
            if (this.expander.IsExpanded)
            {
                this.Decrypt.Visibility = Visibility.Visible;
            }
            else
            {
                this.Decrypt.Visibility = Visibility.Collapsed;
            }
        }

    }
}
