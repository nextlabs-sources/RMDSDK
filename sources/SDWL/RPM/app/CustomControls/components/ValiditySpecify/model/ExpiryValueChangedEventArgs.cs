using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace CustomControls.components.ValiditySpecify.model
{
    public struct ExpiryValueChangedEventArgs
    {
        public IExpiry Expiry { get; set; }
        public string ExpiryDateValue { get; set; }
    }
}
