using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.ComponentModel;
using System.Linq;
using System.Text;

namespace CustomControls.components.TreeView.viewModel
{
    /// <summary>
    /// Base class for all ViewModel classes displayed by TreeViewItems.  
    /// This acts as an adapter between a raw data object and a TreeViewItem.
    /// </summary>
    public class TreeViewItemViewModel : INotifyPropertyChanged
    {
        #region Data
        static readonly TreeViewItemViewModel DummyChild = new TreeViewItemViewModel();

        readonly ObservableCollection<TreeViewItemViewModel> _children;
        readonly TreeViewItemViewModel _parent;

        bool _isEnable = true;
        bool _isExpanded;
        bool _isSelected;
        #endregion // Data


        #region Constructors

        // lazyLoadChildren --- if lazy loading children?
        public TreeViewItemViewModel(TreeViewItemViewModel parent, bool lazyLoadChildren)
        {
            _parent = parent;

            _children = new ObservableCollection<TreeViewItemViewModel>();

            if (lazyLoadChildren)
                _children.Add(DummyChild);
        }

        // This is used to create the DummyChild instance.
        public TreeViewItemViewModel()
        { }

        #endregion // Constructors


        #region Presentation Members


        #region Children
        /// <summary>
        /// Returns the logical child items of this object.
        /// </summary>
        public ObservableCollection<TreeViewItemViewModel> Children
        {
            get { return _children; }
        }
        #endregion // Children



        #region HasLoadedChildren
        /// <summary>
        /// Returns true if this object's Children have not yet been populated.
        /// 如果这个对象的Children还没有获取到，则会有一个 Dummy Child.
        /// </summary>
        public bool HasDummyChild
        {
            get { return this.Children.Count == 1 && this.Children[0] == DummyChild; }
        }
        #endregion // HasLoadedChildren

        // Used to refresh current view model
        public virtual void Refresh() { }

        #region IsEnable
        public bool IsEnable
        {
            get { return _isEnable; }
            set
            {
                if (value != _isEnable)
                {
                    _isEnable = value;
                    this.OnPropertyChanged("IsEnable");
                }
            }
        }
        #endregion
        #region IsExpanded
        /// <summary>
        /// Gets/sets whether the TreeViewItem 
        /// associated with this object is expanded.
        /// </summary>
        public bool IsExpanded
        {
            get { return _isExpanded; }
            set
            {
                if (value != _isExpanded)
                {
                    _isExpanded = value;
                    this.OnPropertyChanged("IsExpanded");
                }

                // Expand all the way up to the root.
                if (_isExpanded && _parent != null)
                    _parent.IsExpanded = true;

                // Lazy load the child items, if necessary.
                if (this.HasDummyChild)
                {
                    this.Children.Remove(DummyChild);
                    this.LoadChildren(true);
                }
                else
                {
                    this.LoadChildren(false);
                }

            }
        }
        #endregion // IsExpanded


        #region IsSelected
        /// <summary>
        /// Gets/sets whether the TreeViewItem 
        /// associated with this object is selected.
        /// </summary>
        public bool IsSelected
        {
            get { return _isSelected; }
            set
            {
                if (value != _isSelected)
                {
                    _isSelected = value;
                    this.OnPropertyChanged("IsSelected");
                }
                if (_isSelected == true)
                {
                    // Lazy load the child items, if necessary.
                    if (this.HasDummyChild)
                    {
                        this.Children.Remove(DummyChild);
                        this.LoadChildren(true);
                    }
                }
            }
        }
        #endregion // IsSelected


        #region LoadChildren
        /// <summary>
        /// Invoked when the child items need to be loaded on demand.
        /// Subclasses can override this to populate the Children collection.
        /// </summary>
        protected virtual void LoadChildren(bool bFirstLoad)
        {
        }
        #endregion // LoadChildren


        #region Parent
        public TreeViewItemViewModel Parent
        {
            get { return _parent; }
        }
        #endregion // Parent


        #endregion // Presentation Members


        #region INotifyPropertyChanged Members
        public event PropertyChangedEventHandler PropertyChanged;
        protected virtual void OnPropertyChanged(string propertyName)
        {
            if (this.PropertyChanged != null)
                this.PropertyChanged(this, new PropertyChangedEventArgs(propertyName));
        }

        #endregion // INotifyPropertyChanged Members
    }
}
