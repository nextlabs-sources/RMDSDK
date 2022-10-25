using CustomControls.common.sharedResource;
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
using System.Windows.Navigation; 
using System.Windows.Shapes;

namespace CustomControls.pages.Share
{
    /// <summary>
    /// Interaction logic for SharePage.xaml
    /// </summary>
    public partial class SharePage : Page
    {
        private ShareViewModel viewModel;

        public SharePage(ShareViewModel viewModel)
        {
            this.Resources.MergedDictionaries.Add(SharedDictionaryManager.UnifiedBtnStyle);
            this.Resources.MergedDictionaries.Add(SharedDictionaryManager.StringResource);
            InitializeComponent();
            this.viewModel = viewModel;
            this.DataContext = viewModel;
        }

        private void EmailInputTB_TextChanged(object sender, TextChangedEventArgs e)
        {
            // mSharePageViewModel.EmailInputTB_TextChanged(sender, e);
        }

        private void EmailInput_KeyDown(object sender, KeyEventArgs e)
        {
            // mSharePageViewModel.EmailInput_KeyDown(sender, e);
        }

        private void On_GetOutlookEmail_Btn(object sender, MouseButtonEventArgs e)
        {
            //  mSharePageViewModel.On_GetOutlookEmail_Btn(sender, e);
        }

        private void DeleteEmailItem_MouseLeftButtonUp(object sender, MouseButtonEventArgs e)
        {
            // mSharePageViewModel.DeleteEmailItem_MouseLeftButtonUp(sender, e);
        }

        private void commentTB_TextChanged(object sender, TextChangedEventArgs e)
        {
            // mSharePageViewModel.commentTB_TextChanged(sender, e);
        }

        private void Button_Ok(object sender, RoutedEventArgs e)
        {
            // mSharePageViewModel.Button_Ok(sender, e);
        }

        private void Button_Cancel(object sender, RoutedEventArgs e)
        {
            //  mSharePageViewModel.Button_Cancel(sender, e);
        }
    }
}
