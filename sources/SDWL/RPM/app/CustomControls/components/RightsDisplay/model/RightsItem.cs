using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Windows.Media.Imaging;

namespace CustomControls.components.RightsDisplay.model
{
    public class RightsItem
    {
        private BitmapImage icon;
        private string rights;

        public RightsItem(BitmapImage icon, string rights)
        {
            this.icon = icon;
            this.rights = rights;
        }

        public BitmapImage Icon
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
