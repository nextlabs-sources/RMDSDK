using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;

namespace CustomControls.pages.CentralPolicy.model
{
    /// <summary>
    /// Classification
    /// </summary>
    public struct Classification
    {
        public string name;
        public bool isMultiSelect;
        public bool isMandatory;

        /// <summary>
        /// string is  labels value, bool is defult selected
        /// </summary>
        public Dictionary<String, Boolean> labels;  // <LabelValue,isDefault>
    }
}
