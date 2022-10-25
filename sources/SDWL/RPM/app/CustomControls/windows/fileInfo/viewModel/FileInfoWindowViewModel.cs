using CustomControls.common.helper;
using CustomControls.components.DigitalRights.model;
using CustomControls.windows.fileInfo.helper;
using CustomControls.windows.fileInfo.view;
using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.ComponentModel;
using System.IO;
using System.Linq;
using System.Text;
using System.Windows;
using System.Windows.Documents;
using System.Windows.Media;

namespace CustomControls.windows.fileInfo.viewModel
{
    public class FileInfoWindowViewModel : INotifyPropertyChanged
    {
        public event PropertyChangedEventHandler PropertyChanged;
        private string name = string.Empty;
        private string path = string.Empty;
        private Int64  size = 0;
        private string lastModified = string.Empty;
        private Expiration expiration;
        private string waterMark = string.Empty;
        private bool isByCentrolPolicy = false;
        private ObservableCollection<string> emails = new ObservableCollection<string>();
        private ObservableCollection<FileRights> fileRights = new ObservableCollection<FileRights>();
        private Dictionary<string, List<string>> centralTag = new Dictionary<string, List<string>>();
        private FileMetadate fileMetadate;
        private FileInfoWindow host;

        public string Name
        {
            get { return name; }
            set
            {
                name = value;
                PropertyChanged?.Invoke(this, new PropertyChangedEventArgs("Name"));

            }
        }

        public string Path
        {
            get { return path; }
            set
            {
                path = value;
                PropertyChanged?.Invoke(this, new PropertyChangedEventArgs("Path"));

            }
        }

        public Int64 Size
        {
            get { return size; }
            set
            {
                size = value;
                PropertyChanged?.Invoke(this, new PropertyChangedEventArgs("Size"));
            }
        }

        public string LastModified
        {
            get { return lastModified; }
            set
            {
                lastModified = value;
                PropertyChanged?.Invoke(this, new PropertyChangedEventArgs("LastModified"));
            }
        }

        public Expiration Expiration
        {
            get { return expiration; }
            set
            {
                expiration = value;
                PropertyChanged?.Invoke(this, new PropertyChangedEventArgs("Expiration"));

            }
        }
     
        public string WaterMark
        {
            get { return waterMark; }
            set
            {
                SetWaterMark(value);
                PropertyChanged?.Invoke(this, new PropertyChangedEventArgs("WaterMark"));
            }
        }

        private const string DOLLAR_USER = "$(User)";
        private const string DOLLAR_BREAK = "$(Break)";
        private const string DOLLAR_DATE = "$(Date)";
        private const string DOLLAR_TIME = "$(Time)";
        private string PRESET_VALUE_EMAIL_ID;
        private string PRESET_VALUE_DATE;
        private string PRESET_VALUE_TIME;
        private string PRESET_VALUE_LINE_BREAK;
        private void SetWaterMark(string value)
        {
            if (value == null)
            {
                return;
            }

            string initWatermark = value;
            if (initWatermark.Contains("\n"))
            {
                initWatermark = initWatermark.Replace("\n", DOLLAR_BREAK);
            }
            waterMark = initWatermark;

            this.host.tbWaterMark.Inlines.Clear();
            PRESET_VALUE_EMAIL_ID = host.TryFindResource("Preset_User_ID").ToString();
            PRESET_VALUE_DATE = host.TryFindResource("Preset_Date").ToString();
            PRESET_VALUE_TIME = host.TryFindResource("Preset_Time").ToString();
            PRESET_VALUE_LINE_BREAK = host.TryFindResource("Preset_Line_break").ToString();
            ConvertString2PresetValue(initWatermark);
        }
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

                    // append text before preset value
                    Run run = new Run(initValue.Substring(0, beginIndex));
                    this.host.tbWaterMark.Inlines.Add(run);

                    // judge if is preset
                    string subStr = initValue.Substring(beginIndex, endIndex - beginIndex + 1);

                    if (subStr.Equals(DOLLAR_USER))
                    {
                        AddPreset(DOLLAR_USER);
                    }
                    else if (subStr.Equals(DOLLAR_BREAK))
                    {
                        AddPreset(DOLLAR_BREAK);
                    }
                    else if (subStr.Equals(DOLLAR_DATE))
                    {
                        AddPreset(DOLLAR_DATE);
                    }
                    else if (subStr.Equals(DOLLAR_TIME))
                    {
                        AddPreset(DOLLAR_TIME);
                    }
                    else
                    {
                        Run r = new Run(subStr);
                        this.host.tbWaterMark.Inlines.Add(r);
                    }

                    // quit
                    break;
                }
            }

            if (beginIndex == -1 || endIndex == -1 || beginIndex > endIndex) // have not preset
            {
                Run run = new Run(initValue);
                this.host.tbWaterMark.Inlines.Add(run);
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
        private void AddPreset(string preset)
        {
            Run run = new Run();

            Run space = new Run(" ");
            this.host.tbWaterMark.Inlines.Add(space);

            switch (preset)
            {
                case DOLLAR_USER:
                    run.Text = PRESET_VALUE_EMAIL_ID;
                    run.Background = new SolidColorBrush(Color.FromRgb(0XD4, 0XEF, 0XDF));
                    break;
                case DOLLAR_BREAK:
                    run.Text = PRESET_VALUE_LINE_BREAK;
                    run.Background = new SolidColorBrush(Color.FromRgb(0XFA, 0XD7, 0XB8));
                    break;
                case DOLLAR_DATE:
                    run.Text = PRESET_VALUE_DATE;
                    run.Background = new SolidColorBrush(Color.FromRgb(0XD4, 0XEF, 0XDF));
                    break;
                case DOLLAR_TIME:
                    run.Text = PRESET_VALUE_TIME;
                    run.Background = new SolidColorBrush(Color.FromRgb(0XD4, 0XEF, 0XDF));
                    break;
                default:
                    break;
            }

            this.host.tbWaterMark.Inlines.Add(run);

            Run space2 = new Run(" ");
            this.host.tbWaterMark.Inlines.Add(space2);
        }

        public bool IsByCentrolPolicy
        {
            get
            {
                return isByCentrolPolicy;
            }
            set
            {
                isByCentrolPolicy = value;
                PropertyChanged?.Invoke(this, new PropertyChangedEventArgs("IsByCentrolPolicy"));
            }
        }

        public ObservableCollection<string> Emails
        {
            get
            {
                return emails;
            }
            set
            {
                emails = value;
                PropertyChanged?.Invoke(this, new PropertyChangedEventArgs("Emails"));
            }
        }

        public ObservableCollection<FileRights> FileRights
        {
            get
            {
                return fileRights;
            }
            set
            {
                fileRights = value;
                PropertyChanged?.Invoke(this, new PropertyChangedEventArgs("FileRights"));
            }
        }

        public Dictionary<string, List<string>> CentralTag
        {
            get
            {
                return centralTag;
            }
            set
            {
                centralTag = value;
                PropertyChanged?.Invoke(this, new PropertyChangedEventArgs("CentralTag"));
            }
        }

        public FileMetadate FileMetadate
        {
            get
            {
                return fileMetadate;
            }
            set
            {
                fileMetadate = value;
                PropertyChanged?.Invoke(this, new PropertyChangedEventArgs("FileMetadate"));
            }
        }

        //public EventSource EventSource
        //{
        //    get
        //    {
        //        return eventSource;
        //    }
        //    set
        //    {
        //        eventSource = value;
        //        PropertyChanged?.Invoke(this, new PropertyChangedEventArgs("EventSource"));
        //    }
        //}

        public FileInfoWindowViewModel(FileInfoWindow window)
        {
            host = window;
        }

    }
}
