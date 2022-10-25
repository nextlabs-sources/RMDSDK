using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace CustomControls.common.helper
{
    public class DateTimeHelper
    {
        public static string DateTimeFormat = "MMMM dd, yyyy";

        public static long DateTimeToTimestamp(DateTime time)
        {
            DateTime startDateTime = TimeZone.CurrentTimeZone.ToLocalTime(new DateTime(1970, 1, 1, 0, 0, 0));
            return Convert.ToInt64((time - startDateTime).TotalMilliseconds);
        }
        public static string TimestampToDateTime(long time)
        {
            DateTime startDateTime = TimeZone.CurrentTimeZone.ToLocalTime(new DateTime(1970, 1, 1, 0, 0, 0));
            DateTime newTime = startDateTime.AddMilliseconds(time);
            return newTime.ToString(DateTimeFormat);
        }
    }

    internal class WaterMarkHelper
    {
        private const string DOLLAR_USER = "$(User)";
        private const string DOLLAR_BREAK = "$(Break)";
        private const string DOLLAR_DATE = "$(Date)";
        private const string DOLLAR_TIME = "$(Time)";

        internal static void ConvertWatermark2DisplayStyle(string value, ref StringBuilder sb)
        {
            // value = " aa$(user)bb$(tmie)cc$(date)dd$(break)eee"

            if (string.IsNullOrEmpty(value))
            {
                return;
            }

            char[] array = value.ToCharArray();
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


                    sb.Append(value.Substring(0, beginIndex));


                    // judge if is preset
                    string subStr = value.Substring(beginIndex, endIndex - beginIndex + 1);

                    if (subStr.Equals(DOLLAR_USER))
                    {
                        //sb.Append(" ");
                        sb.Append(ReplaceDollar(DOLLAR_USER));
                        sb.Append(" ");
                    }
                    else if (subStr.Equals(DOLLAR_BREAK))
                    {
                        sb.Append(ReplaceDollar(DOLLAR_BREAK));
                    }
                    else if (subStr.Equals(DOLLAR_DATE))
                    {
                        //sb.Append(" ");
                        sb.Append(ReplaceDollar(DOLLAR_DATE));
                        sb.Append(" ");
                    }
                    else if (subStr.Equals(DOLLAR_TIME))
                    {
                        //sb.Append(" ");
                        sb.Append(ReplaceDollar(DOLLAR_TIME));
                        sb.Append(" ");
                    }
                    else
                    {
                        sb.Append(subStr);
                    }

                    // quit
                    break;
                }
            }

            if (beginIndex == -1 || endIndex == -1 || beginIndex > endIndex) // have not preset
            {
                sb.Append(value);

            }
            else if (beginIndex < endIndex)
            {
                if (endIndex + 1 < value.Length)
                {
                    // Converter the remaining by recursive
                    ConvertWatermark2DisplayStyle(value.Substring(endIndex + 1), ref sb);
                }
            }

        }

        private static string ReplaceDollar(string dollarStr)
        {
            string ret = "";
            switch (dollarStr)
            {
                case DOLLAR_USER:
                    ret = "test@email.com";
                    break;
                case DOLLAR_DATE:
                    ret = DateTime.Now.ToString("dd MMMM yyyy");
                    break;
                case DOLLAR_TIME:
                    ret = DateTime.Now.ToString("hh:mm");
                    break;
                case DOLLAR_BREAK:
                    ret = " ";
                    break;
                default:
                    break;
            }

            return ret;
        }
    }

    public class NameColorHelper
    {
        public static string SelectionBackgroundColor(string name)
        {
            if (string.IsNullOrEmpty(name))
            {
                return "#9D9FA2";
            }
            switch (name.Substring(0, 1).ToUpper())
            {
                case "A":
                    return "#DD212B";

                case "B":
                    return "#FDCB8A";

                case "C":
                    return "#98C44A";

                case "D":
                    return "#1A5279";

                case "E":
                    return "#EF6645";

                case "F":
                    return "#72CAC1";

                case "G":
                    return "#B7DCAF";

                case "H":
                    return "#705A9E";

                case "I":
                    return "#FCDA04";

                case "J":
                    return "#ED1D7C";

                case "K":
                    return "#F7AAA5";

                case "L":
                    return "#4AB9E6";

                case "M":
                    return "#603A18";

                case "N":
                    return "#88B8BC";

                case "O":
                    return "#ECA81E";

                case "P":
                    return "#DAACD0";

                case "Q":
                    return "#6D6E73";

                case "R":
                    return "#9D9FA2";

                case "S":
                    return "#B5E3EE";

                case "T":
                    return "#90633D";

                case "U":
                    return "#BDAE9E";

                case "V":
                    return "#C8B58E";

                case "W":
                    return "#F8BDD2";

                case "X":
                    return "#FED968";

                case "Y":
                    return "#F69679";

                case "Z":
                    return "#EE6769";

                case "0":
                    return "#D3E050";

                case "1":
                    return "#D8EBD5";

                case "2":
                    return "#F27EA9";

                case "3":
                    return "#1782C0";

                case "4":
                    return "#CDECF9";

                case "5":
                    return "#FDE9E6";

                case "6":
                    return "#FCED95";

                case "7":
                    return "#F99D21";

                case "8":
                    return "#F9A85D";

                case "9":
                    return "#BCE2D7";

                default:
                    return "#333333";
            }
        }

        public static string SelectionTextColor(string name)
        {
            if (string.IsNullOrEmpty(name))
            {
                return "#ffffff";
            }
            switch (name.Substring(0, 1).ToUpper())
            {
                case "A":
                    return "#ffffff";

                case "B":
                    return "#8F9394";

                case "C":
                    return "#ffffff";

                case "D":
                    return "#ffffff";

                case "E":
                    return "#ffffff";

                case "F":
                    return "#ffffff";

                case "G":
                    return "#8F9394";

                case "H":
                    return "#ffffff";

                case "I":
                    return "#8F9394";

                case "J":
                    return "#ffffff";

                case "K":
                    return "#ffffff";

                case "L":
                    return "#ffffff";

                case "M":
                    return "#ffffff";

                case "N":
                    return "#ffffff";

                case "O":
                    return "#ffffff";

                case "P":
                    return "#ffffff";

                case "Q":
                    return "#ffffff";

                case "R":
                    return "#ffffff";

                case "S":
                    return "#ffffff";


                case "T":
                    return "#ffffff";


                case "U":
                    return "ffffff";


                case "V":
                    return "#ffffff";


                case "W":
                    return "#ffffff";


                case "X":
                    return "#8F9394";


                case "Y":
                    return "#ffffff";


                case "Z":
                    return "#ffffff";


                case "0":
                    return "#8F9394";


                case "1":
                    return "#8F9394";


                case "2":
                    return "#ffffff";


                case "3":
                    return "#ffffff";

                case "4":
                    return "#8F9394";


                case "5":
                    return "#8F9394";


                case "6":
                    return "#8F9394";

                case "7":
                    return "#ffffff";

                case "8":
                    return "#ffffff";

                case "9":
                    return "#8F9394";

                default:
                    return "#ffffff";

            }
        }

    }

}
