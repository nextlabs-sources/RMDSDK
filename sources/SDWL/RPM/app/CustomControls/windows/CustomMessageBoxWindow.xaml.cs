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
    /// Interaction logic for CustomMessageBoxWindow.xaml
    /// </summary>
    public partial class CustomMessageBoxWindow : Window
    {
        public enum CustomMessageBoxResult
        {
            None = 0,
            Positive = 1,
            Neutral = 2,
            Negative = 3
        }

        public enum CustomMessageBoxIcon
        {
            None = 0,
            Error = 1,
            Question = 2,
            Warning = 3
        }
        #region Filed
        public string MessageBoxTitle { get; set; }

        public string MessageSubjectText { get; set; }

        public string MessageBoxText { get; set; }

        public string ImagePath { get; set; }

        public CustomMessageBoxResult Result { get; set; }
        #endregion

        public CustomMessageBoxWindow()
        {
            this.Resources.MergedDictionaries.Add(SharedDictionaryManager.UnifiedBtnStyle);

            InitializeComponent();

            this.DataContext = this;

            Result = CustomMessageBoxResult.None;
        }

        /// <summary>
        /// 
        /// </summary>
        /// <param name="title"></param>
        /// <param name="subjectText"></param>
        /// <param name="details"></param>
        /// <param name="image"></param>
        /// <param name="positiveBtnContent"></param>
        /// <param name="neutralBtnContent"></param>
        /// <param name="negativeBtnContent"></param>
        /// <returns></returns>
        public static CustomMessageBoxResult Show(string title,
            string subjectText,
            string details,
            CustomMessageBoxIcon image,
            string positiveBtnContent, // At least one button
            string negativeBtnContent = null, // Should pass this if need two buttons
            string neutralBtnContent = null, // Should pass this if need three buttons
            int fontSize = 14 // default
           )
        {
            CustomMessageBoxWindow window = new CustomMessageBoxWindow();

            //window.Owner = Application.Current.MainWindow;
            window.Topmost = true;

            window.MessageBoxTitle = title;
            window.MessageSubjectText = subjectText;
            window.MessageBoxText = details;

            window.Tb_Details.FontSize = fontSize;

            // Set box image
            switch (image)
            {
                case CustomMessageBoxIcon.Question:
                    window.ImagePath = @"/CustomControls;component/resources/icons/red_warning.png";
                    break;
                case CustomMessageBoxIcon.Error:
                case CustomMessageBoxIcon.Warning:
                    window.ImagePath = @"/CustomControls;component/resources/icons/red_warning.png";
                    break;
                case CustomMessageBoxIcon.None:
                    window.ImagePath = @"/CustomControls;component/resources/icons/red_warning.png";
                    window.img_Icon.Visibility = Visibility.Collapsed;
                    break;
            }

            // Need Positive button
            if (positiveBtnContent != null)
            {
                window.Positive_Btn.Visibility = Visibility.Visible;
                window.Positive_Btn.Content = positiveBtnContent;
            }

            // Need negative button
            if (negativeBtnContent != null)
            {
                window.Negative_Btn.Visibility = Visibility.Visible;
                window.Negative_Btn.Content = negativeBtnContent;
            }

            // Need Neutral button
            if (neutralBtnContent != null)
            {
                window.Neutral_Btn.Visibility = Visibility.Visible;
                window.Neutral_Btn.Content = neutralBtnContent;
            }


            window.ShowDialog();
            return window.Result;
        }

        private bool IsCloseByClickX = true;
        private void Positive_Btn_Click(object sender, RoutedEventArgs e)
        {
            Result = CustomMessageBoxResult.Positive;
            IsCloseByClickX = false;
            this.Close();
        }

        private void Neutral_Btn_Click(object sender, RoutedEventArgs e)
        {
            Result = CustomMessageBoxResult.Neutral;
            IsCloseByClickX = false;
            this.Close();
        }

        private void Negative_Btn_Click(object sender, RoutedEventArgs e)
        {
            Result = CustomMessageBoxResult.Negative;
            IsCloseByClickX = false;
            this.Close();
        }

        private void Window_Closed(object sender, EventArgs e)
        {
            // omit the cody by osmond, when other buttons pressed it will call this.close()
            // and the later will triger Windows_Closed() being called
            if (IsCloseByClickX)
            {
                Result = CustomMessageBoxResult.None;
            }
        }

        //public delegate void CloseClickDelegate();
        //public delegate void DeleteClickDelegate();
        //public delegate void CancelClickDelegate();

        //private CloseClickDelegate close;

        //private DeleteClickDelegate delete;

        //private CancelClickDelegate cancel;

        //private void Delete_Click(object sender, RoutedEventArgs e)
        //{
        //    delete?.Invoke();
        //}

        //private void Cancel_Click(object sender, RoutedEventArgs e)
        //{
        //    cancel?.Invoke();
        //}

        //private void Close_Click(object sender, RoutedEventArgs e)
        //{
        //    close?.Invoke();



        public class CustomMessageBoxButton
        {
            public static string BTN_OK = "OK";
            public static string BTN_YES = "Yes";
            public static string BTN_NO = "No";
            //public const string BTN_OK = "OK";
            public static string BTN_DELETE = "Delete";
            public static string BTN_CANCEL = "Cancel";
            public static string BTN_CLOSE = "Close";

            public static string BTN_OVERWRITE = "Overwrite";
            public static string BTN_DISCARD = "Discard";

            // can also extend other button here ....
        }
    }
}
