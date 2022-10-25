using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Windows.Input;

namespace CustomControls.common.helper
{
    public class DelegateCommand : ICommand
    {
        #region Fields
        // Declare two delegate for delay the Execute and CanExecute Command
        private readonly Action<object> executeCommand;
        private readonly Func<object, bool> canExecuteCommand;

        #endregion // Fields

        #region Constructors
        public DelegateCommand(Action<object> execute) : this(execute, null)
        {
        }

        public DelegateCommand(Action<object> execute, Func<object, bool> canExecute)
        {
            this.executeCommand = execute ?? throw new ArgumentNullException("execute is empty.");
            this.canExecuteCommand = canExecute;
        }
        #endregion // Constructors


        #region ICommand members
        public event EventHandler CanExecuteChanged;

        public bool CanExecute(object parameter)
        {
            return canExecuteCommand != null ? canExecuteCommand(parameter) : true;
        }

        public void Execute(object parameter)
        {
            executeCommand?.Invoke(parameter);
        }

        public void RaiseCanExecuteChanged()
        {
            CanExecuteChanged?.Invoke(this, EventArgs.Empty);
        }


        #endregion // ICommand members
    }
}
