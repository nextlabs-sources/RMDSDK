using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace CustomControls.pages.CentralPolicy.model
{
    public struct SelectClassificationEventArgs
    {
        public bool IsValid { get; set; }
        public Dictionary<string, List<string>> KeyValues { get; set; }
    }
}
