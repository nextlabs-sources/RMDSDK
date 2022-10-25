using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace CustomControls.windows.fileInfo.helper
{
    public class Utils
    {
        public static long DateTimeToTimestamp(DateTime time)
        {
            DateTime startDateTime = TimeZone.CurrentTimeZone.ToLocalTime(new DateTime(1970, 1, 1, 0, 0, 0));
            return Convert.ToInt64((time - startDateTime).TotalMilliseconds);
        }
        public static string TimestampToDateTime(long time)
        {
            DateTime startDateTime = TimeZone.CurrentTimeZone.ToLocalTime(new DateTime(1970, 1, 1, 0, 0, 0));
            DateTime newTime = startDateTime.AddMilliseconds(time);
            return newTime.ToString("MMMM dd, yyyy");
        }

        public static string FormatExpiration(Expiration expiration)
        {
            string result = string.Empty;
            ExpiryType operationType = expiration.type;
            if (operationType != ExpiryType.NEVER_EXPIRE && Utils.DateTimeToTimestamp(DateTime.Now) > expiration.End)
            {
                result = "Expired";
                return result;
            }
            switch (operationType)
            {
                case ExpiryType.NEVER_EXPIRE:
                    result = "Never expire";
                    break;
                case ExpiryType.RELATIVE_EXPIRE:
                    string dateRelativeS = Utils.TimestampToDateTime(expiration.Start);
                    string dateRelativeE = Utils.TimestampToDateTime(expiration.End);
                    result = "Until " + dateRelativeE;
                    break;
                case ExpiryType.ABSOLUTE_EXPIRE:
                    string dateAbsoluteS = Utils.TimestampToDateTime(expiration.Start);
                    string dateAbsoluteE = Utils.TimestampToDateTime(expiration.End);
                    result = "Until " + dateAbsoluteE;
                    break;
                case ExpiryType.RANGE_EXPIRE:
                    string dateRangeS = Utils.TimestampToDateTime(expiration.Start);
                    string dateRangeE = Utils.TimestampToDateTime(expiration.End);
                    result = dateRangeS + " To " + dateRangeE;
                    break;
            }

            return result;
        }


    }
}
