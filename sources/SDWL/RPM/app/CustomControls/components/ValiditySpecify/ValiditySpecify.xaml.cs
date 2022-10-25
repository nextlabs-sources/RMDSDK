using CustomControls.common.helper;
using CustomControls.common.sharedResource;
using CustomControls.components.ValiditySpecify.model;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Text.RegularExpressions;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;

namespace CustomControls.components.ValiditySpecify
{
    /// <summary>
    /// Interaction logic for ValiditySpecify.xaml
    /// </summary>
    public partial class ValiditySpecify : UserControl
    {
        private const string DATE_FORMATTER = "MMMM dd, yyyy";

        private ValiditySpecifyConfig validitySpecifyConfig;
        private int years;
        private int months = 1;
        private int weeks = 0;
        private int days;

        private DateTime settedStartDateTime;
        private DateTime settedEndDateTime;
        //for clear button
        private DateTime clearStartDateTime;
        private DateTime clearEndDateTime;

        private DateTime settedRelativeEndDateTime;
        private DateTime settedAbsoluteEndDateTime;
        private DateTime settedRangeStartDateTime;
        private DateTime settedRangeEndDateTime;

        private long relativeStartDateTimeMillis;
        private long relativeEndDateTimeMillis;
        private string today;
        private string another;
        private string rangestart;
        private string rangeend;

        public ValiditySpecify()
        {
            this.Resources.MergedDictionaries.Add(SharedDictionaryManager.StringResource);

            InitializeComponent();

            // If code is "this.DataContext = validitySpecifyConfig", external user can't binding DependencyProperty ExpiryProperty.
            // so set DataContext to DockPanel
            this.dockP.DataContext = validitySpecifyConfig = new ValiditySpecifyConfig(this);
        }

        #region Declare and register Dependency Property

        public static readonly DependencyProperty ExpiryProperty = DependencyProperty.Register("Expiry", 
            typeof(IExpiry), typeof(ValiditySpecify),
            new FrameworkPropertyMetadata(new PropertyChangedCallback(OnExpiryPropertyChanged)));

        public static readonly DependencyProperty RadioButtonOrientationProperty = DependencyProperty.Register("RadioButtonOrientation", 
            typeof(Orientation), typeof(ValiditySpecify),
            new FrameworkPropertyMetadata(Orientation.Horizontal));
        
        /// <summary>
        /// Use for Init data
        /// </summary>
        public IExpiry Expiry
        {
            get { return (IExpiry)GetValue(ExpiryProperty); }
            set { SetValue(ExpiryProperty, value); }
        }
        /// <summary>
        /// Radio button stackPanel orientation
        /// </summary>
        public Orientation RadioButtonOrientation
        {
            get { return (Orientation)GetValue(RadioButtonOrientationProperty); }
            set { SetValue(RadioButtonOrientationProperty, value); }
        }

        private static void OnExpiryPropertyChanged(DependencyObject sender, DependencyPropertyChangedEventArgs e)
        {
            ValiditySpecify validitySpecify = (ValiditySpecify)sender;
            if (e.Property == ExpiryProperty)
            {
                IExpiry expiry = (IExpiry)e.NewValue;
                validitySpecify.DoInitial(expiry);
            }
        }

        #endregion

        #region Declare and register RoutedEvent

        public static readonly RoutedEvent ExpiryValueChangedEvent =
           EventManager.RegisterRoutedEvent("ExpiryValueChanged", RoutingStrategy.Bubble,
               typeof(RoutedPropertyChangedEventHandler<ExpiryValueChangedEventArgs>), typeof(ValiditySpecify));

        /// <summary>
        /// Change selected value, will trigger this event
        /// </summary>
        public event RoutedPropertyChangedEventHandler<ExpiryValueChangedEventArgs> ExpiryValueChanged
        {
            add { AddHandler(ExpiryValueChangedEvent, value); }
            remove { RemoveHandler(ExpiryValueChangedEvent, value); }
        }

        private void OnValidityDateChanged(ExpiryValueChangedEventArgs oldValue, ExpiryValueChangedEventArgs newValue)
        {
            RoutedPropertyChangedEventArgs<ExpiryValueChangedEventArgs> args = new RoutedPropertyChangedEventArgs<ExpiryValueChangedEventArgs>(oldValue, newValue);
            args.RoutedEvent = ValiditySpecify.ExpiryValueChangedEvent;
            RaiseEvent(args);
        }

        /// <summary>
        /// Get Expiry and trigger ExpiryValueChangedEvent
        /// </summary>
        private void GetExpiryValueHandler()
        {
            GetExpireValue(out IExpiry expiry, out string validityContent);
            OnValidityDateChanged(new ExpiryValueChangedEventArgs { },
                new ExpiryValueChangedEventArgs { Expiry=expiry, ExpiryDateValue=validityContent });
        }
        #endregion

        private void DoInitial(IExpiry expiry)
        {
            Console.WriteLine("Start init ValiditySpecify");
            //Init start date timillis.
            //Get current year,month,day.
            int year = DateTime.Now.Year;
            int month = DateTime.Now.Month;
            int day = DateTime.Now.Day;
            //Set start DateTime as 2018-03-20-0:0:0 OR 12:0:0 AM;
            settedStartDateTime = new DateTime(year, month, day, 0, 0, 0);
            clearStartDateTime = settedStartDateTime;
            //Set End DateTime add 1 month make the end like 23:59:59 OR 11:59:59 PM;
            settedEndDateTime = settedStartDateTime.AddYears(years).AddMonths(months).AddDays(7 * weeks + days - 1).AddHours(23).AddMinutes(59).AddSeconds(59);
            clearEndDateTime = settedEndDateTime;

            relativeStartDateTimeMillis = settedStartDateTime.Ticks;
            relativeEndDateTimeMillis = settedEndDateTime.Ticks;

            settedRelativeEndDateTime = settedEndDateTime;

            settedAbsoluteEndDateTime = settedEndDateTime;

            settedRangeStartDateTime = settedStartDateTime;
            settedRangeEndDateTime = settedEndDateTime;

            today = settedStartDateTime.ToString(DATE_FORMATTER);

            ParseData(expiry);

            Console.WriteLine("End init ValiditySpecify");
        }

        private void ParseData(IExpiry expiry)
        {
            //for Restore the last operation
            if (expiry != null)
            {
                int opetion = expiry.GetOpetion();
                switch (opetion)
                {
                    case 1://validitySpecifyConfig.ExpiryMode == ExpiryMode.RELATIVE
                        IRelative relative = (IRelative)expiry;
                        years = relative.GetYears();
                        months = relative.GetMonths();
                        weeks = relative.GetWeeks();
                        days = relative.GetDays();
                        if (years == 0 && months == 0 && weeks == 0 && days == 0)
                        {
                            days = 1;
                        }
                        DateTime dateEnd= settedStartDateTime.AddYears(years).AddMonths(months).AddDays(7 * weeks + days - 1).AddHours(23).AddMinutes(59).AddSeconds(59);
                        relativeEndDateTimeMillis = dateEnd.Ticks;

                        validitySpecifyConfig.ExpiryMode = ExpiryMode.RELATIVE;
                        break;
                    case 2://validitySpecifyConfig.ExpiryMode == ExpiryMode.ABSOLUTE_DATE
                        IAbsolute absolute=(IAbsolute)expiry;
                        string aEnd = DateTimeHelper.TimestampToDateTime(absolute.EndDate());
                        settedAbsoluteEndDateTime = (DateTime)GenerateSettedEndDate(Convert.ToDateTime(aEnd));
                        if (settedAbsoluteEndDateTime < settedStartDateTime)
                        {
                            settedAbsoluteEndDateTime = (DateTime)GenerateSettedEndDate(settedStartDateTime);
                        }
                        validitySpecifyConfig.ExpiryMode = ExpiryMode.ABSOLUTE_DATE;
                        break;
                    case 3://validitySpecifyConfig.ExpiryMode == ExpiryMode.DATA_RANGE
                        IRange range = (IRange)expiry;
                        string rStart= DateTimeHelper.TimestampToDateTime(range.StartDate());
                        string rEnd = DateTimeHelper.TimestampToDateTime(range.EndDate());
                        settedRangeStartDateTime = (DateTime)GenerateSettedStartDate(Convert.ToDateTime(rStart));
                        settedRangeEndDateTime = (DateTime)GenerateSettedEndDate(Convert.ToDateTime(rEnd));
                        if (settedRangeStartDateTime < settedStartDateTime)
                        {
                            settedRangeStartDateTime = settedStartDateTime;
                            settedRangeEndDateTime = settedEndDateTime;
                        }
                        validitySpecifyConfig.ExpiryMode = ExpiryMode.DATA_RANGE;
                        break;
                    default:
                        break;
                }
            }
        }

        /*
         *Callback of radio button checked.
         */
        private void RadioButton_ModeChecked(object sender, RoutedEventArgs e)
        {
            RadioButton radioButton = sender as RadioButton;
            if (radioButton != null && radioButton.Name != null)
            {
                switch (radioButton.Name.ToString())
                {
                    case "Radio_Never_Expire":
                        Never_Expire();
                        break;
                    case "Radio_Relative":
                        //Text change will excute TextBox_InputTextChanged event
                        this.yearsTB.Text = years.ToString();
                        this.monthsTB.Text = months.ToString();
                        this.weeksTB.Text = weeks.ToString();
                        this.daysTB.Text = days.ToString();

                        Relative();
                        break;
                    case "Radio_Absolute_Date":
                        Absolute_Date();
                        break;
                    case "Radio_Data_Range":
                        Data_Range();
                        break;
                }
                //SolidColorBrush myBrush = new SolidColorBrush(System.Windows.Media.Color.FromArgb(0xFF, 0x27, 0xAE, 0x60));
                //radioButton.Foreground = (bool)radioButton.IsChecked ? myBrush : Brushes.Black;
            }
            GetExpiryValueHandler();
        }

        private void Never_Expire()
        {
            validitySpecifyConfig.ExpiryMode = ExpiryMode.NEVER_EXPIRE;
            validitySpecifyConfig.ValidityDateValue = this.TryFindResource("ValidityCom_Never_Description0").ToString();
        }

        private void Relative()
        {
            validitySpecifyConfig.ExpiryMode = ExpiryMode.RELATIVE;

            long elapsedTicks = relativeEndDateTimeMillis - relativeStartDateTimeMillis;
            TimeSpan elapsedSpan = new TimeSpan(elapsedTicks);

            another = settedRelativeEndDateTime.ToString(DATE_FORMATTER);

            //Nofify UI changes.
            validitySpecifyConfig.ValidityDateValue = today + " To " + another;
            validitySpecifyConfig.ValidityCountDaysValue = (elapsedSpan.Days + 1).ToString() + " days";
        }

        private void Absolute_Date()
        {
            validitySpecifyConfig.ExpiryMode = ExpiryMode.ABSOLUTE_DATE;
            another = settedAbsoluteEndDateTime.ToString(DATE_FORMATTER);

            //Nofify UI change.
            validitySpecifyConfig.ValidityDateValue = "Until " + another;
            validitySpecifyConfig.ValidityCountDaysValue = CountDays(settedStartDateTime.Ticks, settedAbsoluteEndDateTime.Ticks) + " days";

            //Update calendar selected dates.
            this.calendar1.SelectedDate = settedAbsoluteEndDateTime;
            this.calendar1.DisplayDate = settedAbsoluteEndDateTime;
            //Update calendar blackout dates.
            UpdateBlackOutDates(this.calendar1, settedStartDateTime);

        }

        private void Data_Range()
        {
            validitySpecifyConfig.ExpiryMode = ExpiryMode.DATA_RANGE;
            rangestart = settedRangeStartDateTime.ToString(DATE_FORMATTER);
            rangeend = settedRangeEndDateTime.ToString(DATE_FORMATTER);

            validitySpecifyConfig.ValidityDateValue = rangestart + " To " + rangeend;
            validitySpecifyConfig.ValidityCountDaysValue = CountDays(settedRangeStartDateTime.Ticks, settedRangeEndDateTime.Ticks) + " days";
            //Update calendar selected dates.
            this.calendar1.SelectedDate = settedRangeStartDateTime;
            this.calendar1.DisplayDate = settedRangeStartDateTime;
            this.calendar2.SelectedDate = settedRangeEndDateTime;
            this.calendar2.DisplayDate = settedRangeEndDateTime;
            //Update calendar blackout dates.
            UpdateBlackOutDates(this.calendar1, settedStartDateTime);
            UpdateBlackOutDates(this.calendar2, settedRangeStartDateTime);
        }

        private void TextBox_PreviewTextInput(object sender, TextCompositionEventArgs e)
        {
            var textBox = sender as TextBox;
            e.Handled = Regex.IsMatch(e.Text, "[^0-9]+");
        }

        private void TextBox_InputTextChanged(object sender, TextChangedEventArgs e)
        {
            TextBox textBox = sender as TextBox;

            switch (textBox.Name)
            {
                case "yearsTB":
                    if (textBox.Text != null && textBox.Text != "" && textBox.Text != " ")
                    {
                        years = Convert.ToInt32(textBox.Text);
                    }
                    else
                    {
                        years = 0;
                    }
                    UpdateRelativeDays(years, months, weeks, days);
                    break;
                case "monthsTB":
                    if (textBox.Text != null && textBox.Text != "" && textBox.Text != " ")
                    {
                        months = Convert.ToInt32(textBox.Text);
                    }
                    else
                    {
                        months = 0;
                    }
                    UpdateRelativeDays(years, months, weeks, days);
                    break;
                case "weeksTB":
                    if (textBox.Text != null && textBox.Text != "" && textBox.Text != " ")
                    {
                        weeks = Convert.ToInt32(textBox.Text);
                    }
                    else
                    {
                        weeks = 0;
                    }
                    UpdateRelativeDays(years, months, weeks, days);
                    break;
                case "daysTB":
                    if (textBox.Text != null && textBox.Text != "" && textBox.Text != " ")
                    {
                        days = Convert.ToInt32(textBox.Text);
                    }
                    else
                    {
                        days = 0;
                    }
                    UpdateRelativeDays(years, months, weeks, days);
                    break;
            }

            GetExpiryValueHandler();
        }

        private void UpdateRelativeDays(int years, int months, int weeks, int days)
        {
            if (validitySpecifyConfig.ExpiryMode != ExpiryMode.RELATIVE)
            {
                return;
            }
            //Generate setted end date time.
            if (years == 0 && months == 0 && weeks == 0 && days == 0)
            {
                //MessageBox.Show("Thoes four inputs cannot be null at the same time.");
                this.daysTB.Text = "1";//modify texbox.text will trigger 'TextBox_InputTextChanged' event
                return;
            }
            settedRelativeEndDateTime = settedStartDateTime.AddYears(years).AddMonths(months).AddDays(7 * weeks + days - 1).AddHours(23).AddMinutes(59).AddSeconds(59);
            relativeEndDateTimeMillis = settedRelativeEndDateTime.Ticks;
            another = settedRelativeEndDateTime.ToString(DATE_FORMATTER);

            //Update ui display.
            validitySpecifyConfig.ValidityDateValue = today + " To " + another;
            //Calc ticks between two date time.
            validitySpecifyConfig.ValidityCountDaysValue = CountDays(relativeStartDateTimeMillis, relativeEndDateTimeMillis) + " days";

            //For test
            //Console.WriteLine("start---------:" + settedStartDateTime.ToString());
            //Console.WriteLine("end-----------:" + settedEndDateTime.ToString());
        }

        private void Calendar_SelectedDatesChanged(object sender, SelectionChangedEventArgs e)
        {
            Calendar calendar = sender as Calendar;
            if (calendar != null)
            {
                //When select a date in calendar,when you want click the "Select" button.However you need to click the "Select" button twice:
                //once to de-focus the calendar ,and again to actually press it.The mouse leave event does not trigger on the calendar if an 
                //item is selected onside it.
                //Add follow to avoid the situation above when Calendar selected dates changed.
                Mouse.Capture(null);
                switch (calendar.Name)
                {
                    case "calendar1":
                        if (validitySpecifyConfig.ExpiryMode == ExpiryMode.ABSOLUTE_DATE)
                        {
                            DateTime settedSelectedTime = (DateTime)GenerateSettedEndDate((DateTime)calendar.SelectedDate);
                            settedAbsoluteEndDateTime = settedSelectedTime;

                            another = settedSelectedTime.ToString(DATE_FORMATTER);
                            validitySpecifyConfig.ValidityDateValue = "Until " + another;
                            validitySpecifyConfig.ValidityCountDaysValue = CountDays(settedStartDateTime.Ticks, settedSelectedTime.Ticks) + " days";

                            Console.WriteLine("calendar1 absolute end date: " + settedSelectedTime.ToString());
                        }
                        else
                        {
                            settedRangeStartDateTime = (DateTime)GenerateSettedStartDate((DateTime)calendar.SelectedDate);
                            rangestart = settedRangeStartDateTime.ToString(DATE_FORMATTER);
                            if (settedRangeStartDateTime > settedRangeEndDateTime)
                            {
                                settedRangeEndDateTime = settedRangeStartDateTime;
                                this.calendar2.SelectedDate = settedRangeEndDateTime;
                                this.calendar2.DisplayDate = settedRangeEndDateTime;
                                rangeend = settedRangeEndDateTime.ToString(DATE_FORMATTER);
                                UpdateBlackOutDates(this.calendar2, settedRangeEndDateTime);
                            }
                            else
                            {
                                UpdateBlackOutDates(this.calendar2, settedRangeStartDateTime);
                            }
                            Console.WriteLine("calendar1 range start date: " + settedRangeStartDateTime.ToString());
                            UpdateRangeCountDays(settedRangeStartDateTime, settedRangeEndDateTime);
                        }
                        break;
                    case "calendar2":
                        settedRangeEndDateTime = (DateTime)GenerateSettedEndDate((DateTime)calendar.SelectedDate);
                        rangeend = settedRangeEndDateTime.ToString(DATE_FORMATTER);
                        UpdateRangeCountDays(settedRangeStartDateTime, settedRangeEndDateTime);
                        Console.WriteLine("calendar1 range end date: " + settedRangeEndDateTime.ToString());
                        break;
                }
                GetExpiryValueHandler();
            }
        }

        private void UpdateRangeCountDays(DateTime start, DateTime end)
        {
            validitySpecifyConfig.ValidityDateValue = start.ToString(DATE_FORMATTER) + " To " + end.ToString(DATE_FORMATTER);
            validitySpecifyConfig.ValidityCountDaysValue = CountDays(start.Ticks, end.Ticks) + " days";
        }

        private int CountDays(long startMillis, long endMillis)
        {
            long elapsedTicks = endMillis - startMillis;
            TimeSpan elapsedSpan = new TimeSpan(elapsedTicks);
            if (elapsedSpan.Days < 0)
            {
                return 0;
            }
            return elapsedSpan.Days + 1;
        }

        private DateTime? GenerateSettedStartDate(DateTime targetTime)
        {
            if (targetTime != null)
            {
                return new DateTime(targetTime.Year, targetTime.Month, targetTime.Day).AddHours(0).AddMinutes(0).AddSeconds(0);
            }
            return null;
        }

        private DateTime? GenerateSettedEndDate(DateTime dateTime)
        {
            if (dateTime != null)
            {
                return new DateTime(dateTime.Year, dateTime.Month, dateTime.Day).AddHours(23).AddMinutes(59).AddSeconds(59);
            }
            return null;
        }

        private void UpdateBlackOutDates(Calendar target, DateTime blackoutEndTime)
        {
            if (target == null)
            {
                return;
            }
            target.BlackoutDates.Clear();
            target.BlackoutDates.Add(new CalendarDateRange(DateTime.MinValue, blackoutEndTime.AddDays(-1)));
            //This is not suitable for the situation that the end dates is changeable.
            //target.BlackoutDates.AddDatesInPast();
        }

        private void GetExpireValue(out IExpiry expiry, out string validityContent)
        {
            expiry = null;
            validityContent = this.TryFindResource("ValidityCom_Never_Description2").ToString();
            if (validitySpecifyConfig != null)
            {
                switch (validitySpecifyConfig.ExpiryMode)
                {
                    case ExpiryMode.NEVER_EXPIRE:
                        expiry = new NeverExpireImpl();
                        validityContent = this.TryFindResource("ValidityCom_Never_Description2").ToString();
                        break;
                    case ExpiryMode.RELATIVE:
                        expiry = new RelativeImpl(years, months, weeks, days);
                        validityContent = today + " To " + another;
                        break;
                    case ExpiryMode.ABSOLUTE_DATE:
                        expiry = new AbsoluteImpl(DateTimeHelper.DateTimeToTimestamp(settedAbsoluteEndDateTime));
                        validityContent = "Until " + another;
                        break;
                    case ExpiryMode.DATA_RANGE:
                        expiry = new RangeImpl(DateTimeHelper.DateTimeToTimestamp(settedRangeStartDateTime), DateTimeHelper.DateTimeToTimestamp(settedRangeEndDateTime));
                        validityContent = rangestart + " To " + rangeend;
                        break;
                }
            }
        }

        //should use  CommonUtils.DateTimeToTimestamp(), should Gets the time zone of the current computer.
        private long GetUnixTimestamp(DateTime target) => (long)target.Subtract(new DateTime(1970, 1, 1, 0, 0, 0)).TotalMilliseconds;

        private void BtnClear1_MouseLeftButtonUp(object sender, MouseButtonEventArgs e)
        {

            if (validitySpecifyConfig.ExpiryMode == ExpiryMode.ABSOLUTE_DATE)//ExpiryMode.ABSOLUTE_DATE
            {
                clearStartDateTime = settedEndDateTime;
            }
            if (validitySpecifyConfig.ExpiryMode == ExpiryMode.DATA_RANGE)
            {
                clearStartDateTime = settedStartDateTime;
            }

            this.calendar1.SelectedDate = clearStartDateTime;
            this.calendar1.DisplayDate = clearStartDateTime;
        }

        private void BtnClear2_MouseLeftButtonUp(object sender, MouseButtonEventArgs e)
        {
            //Data_Range
            clearEndDateTime = settedRangeStartDateTime.AddDays(29).AddHours(23).AddMinutes(59).AddSeconds(59);

            this.calendar2.SelectedDate = clearEndDateTime;
            this.calendar2.DisplayDate = clearEndDateTime;
        }
    }

}
