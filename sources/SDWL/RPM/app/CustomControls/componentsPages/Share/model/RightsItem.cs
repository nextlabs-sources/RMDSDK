using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace CustomControls.pages.Share
{
    public class RightsItem
    {
        private string icon;
        private string rights;

        public RightsItem(string icon, string rights)
        {
            this.icon = icon;
            this.rights = rights;
        }

        public string Icon
        {
            get { return icon; }
            set { icon = value; }
        }
        public string Rights
        {
            get { return rights; }
            set { rights = value; }
        }
    }
}
