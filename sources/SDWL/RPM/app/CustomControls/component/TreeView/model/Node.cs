using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Windows.Media.Imaging;

namespace CustomControls.components.TreeView.model
{
    public class Node
    {
        private string rootName;
        private string ownerId;
        private string name;
        private BitmapImage icon;
        private List<Node> children;
        private string pathId;
        private string pathDisplay;
        private bool isFirstSelected;

        /// <summary>
        /// Tree Node model
        /// </summary>
        /// <param name="rootName">root node name, like 'MySpace', 'Project', 'WorkSpace'</param>
        /// <param name="nodeOwnerId">if node is 'MySpace', 'Project' id = -1; node is 'MyVault','MyDrive','WorkSpace' id = 0; node is project id = projectId</param>
        /// <param name="nodeName"></param>
        /// <param name="nodeIcon"></param>
        /// <param name="child"></param>
        /// <param name="nodePathId">current node pathId, like "/folder/"; "oneProject/oneFolder/"</param>
        /// <param name="nodePathDisplay">display currently selected path,like "WorkSpace:/allentest/1/"; "Project: oneProject/oneFolder/"</param>
        /// <param name="firstSelect">use for first open treeview,defult selected node</param>
        public Node(string nodeRootName, string nodeOwnerId, string nodeName, BitmapImage nodeIcon, List<Node> child, string nodePathId = "/", string nodePathDisplay = "/", bool firstSelect=false)
        {
            rootName = nodeRootName;
            ownerId = nodeOwnerId;
            name = nodeName;
            icon = nodeIcon;
            children = child;
            pathId = nodePathId;
            pathDisplay = nodePathDisplay;
            isFirstSelected = firstSelect;
        }

        public string RootName { get => rootName; }
        public string OwnerId { get => ownerId; }
        public string Name { get => name;  }
        public BitmapImage Icon { get => icon; }
        public List<Node> Children { get => children; }
        public string PathId { get => pathId; }
        public string PathDisplay { get => pathDisplay; }
        public bool IsFirstSelected { get => isFirstSelected; set => isFirstSelected = value; }
        
    }

}
