using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Windows.Controls;
using System.Windows.Controls.Primitives;

namespace CustomControls.components.CentralPolicy.model
{
    internal class LabelUIElement
    {
        private TextBlock title;
        private List<ToggleButton> lables;
        private bool isMandatory = false;
        private bool isMultiSelect = false;

        internal LabelUIElement()
        { }

        public TextBlock Title { get => title; set => title = value; }
        public List<ToggleButton> Lables { get => lables; set => lables = value; }
        public bool IsMandatory { get => isMandatory; set => isMandatory = value; }
        public bool IsMultiSelect { get => isMultiSelect; set => isMultiSelect = value; }
    }
}
