using CustomControls.common.sharedResource;
using CustomControls.pages.CentralPolicy.model;
using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Linq;
using System.Text;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Controls.Primitives;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;

namespace CustomControls.pages.CentralPolicy
{
    /// <summary>
    /// Interaction logic for SelectCentralPolicy.xaml
    /// </summary>
    public partial class SelectCentralPolicy : Page
    {
        private Dictionary<string, LabelUIElement> ProjectClassifications = new Dictionary<string, LabelUIElement>();

        public SelectCentralPolicy()
        {
            this.Resources.MergedDictionaries.Add(SharedDictionaryManager.StringResource);
            InitializeComponent();
        }

        #region Declare and register Dependency Property

        public static readonly DependencyProperty DescribeTextProperty = DependencyProperty.Register("DescribeText",
            typeof(string), typeof(SelectCentralPolicy));

        public static readonly DependencyProperty DescribeTextAlignmentProperty = DependencyProperty.Register("DescribeTextAlignment",
            typeof(TextAlignment), typeof(SelectCentralPolicy), new FrameworkPropertyMetadata(TextAlignment.Center));

        public static readonly DependencyProperty WarningDescribeTextProperty = DependencyProperty.Register("WarningDescribeText",
            typeof(string), typeof(SelectCentralPolicy));

        public static readonly DependencyProperty WarningVisibilityProperty = DependencyProperty.Register("WarningVisibility",
            typeof(Visibility), typeof(SelectCentralPolicy),new PropertyMetadata(Visibility.Collapsed));

        public static readonly DependencyProperty ClassificationProperty = DependencyProperty.Register("Classification", 
            typeof(Classification[]), typeof(SelectCentralPolicy), new FrameworkPropertyMetadata(new Classification[0], new PropertyChangedCallback(OnClassificationPropertyChanged)));

        public static readonly DependencyProperty AddInheritedClassificationProperty = DependencyProperty.Register("AddInheritedClassification",
            typeof(Dictionary<string, List<string>>), typeof(SelectCentralPolicy), new FrameworkPropertyMetadata(new PropertyChangedCallback(OnAddandSelectedTagPropertyChanged)));

        #region .NET property wrapper Dependency Property
        /// <summary>
        /// Display describeText in top, defult value is null
        /// </summary>
        public string DescribeText
        {
            get { return (string)GetValue(DescribeTextProperty); }
            set { SetValue(DescribeTextProperty, value); }
        }

        /// <summary>
        /// Display warning describeText in under of "DescribeText", defult value is null
        /// </summary>
        public string WarningDescribeText
        {
            get { return (string)GetValue(WarningDescribeTextProperty); }
            set { SetValue(WarningDescribeTextProperty, value); }
        }

        /// <summary>
        /// Warning describeText visibility, defult value is Collapsed
        /// </summary>
        public Visibility WarningVisibility
        {
            get { return (Visibility)GetValue(WarningVisibilityProperty); }
            set { SetValue(WarningVisibilityProperty, value); }
        }

        /// <summary>
        /// DescribeText TextAlignment, defult value is Center
        /// </summary>
        public string DescribeTextAlignment
        {
            get { return (string)GetValue(DescribeTextAlignmentProperty); }
            set { SetValue(DescribeTextAlignmentProperty, value); }
        }
        /// <summary>
        /// Init data to display
        /// </summary>
        public Classification[] Classification
        {
            get { return (Classification[])GetValue(ClassificationProperty); }
            set { SetValue(ClassificationProperty, value); }
        }
        /// <summary>
        /// For modify rights, display the Inherited Tags and defult selected.
        /// This parameter must be set after setting Classification property
        /// </summary>
        public Dictionary<string, List<string>> AddInheritedClassification
        {
            get { return (Dictionary<string, List<string>>)GetValue(AddInheritedClassificationProperty); }
            set { SetValue(AddInheritedClassificationProperty, value); }
        }
        #endregion

        private static void OnClassificationPropertyChanged(DependencyObject sender, DependencyPropertyChangedEventArgs e)
        {
            SelectCentralPolicy centralPolicy = (SelectCentralPolicy)sender;
            if (e.Property == ClassificationProperty)
            {
                Classification[] classifications = (Classification[])e.NewValue;
                centralPolicy.InitClassifications(classifications);
            }
        }
        private static void OnAddandSelectedTagPropertyChanged(DependencyObject sender, DependencyPropertyChangedEventArgs e)
        {
            SelectCentralPolicy centralPolicy = (SelectCentralPolicy)sender;
            if (e.Property == AddInheritedClassificationProperty)
            {
                Dictionary<string, List<string>> classifications = (Dictionary<string, List<string>>)e.NewValue;
                centralPolicy.AddInheritedTags(classifications);
                centralPolicy.SetDefultSelectTags(classifications);
            }
        }
        #endregion

        #region Declare and register RoutedEvent

        public static readonly RoutedEvent SelectClassificationChangedEvent =
           EventManager.RegisterRoutedEvent("SelectClassificationChanged", RoutingStrategy.Bubble,
               typeof(RoutedPropertyChangedEventHandler<SelectClassificationEventArgs>), typeof(SelectCentralPolicy));
        
        /// <summary>
        /// When selected classification changed, will trigger this event and transmit selected new value.
        /// </summary>
        public event RoutedPropertyChangedEventHandler<SelectClassificationEventArgs> SelectClassificationChanged
        {
            add { AddHandler(SelectClassificationChangedEvent, value); }
            remove { RemoveHandler(SelectClassificationChangedEvent, value); }
        }

        private void OnClassificationChanged(SelectClassificationEventArgs oldValue, SelectClassificationEventArgs newValue)
        {
            RoutedPropertyChangedEventArgs<SelectClassificationEventArgs> args = new RoutedPropertyChangedEventArgs<SelectClassificationEventArgs>(oldValue, newValue);
            args.RoutedEvent = SelectCentralPolicy.SelectClassificationChangedEvent;
            RaiseEvent(args);
        }

        /// <summary>
        /// Get classification and trigger ClassificationChangedEvent
        /// </summary>
        private void GetClassificationHandler()
        {
            bool result_N = GetClassification(out Dictionary<string, List<string>> newValue);
            OnClassificationChanged(new SelectClassificationEventArgs { },
                new SelectClassificationEventArgs { IsValid = result_N, KeyValues = newValue });
        }
        #endregion

        #region Init Controls
        private void InitClassifications(Classification[] projectClassifications)
        {
            InitData(projectClassifications);
        }

        private void InitData(Classification[] classifications)
        {
            WrapPanel.Children.Clear();
            ProjectClassifications.Clear();
            //if (classifications.Length != 0)
            //{
            //    WrapPanel.Children.Add(CompanyDefineString(this.TryFindResource("Rights_Company_Text").ToString()));
            //}

            foreach (Classification section in classifications)
            {
                if (ProjectClassifications.ContainsKey(section.name))
                {
                    continue;
                }

                LabelUIElement labelUIElement = new LabelUIElement
                {
                    IsMultiSelect = section.isMultiSelect,
                    IsMandatory = section.isMandatory
                };

                //Create Classification textblock.
                TextBlock titleTB = CreateClassificationTB(section.name);

                labelUIElement.Title = titleTB;
                //Attach classificaiton textblock to the wrap panel.
                WrapPanel.Children.Add(titleTB);

                List<ToggleButton> labelBTs = new List<ToggleButton>();

                labelUIElement.Lables = labelBTs;

                ProjectClassifications.Add(section.name, labelUIElement);

                UInt16 checkedSize = 0;
                foreach (KeyValuePair<String, Boolean> kv in section.labels)
                {
                    ToggleButton label = new ToggleButton
                    {
                        Tag = section.name,
                        Padding = new Thickness(15, 5, 15, 5)
                    };

                    labelBTs.Add(label);
                    //If tag is selected by default then set the toogle button as checked.
                    if (kv.Value)
                    {
                        checkedSize++;
                        label.IsChecked = true;
                    }
                    else
                    {
                        label.IsChecked = false;
                    }
                    //Bind toggle button checked event.
                    label.Checked += new RoutedEventHandler(ToggleButton_Checked);
                    //Bind toggle button unchecked event.
                    label.Unchecked += new RoutedEventHandler(ToggleButton_Unchecked);

                    label.Content = kv.Key;
                    label.ToolTip = kv.Key;
                    
                    //Bind those label button to wrap panel.
                    WrapPanel.Children.Add(label);
                }

                //If this section is mandatory and no item selected by default.
                //then keep the mandatory run as hint style by set forground color as red.
                if (section.isMandatory)
                {
                    AddMandatory(titleTB, checkedSize == 0);
                }
            }
            GetClassificationHandler();
        }

        private TextBlock CompanyDefineString(string value)
        {
            return new TextBlock
            {
                Width = 1000,
                Foreground = new SolidColorBrush(Colors.Gray),
                Background = new SolidColorBrush(Colors.White),
                Padding = new Thickness(10, 20, 10, 10),
                TextAlignment = TextAlignment.Center,
                HorizontalAlignment = HorizontalAlignment.Center,
                FontSize = 14,
                FontFamily = new FontFamily("Lato"),
                FontStyle = FontStyles.Normal,
                FontWeight = FontWeights.Regular,
                Text = value,
                TextWrapping=TextWrapping.Wrap
            };
        }

        private TextBlock CreateClassificationTB(string value)
        {
            return new TextBlock
            {
                Width = 1000,
                Foreground = new SolidColorBrush(Colors.Black),
                Background = new SolidColorBrush(Colors.White),
                Padding = new Thickness(10, 20, 10, 10),
                TextAlignment = TextAlignment.Left,
                FontSize = 16,
                FontFamily = new FontFamily("Lato"),
                FontStyle = FontStyles.Normal,
                FontWeight = FontWeights.SemiBold,
                Text = value,
                ToolTip = value
            };
        }

        private void AddMandatory(TextBlock tb, bool hint)
        {
            tb.Inlines.Add(CreateMandatoryRun(hint ? Colors.Red : Colors.DarkGray));
        }

        private Run CreateMandatoryRun(Color color)
        {
            return new Run
            {
                Foreground = new SolidColorBrush(color),
                FontSize = 14,
                Text = this.TryFindResource("SelectCentral_Mandatory").ToString(),
                FontWeight = FontWeights.Normal,
            };
        }

        private void UpdateTitleTB(TextBlock title, bool hint)
        {
            string value = title.Text;
            title.Inlines.Remove(title.Inlines.LastInline);
            AddMandatory(title, hint);
        }

        private void ToggleButton_Unchecked(object sender, RoutedEventArgs e)
        {
            ToggleButton label = sender as ToggleButton;

            foreach (KeyValuePair<string, LabelUIElement> kvp in ProjectClassifications)
            {
                if (kvp.Key.Equals(label.Tag.ToString()))
                {
                    LabelUIElement labelUIElement = kvp.Value;

                    if (labelUIElement.IsMandatory)
                    {
                        bool b = false;
                        foreach (ToggleButton item in labelUIElement.Lables)
                        {
                            if (item.IsChecked == true)
                            {
                                b = true;
                                break;
                            }
                        }

                        if (!b)
                        {
                            //label.IsChecked = true;
                            TextBlock title = labelUIElement.Title;
                            UpdateTitleTB(title, true);
                        }
                    }
                }
            }

            GetClassificationHandler();
        }

        private void ToggleButton_Checked(object sender, RoutedEventArgs e)
        {
            ToggleButton target = sender as ToggleButton;
            //Keep the target tag string.
            string content = target.Content.ToString();
            //Iterate the ProjectClassifications to find the current LabelUIElement belong to the current tag.
            foreach (KeyValuePair<string, LabelUIElement> kvp in ProjectClassifications)
            {
                if (kvp.Key.Equals(target.Tag.ToString()))
                {
                    //The current LabelUIElement section the checked item belongs to.
                    LabelUIElement section = kvp.Value;
                    //If this section not support multi select
                    //then should diselect the rest of items belong to the current section.
                    if (!section.IsMultiSelect)
                    {
                        foreach (ToggleButton label in section.Lables)
                        {
                            string labelContent = label.Content.ToString();
                            if (!content.Equals(labelContent))
                            {
                                if (label.IsChecked == true)
                                {
                                    label.IsChecked = false;
                                }
                            }
                        }
                    }
                    //If this section is mandatory 
                    //then should check if there is no item checked.
                    if (section.IsMandatory)
                    {
                        TextBlock title = section.Title;
                        UpdateTitleTB(title, false);
                    }
                }
            }

            GetClassificationHandler();
        }
        #endregion

        /// <summary>
        /// Get selected classifications
        /// </summary>
        /// <returns></returns>
        private bool GetClassification(out Dictionary<string, List<string>> keyValuePairs)
        {
            bool result = true;
            keyValuePairs = new Dictionary<string, List<string>>();

            var clist = IsCorrectChooseClassification();
            if (clist.Count != 0)
            {
                result = false;
                return result;
            }
            
            foreach (KeyValuePair<string, LabelUIElement> kvp in ProjectClassifications)
            {
                string key = kvp.Key;
                LabelUIElement labelUIElement = kvp.Value;
                List<string> list = new List<string>();
                foreach (ToggleButton toggleButton in labelUIElement.Lables)
                {
                    if (toggleButton.IsChecked == true)
                    {
                        list.Add(toggleButton.Content.ToString());
                    }
                }
                if (list.Count != 0)
                {
                    keyValuePairs.Add(key, list);
                }
            }
            return result;
        }

        //Return incorrect Choosed Classification key
        private List<string> IsCorrectChooseClassification()
        {
            List<string> result = new List<string>();
            foreach (KeyValuePair<string, LabelUIElement> kvp in ProjectClassifications)
            {
                string key = kvp.Key;
                LabelUIElement labelUIElement = kvp.Value;
                if (labelUIElement.IsMandatory)
                {
                    bool IsCorrectChoose = false;
                    foreach (ToggleButton item in labelUIElement.Lables)
                    {
                        if (item.IsChecked == true)
                        {
                            IsCorrectChoose = true;
                            break;
                        }
                    }
                    if (!IsCorrectChoose)
                    {
                        result.Add(key);
                    }
                }
            }
            return result;
        }

        #region For modify rights, set defult select tags

        /// <summary>
        /// Display the Inherited Tags on the CentralPolicyPage. if the key existed, will not add.
        /// </summary>
        /// <param name="keyValues"></param>
        private void AddInheritedTags(Dictionary<string, List<string>> keyValues)
        {
            foreach (var item in keyValues)
            {
                if (ProjectClassifications.ContainsKey(item.Key))
                {
                    continue;
                }

                LabelUIElement labelUIElement = new LabelUIElement
                {
                    IsMultiSelect = item.Value.Count > 1,
                    IsMandatory = false
                };

                //Create Classification textblock.
                TextBlock titleTB = CreateClassificationTB(item.Key);

                labelUIElement.Title = titleTB;
                //Attach classificaiton textblock to the wrap panel.
                WrapPanel.Children.Add(titleTB);

                List<ToggleButton> labelBTs = new List<ToggleButton>();

                labelUIElement.Lables = labelBTs;

                ProjectClassifications.Add(item.Key, labelUIElement);

                UInt16 checkedSize = 0;
                foreach (var kv in item.Value)
                {
                    ToggleButton label = new ToggleButton
                    {
                        Tag = item.Key,
                        Padding = new Thickness(15, 5, 15, 5)
                    };

                    labelBTs.Add(label);

                    checkedSize++;
                    // Inherited Tags should defult checked.
                    label.IsChecked = true;

                    //Bind toggle button checked event.
                    label.Checked += new RoutedEventHandler(ToggleButton_Checked);
                    //Bind toggle button unchecked event.
                    label.Unchecked += new RoutedEventHandler(ToggleButton_Unchecked);

                    label.Content = kv;
                    //Bind those label button to wrap panel.
                    WrapPanel.Children.Add(label);
                }

                //If this section is mandatory and no item selected by default.
                //then keep the mandatory run as hint style by set forground color as red.
                //if (item.isMandatory)
                //{
                //    AddMandatory(titleTB, checkedSize == 0);
                //}
            }
        }

        /// <summary>
        /// Set defult selected tag for modify rights
        /// </summary>
        /// <param name="keyValues"></param>
        private void SetDefultSelectTags(Dictionary<string, List<string>> keyValues)
        {
            var tags = keyValues;
            //Check nonull for tags.
            //If there is nothing just return.
            if (tags == null || tags.Count == 0)
            {
                return;
            }
            //Get the iterator of the dictionary.
            var iterator = tags.GetEnumerator();
            //If there is any items inside it.
            while (iterator.MoveNext())
            {
                //Get the current one.
                var current = iterator.Current;

                var key = current.Key;
                var values = current.Value;

                DefultTgBtnChecked(key, values);
            }
        }

        private void DefultTgBtnChecked(string key, List<string> values)
        {
            foreach (var value in values)
            {
                foreach (var item in WrapPanel.Children)
                {
                    if (item is ToggleButton)
                    {
                        ToggleButton button = item as ToggleButton;
                        if (button.Tag.ToString() == key)
                        {
                            if (button.Content.ToString() == value)
                            {
                                button.IsChecked = true;
                                break;
                            }
                        }
                    }
                }
            }
        }
        #endregion

    }
    
}
