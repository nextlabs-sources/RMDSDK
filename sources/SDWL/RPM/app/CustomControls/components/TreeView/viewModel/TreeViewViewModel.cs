using CustomControls.components.TreeView.model;
using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Linq;
using System.Text;

namespace CustomControls.components.TreeView.viewModel
{
    /// <summary>
    /// ViewModel for TreeViewComponent.xaml
    /// </summary>
    public class TreeViewViewModel
    {
        private List<Node> nodes = new List<Node>();
        private ObservableCollection<TreeNodeViewModel> treeNodeVMList = new ObservableCollection<TreeNodeViewModel>();
        
        public TreeViewViewModel() { }

        /// <summary>
        /// To init TreeView data
        /// </summary>
        public List<Node> Nodes { get => nodes; set { nodes = value; AddTreeNodeViewModel(value); } }
        private void AddTreeNodeViewModel(List<Node> treeNodes)
        {
            if (treeNodes == null || treeNodes.Count == 0)
            {
                return;
            }
            treeNodeVMList.Clear();
            foreach (var node in treeNodes)
            {
                // in order to first select node, can't use laze load
                treeNodeVMList.Add(new TreeNodeViewModel(node, null, false));
            }
        }

        /// <summary>
        /// TreeView ItemsSource
        /// </summary>
        public ObservableCollection<TreeNodeViewModel> TreeNodeVMList
        {
            get { return treeNodeVMList; }
        }

    }
}
