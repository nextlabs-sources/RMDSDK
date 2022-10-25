using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace CustomControls.components.ValiditySpecify.model
{

    public class NeverExpireImpl : INeverExpire
    {
        public int GetOpetion()
        {
            return 0;
        }
    }

    public class RelativeImpl : IRelative
    {
        private int years;
        private int months;
        private int weeks;
        private int days;

        public RelativeImpl(int years, int months, int weeks, int days)
        {
            this.years = years;
            this.months = months;
            this.weeks = weeks;
            this.days = days;
        }
        public int GetOpetion()
        {
            return 1;
        }
        public int GetYears()
        {
            return years;
        }

        public int GetMonths()
        {
            return months;
        }
        public int GetDays()
        {
            return days;
        }
        public int GetWeeks()
        {
            return weeks;
        }
    }

    public class AbsoluteImpl : IAbsolute
    {
        private long enddate;
        public AbsoluteImpl(long end)
        {
            this.enddate = end;
        }
        public int GetOpetion()
        {
            return 2;
        }
        public long EndDate()
        {
            return enddate;
        }
    }
    public class RangeImpl : IRange
    {
        private long startdate;
        private long enddate;

        public RangeImpl(long start, long end)
        {
            this.startdate = start;
            this.enddate = end;
        }

        public int GetOpetion()
        {
            return 3;
        }
        public long StartDate()
        {
            return startdate;
        }
        public long EndDate()
        {
            return enddate;
        }
    }
}
