using CustomControls.components.TreeView.model;
using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Linq;
using System.Text;
using System.Windows.Media.Imaging;

namespace CustomControls.components.TreeView.viewModel
{
    /// <summary>
    /// TreeViewItem ViewModel
    /// </summary>
    public class TreeNodeViewModel : TreeViewItemViewModel
    {
        private Node node { get; }

        /// <summary>
        /// TreeNodeViewModel constructor
        /// </summary>
        /// <param name="node"></param>
        /// <param name="parent"></param>
        /// <param name="isLazyLoad">if true,Children will have a dummyChild</param>
        public TreeNodeViewModel(Node node, TreeNodeViewModel parent = null, bool isLazyLoad = false) : base(parent, isLazyLoad)
        {
            this.node = node;

            if (node.Children != null && node.Children.Count > 0)
            {
                foreach (var item in node.Children)
                {
                    base.Children.Add(new TreeNodeViewModel(item, this, false));
                }
            }

            if (node.IsFirstSelected)
            {
                this.IsExpanded = true;
                this.IsSelected = true;
            }
        }

        public string RootName { get => node.RootName; }
        public string OwnerId { get => node.OwnerId; }
        public string Name { get => node.Name; }
        public BitmapImage Icon { get => node.Icon; }
        public string PathId { get => node.PathId; }
        public string PathDisplay { get => node.PathDisplay; }

    }
}
