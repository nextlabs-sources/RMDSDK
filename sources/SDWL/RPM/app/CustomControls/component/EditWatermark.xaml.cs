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

namespace CustomControls.components
{
    public struct WarterMarkChangedEventArgs
    {
        public bool IsValid { get; set; }
        public string WarterMarkValue { get; set; }
    }

    /// <summary>
    /// Interaction logic for EditWatermark.xaml
    /// </summary>
    public partial class EditWatermark : UserControl
    {
        #region private field
        private const string DOLLAR_USER = "$(User)";
        private const string DOLLAR_BREAK = "$(Break)";
        private const string DOLLAR_DATE = "$(Date)";
        private const string DOLLAR_TIME = "$(Time)";

        private const string PRESET_VALUE_EMAIL_ID = "Email ID";
        private const string PRESET_VALUE_DATE = "Date";
        private const string PRESET_VALUE_TIME = "Time";
        private const string PRESET_VALUE_LINE_BREAK = "Line break";

        FlowDocument document = new FlowDocument();
        // A block-level flow content element that used to add Run text and InlineUIContainer.
        private Paragraph paragraph = new Paragraph();
        // used to embed one UIContainer into Paragraph.
        private InlineUIContainer uIContainer;

        // used to record the count of "Line Break" preset value.
        private int lineBreakCount = 0;

        // Max length that allow to input
        private const int maxLen = 50;
        // Length that has input
        private int usedLen = 0;
        // Used to record the preset value.
        private List<string> presetValueList = new List<string>();

        // Judge if click the preset button that in RichTextBox.
        private bool bIsClickRemovePresetFromRichBox = false;

        // record the input box content before and after.
        private string richBoxBeforeChanged = "";
        private string richBoxAfterChanged = "";
        #endregion

        public EditWatermark()
        {
            this.Resources.MergedDictionaries.Add(SharedDictionaryManager.StringResource);
            InitializeComponent();
        }

        #region Declare and register Dependency Property

        public static readonly DependencyProperty WarterMarkProperty = DependencyProperty.Register("WarterMark", typeof(string),
            typeof(EditWatermark),
            new FrameworkPropertyMetadata(new PropertyChangedCallback(OnWarterMarkPropertyChanged)));

        /// <summary>
        /// Use for Init data
        /// </summary>
        public string WarterMark
        {
            get { return (string)GetValue(WarterMarkProperty); }
            set { SetValue(WarterMarkProperty, value); }
        }

        private static void OnWarterMarkPropertyChanged(DependencyObject sender, DependencyPropertyChangedEventArgs e)
        {
            EditWatermark editWatermark = (EditWatermark)sender;
            if (e.Property == WarterMarkProperty)
            {
                string value = (string)e.NewValue;
                editWatermark.doInit(value);
            }
        }
        #endregion

        #region Declare and register RoutedEvent

        public static readonly RoutedEvent WarterMarkChangedEvent =
           EventManager.RegisterRoutedEvent("WarterMarkChanged", RoutingStrategy.Bubble,
               typeof(RoutedPropertyChangedEventHandler<WarterMarkChangedEventArgs>), typeof(EditWatermark));

        /// <summary>
        /// When warterMark value changed, will trigger this event and transmit new value.
        /// </summary>
        public event RoutedPropertyChangedEventHandler<WarterMarkChangedEventArgs> WarterMarkChanged
        {
            add { AddHandler(WarterMarkChangedEvent, value); }
            remove { RemoveHandler(WarterMarkChangedEvent, value); }
        }

        private void OnWarterMarkChanged(WarterMarkChangedEventArgs oldValue, WarterMarkChangedEventArgs newValue)
        {
            RoutedPropertyChangedEventArgs<WarterMarkChangedEventArgs> args = new RoutedPropertyChangedEventArgs<WarterMarkChangedEventArgs>(oldValue, newValue);
            args.RoutedEvent = EditWatermark.WarterMarkChangedEvent;
            RaiseEvent(args);
        }

        /// <summary>
        /// Get classification and trigger WarterMarkChangedEvent
        /// </summary>
        private void GetWarterMarkHandler(bool isValid)
        {
            string value = GetWaterMark();
            OnWarterMarkChanged(new WarterMarkChangedEventArgs { }, 
                new WarterMarkChangedEventArgs {IsValid=isValid, WarterMarkValue=value });
        }
        #endregion

        private void doInit(string initValue)
        {
            // --- It's important to set this, otherwise you'll get blank lines
            document.LineHeight = 1;
            // Init RichTextBox
            ConvertString2PresetValue(initValue);

            // Init Preset value container(WrapStack)
            InitPresetValueContainer(initValue);

            // record before changed content.
            richBoxBeforeChanged = ConvertPresetValue2String();
        }
        private string GetWaterMark()
        {
            return ConvertPresetValue2String();
        }

        private void InitPresetValueContainer(string initValue)
        {

            if (!initValue.Contains(DOLLAR_USER))
            {
                AddPresetToWrapStack(PRESET_VALUE_EMAIL_ID);
            }

            if (!initValue.Contains(DOLLAR_DATE))
            {
                AddPresetToWrapStack(PRESET_VALUE_DATE);
            }

            if (!initValue.Contains(DOLLAR_TIME))
            {
                AddPresetToWrapStack(PRESET_VALUE_TIME);
            }
        }


        /// <summary>
        /// Converter init watermark string text content into Preset value button.
        /// </summary>
        private void ConvertString2PresetValue(string initValue)
        {
            if (string.IsNullOrEmpty(initValue))
            {
                return;
            }

            char[] array = initValue.ToCharArray();
            // record preset value begin index
            int beginIndex = -1;
            // record preset value end index
            int endIndex = -1;
            for (int i = 0; i < array.Length; i++)
            {
                if (array[i] == '$')
                {
                    beginIndex = i;
                }
                else if (array[i] == ')')
                {
                    endIndex = i;
                }

                if (beginIndex != -1 && endIndex != -1 && beginIndex < endIndex)
                {

                    TextPointer pos = GetCurrentCursor();
                    // append text before preset value
                    Run run = new Run(initValue.Substring(0, beginIndex), pos);
                    paragraph.Inlines.Add(run);
                    document.Blocks.Add(paragraph);
                    this.rtb.Document = document;

                    // judge if is preset
                    string subStr = initValue.Substring(beginIndex, endIndex - beginIndex + 1);

                    if (subStr.Equals(DOLLAR_USER))
                    {
                        CreatePreset(initValue, DOLLAR_USER);
                    }
                    else if (subStr.Equals(DOLLAR_BREAK))
                    {
                        CreatePreset(initValue, DOLLAR_BREAK);
                    }
                    else if (subStr.Equals(DOLLAR_DATE))
                    {
                        CreatePreset(initValue, DOLLAR_DATE);
                    }
                    else if (subStr.Equals(DOLLAR_TIME))
                    {
                        CreatePreset(initValue, DOLLAR_TIME);
                    }
                    else
                    {
                        // Get the current caret position.
                        TextPointer carePos = GetCurrentCursor();

                        Run r = new Run(subStr, carePos);
                        paragraph.Inlines.Add(r);
                        document.Blocks.Add(paragraph);
                        this.rtb.Document = document;
                    }

                    // quit
                    break;
                }
            }

            if (beginIndex == -1 || endIndex == -1 || beginIndex > endIndex) // have not preset
            {
                // Get the current caret position.
                TextPointer carePos = GetCurrentCursor();

                Run run = new Run(initValue, carePos);
                paragraph.Inlines.Add(run);
                document.Blocks.Add(paragraph);
                this.rtb.Document = document;

            }
            else if (beginIndex < endIndex)
            {
                if (endIndex + 1 < initValue.Length)
                {
                    // Converter the remaining by recursive
                    ConvertString2PresetValue(initValue.Substring(endIndex + 1));
                }
            }

        }

        private TextPointer GetCurrentCursor()
        {
            // Get the current caret position.
            TextPointer carePos = this.rtb.CaretPosition;
            // Set the TextPointer to the end the current document.
            carePos = carePos.DocumentEnd;
            // Specify the new caret position at the end of the current document.
            rtb.CaretPosition = carePos;

            return carePos;
        }

        private void CreatePreset(string initValue, string preset)
        {
            Button btn = new Button();
            btn.Height = 25;
            btn.Margin = new Thickness(3, 0, 0, 0);

            switch (preset)
            {
                case DOLLAR_USER:
                    btn.Content = PRESET_VALUE_EMAIL_ID;
                    btn.Tag = PRESET_VALUE_EMAIL_ID;
                    btn.Width = 90;
                    break;
                case DOLLAR_BREAK:
                    btn.Content = PRESET_VALUE_LINE_BREAK;
                    btn.Tag = PRESET_VALUE_LINE_BREAK + lineBreakCount;
                    btn.Width = 95;
                    break;
                case DOLLAR_DATE:
                    btn.Content = PRESET_VALUE_DATE;
                    btn.Tag = PRESET_VALUE_DATE;
                    btn.Width = 75;
                    break;
                case DOLLAR_TIME:
                    btn.Content = PRESET_VALUE_TIME;
                    btn.Tag = PRESET_VALUE_TIME;
                    btn.Width = 75;
                    break;
                default:
                    break;
            }

            // add click event for Remove form RichTextbox.
            btn.Click += Click_RemovePresetFromRtb;

            // Get the current caret position.
            TextPointer carePos = GetCurrentCursor();

            // Then embed the paragraph as InlineUIContainer.(System will produce a default Paragraph to use.)
            uIContainer = new InlineUIContainer(btn, carePos);
            uIContainer.BaselineAlignment = BaselineAlignment.Center;
            uIContainer.Tag = btn.Tag;

        }

        private void SetWrapPanelChildrenClick()
        {
            foreach (UIElement one in wrapPanel.Children)
            {
                Button btn = one as Button;
                btn.Click += Click_AddPreset;
            }
        }

        private void Click_AddPreset(object sender, RoutedEventArgs e)
        {

            // get cursor position and automatically insertpreset value.
            //TextPointer pos = this.rtb.CaretPosition; // or: this.rtb.Selection.Start;
            // fix bug 49532
            TextPointer pos = GetCurrentCursor();

            Button btn = sender as Button;
            // Remove btn from the WrapPanel firstly.
            this.wrapPanel.Children.Remove(btn);

            btn.Click -= Click_AddPreset;
            btn.Click += Click_RemovePresetFromRtb;
            // Then embed the paragraph as InlineUIContainer.  --- 好像系统会默认产生一个Paragraph去使用.
            uIContainer = new InlineUIContainer(btn, pos);
            uIContainer.BaselineAlignment = BaselineAlignment.Center;
            uIContainer.Tag = btn.Tag;

        }

        private void Click_addLineBreak(object sender, RoutedEventArgs e)
        {
            // Create Line Break Preset value
            Button btnLineBreak = new Button();
            lineBreakCount++;
            btnLineBreak.Width = 95;
            btnLineBreak.Height = 25;
            btnLineBreak.Content = PRESET_VALUE_LINE_BREAK;
            btnLineBreak.Tag = PRESET_VALUE_LINE_BREAK + lineBreakCount;
            btnLineBreak.Margin = new Thickness(3, 0, 0, 0);
            btnLineBreak.Click += Click_RemovePresetFromRtb;

            // get cursor position and automatically insert LineBreak preset value.
            //TextPointer pos = this.rtb.CaretPosition; // or: this.rtb.Selection.Start;
            // fix bug 49532
            TextPointer pos = GetCurrentCursor();

            uIContainer = new InlineUIContainer(btnLineBreak, pos);
            uIContainer.BaselineAlignment = BaselineAlignment.Center;
            uIContainer.Tag = PRESET_VALUE_LINE_BREAK + lineBreakCount;

        }


        private void Click_RemovePresetFromRtb(object sender, RoutedEventArgs e)
        {
            bIsClickRemovePresetFromRichBox = true;
            // Note: Remove can't be used in foreach.
            if (!(sender is Button))
            {
                return;
            }

            Button btn = sender as Button;
            string tag = btn.Tag.ToString();

            Paragraph tmpParagraph = null;
            InlineUIContainer tmpUIContainer = null;

            // Traverse and find wanted delete InlineUIContainer(Preset value) by Tag.
            BlockCollection blocks = this.rtb.Document.Blocks;
            foreach (Block oneBlock in blocks)
            {
                if (oneBlock is Paragraph)
                {
                    Paragraph tmpPg = oneBlock as Paragraph;

                    foreach (Inline one in tmpPg.Inlines)
                    {

                        if (one is InlineUIContainer)
                        {
                            InlineUIContainer tmpIU = one as InlineUIContainer;
                            if (tmpIU.Tag.Equals(tag))
                            {
                                tmpParagraph = tmpPg;
                                tmpUIContainer = tmpIU;
                                break;
                            }
                        }

                    }
                }
            }

            // Remove it.
            if (tmpParagraph != null && tmpUIContainer != null)
            {
                tmpParagraph.Inlines.Remove(tmpUIContainer);
            }

            // For "Date", "Email ID", "Time" Preset value, should add them into WrapStack again.
            if (tag.Equals("Email ID") || tag.Equals("Date") || tag.Equals("Time"))
            {
                AddPresetToWrapStack(tag);
            }
        }

        private void AddPresetToWrapStack(string tag)
        {
            Console.WriteLine("***AddPresetToWrapStack****");
            Button btn = new Button();

            if (tag.Equals("Email ID"))
            {
                btn.Width = 90;
            }
            else
            {
                btn.Width = 75;
            }

            btn.Height = 25;
            // here Content equals tag
            btn.Content = tag;
            btn.Tag = tag;
            btn.Margin = new Thickness(3, 0, 0, 0);
            this.wrapPanel.Children.Add(btn);

            // Set click event for this preset button.
            btn.Click += Click_AddPreset;
        }

        private string ConvertPresetValue2String()
        {
            string ret = "";
            // clear
            presetValueList.Clear();

            BlockCollection blocks = this.rtb.Document.Blocks;
            foreach (Block one in blocks)
            {
                if (one is Paragraph)
                {
                    ret += handleParagraph((one as Paragraph));
                }
            }
            return ret;
        }

        private string handleParagraph(Paragraph paragraph)
        {
            InlineCollection inlineCollection = paragraph.Inlines;
            StringBuilder sb = new StringBuilder();
            foreach (Inline one in inlineCollection)
            {
                if (one is Run)
                {
                    string runText = (one as Run).Text;
                    sb.Append(runText);
                }
                else if (one is Span)  // fix bug49416: user may paster string(Actually is Span) from others.
                {
                    Span s = one as Span;
                    TextRange tr = new TextRange(s.ContentStart, s.ContentEnd);
                    sb.Append(tr);

                }
                else if (one is Hyperlink)
                {
                    Span s = one as Hyperlink;
                    TextRange tr = new TextRange(s.ContentStart, s.ContentEnd);
                    sb.Append(tr);
                }
                else if (one is InlineUIContainer)
                {
                    if ((one as InlineUIContainer).Child is Button)
                    {
                        Button btn = (one as InlineUIContainer).Child as Button;

                        string btnText = btn.Content.ToString();
                        switch (btnText)
                        {
                            case PRESET_VALUE_EMAIL_ID:
                                presetValueList.Add(PRESET_VALUE_EMAIL_ID);
                                sb.Append(DOLLAR_USER);
                                break;
                            case PRESET_VALUE_DATE:
                                presetValueList.Add(PRESET_VALUE_DATE);
                                sb.Append(DOLLAR_DATE);
                                break;
                            case PRESET_VALUE_TIME:
                                presetValueList.Add(PRESET_VALUE_TIME);
                                sb.Append(DOLLAR_TIME);
                                break;
                            case PRESET_VALUE_LINE_BREAK:
                                presetValueList.Add(PRESET_VALUE_LINE_BREAK);
                                sb.Append(DOLLAR_BREAK);
                                break;
                            default:
                                break;
                        }

                    }
                }
            };

            return sb.ToString();

        }

        /// <summary>
        /// Now need to control preset values occupying character space like following:
        /// "EmailId" ---- 7 chars
        /// "Date"    ----- 7 chars
        /// "Time"    ----- 7 chars
        /// "Break Line" ---- 8 chars
        /// </summary>
        /// <returns></returns>
        private int CalculateInputLength(string inputContent)
        {
            int length = inputContent.Length;
            int nResultLength = 0;
            if (presetValueList != null && presetValueList.Count > 0)
            { // contains presetValue
                foreach (string onePreset in presetValueList)
                {

                    if (onePreset.Equals(PRESET_VALUE_EMAIL_ID))
                    { // actually occupying 7: $(user) 
                        length -= 7;
                        nResultLength += 7;
                    }
                    else if (onePreset.Equals(PRESET_VALUE_TIME))
                    { // actually occupying 7: $(Time)
                        length -= 7;
                        nResultLength += 7;
                    }
                    else if (onePreset.Equals(PRESET_VALUE_DATE))
                    { // actually occupying 7: $(Date)
                        length -= 7;
                        nResultLength += 7;
                    }
                    else if (onePreset.Equals(PRESET_VALUE_LINE_BREAK))
                    { // actually occupying 8: $(Break)
                        length -= 8;
                        nResultLength += 8;
                    }
                }
            }

            nResultLength += length;

            return nResultLength;
        }

        /// <summary>
        /// Listen the text change of RichTextBox. 
        /// </summary>
        private void rtb_TextChanged(object sender, TextChangedEventArgs e)
        {
            try
            {
                Console.WriteLine("-----rtb_TextChanged----");

                // After content
                richBoxAfterChanged = ConvertPresetValue2String();

                // Cal length
                usedLen = CalculateInputLength(richBoxAfterChanged);
                int remainingLength = maxLen - usedLen;
                TB_RemaingLength.Text = remainingLength.ToString();

                // Display the prompt info
                if (string.IsNullOrWhiteSpace(richBoxAfterChanged))
                {
                    this.Tb_PromptInfo.Visibility = Visibility.Visible;
                    this.Tb_PromptInfo.Text = this.TryFindResource("EditWatermarkCom_Tb_PromptInfo_Text1").ToString();
                    // notify
                    GetWarterMarkHandler(false);
                }
                else // Not empty
                {
                    int len = int.Parse(this.TB_RemaingLength.Text);
                    if (int.Parse(this.TB_RemaingLength.Text) < 0)
                    {
                        this.Tb_PromptInfo.Visibility = Visibility.Visible;
                        this.Tb_PromptInfo.Text = this.TryFindResource("EditWatermarkCom_Tb_PromptInfo_Text2").ToString();
                        TB_RemaingLength.Foreground = new SolidColorBrush(Colors.Red);
                        // notify
                        GetWarterMarkHandler(false);
                    }
                    else
                    {
                        this.Tb_PromptInfo.Visibility = Visibility.Collapsed;
                        TB_RemaingLength.Foreground = new SolidColorBrush(Colors.Black);
                        // notify
                        GetWarterMarkHandler(true);
                    }

                }

                // Ignore it when click remove
                if (bIsClickRemovePresetFromRichBox)
                {
                    // reset
                    bIsClickRemovePresetFromRichBox = false;
                    // reset Before content
                    richBoxBeforeChanged = richBoxAfterChanged;
                    return;
                }

                ICollection<TextChange> chages = e.Changes;
                int removeLen = -1;
                foreach (TextChange one in chages)
                {
                    removeLen = one.RemovedLength;
                }

                // delete by keyboard. should re-init wrapStack preset value if delete "Email ID", "Time", "Date" etc.
                if (removeLen > 0)
                {
                    if (richBoxAfterChanged.Length < richBoxBeforeChanged.Length)
                    {
                        if (richBoxBeforeChanged.Contains(DOLLAR_USER) &&
                            !richBoxAfterChanged.Contains(DOLLAR_USER))
                        {
                            AddPresetToWrapStack(PRESET_VALUE_EMAIL_ID);
                        }

                        if (richBoxBeforeChanged.Contains(DOLLAR_DATE) &&
                             !richBoxAfterChanged.Contains(DOLLAR_DATE))
                        {
                            AddPresetToWrapStack(PRESET_VALUE_DATE);
                        }

                        if (richBoxBeforeChanged.Contains(DOLLAR_TIME) &&
                           !richBoxAfterChanged.Contains(DOLLAR_TIME))
                        {
                            AddPresetToWrapStack(PRESET_VALUE_TIME);
                        }

                    }

                }

                // reset Before content
                richBoxBeforeChanged = richBoxAfterChanged;

                string result = ConvertPresetValue2String(); // "121334ccccEmail ID44Datetttt/Line BreakTime22"

                Console.WriteLine("-----Result----");
                Console.WriteLine(result);
            }
            catch (Exception msg)
            {
                Console.WriteLine($"Exception in rtb_TextChanged:{msg}");
            }
            

        }

        /// <summary>
        ///  Forbit the "Enter" key operation. 
        /// </summary>
        private void rtb_PreviewKeyDown(object sender, KeyEventArgs e)
        {
            if (e.Key == Key.Enter)
            {
                e.Handled = true;
            }
        }
    }
}
