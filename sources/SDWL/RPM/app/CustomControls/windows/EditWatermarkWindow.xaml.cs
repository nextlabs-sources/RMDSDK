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
using System.Windows.Shapes;

namespace CustomControls.windows
{
    /// <summary>
    /// Interaction logic for EditWatermarkWindow.xaml
    /// </summary>
    public partial class EditWatermarkWindow : Window
    {
        private bool isValid;
        private string wartermark;

        public EditWatermarkWindow() : this("$(User)$(Date)$(Break)$(Time)")
        {
        }
        public EditWatermarkWindow(string initValue)
        {
            this.Resources.MergedDictionaries.Add(SharedDictionaryManager.StringResource);
            this.Resources.MergedDictionaries.Add(SharedDictionaryManager.UnifiedBtnStyle);

            InitializeComponent();
            edit.WarterMark=initValue;
        }

        public event EventHandler<WatermarkArgs> WatermarkHandler;

        private void Btn_Cancel_Click(object sender, RoutedEventArgs e)
        {
            this.Close();
        }

        private void Btn_Select_Click(object sender, RoutedEventArgs e)
        {
            //Invoke delegate event.
            WatermarkHandler?.Invoke(this, new WatermarkArgs(wartermark));
            //Close window.
            this.Close();
        }

        private void Edit_WarterMarkChanged(object sender, RoutedPropertyChangedEventArgs<components.WarterMarkChangedEventArgs> e)
        {
            isValid = e.NewValue.IsValid;
            if (isValid)
            { SelectBtn.IsEnabled = true; }
            else
            { SelectBtn.IsEnabled = false; return; }

            wartermark = e.NewValue.WarterMarkValue;
        }
    }
    public class WatermarkArgs : EventArgs
    {
        private string watermarkvalue;
        public WatermarkArgs(string value)
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
