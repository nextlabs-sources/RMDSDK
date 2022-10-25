using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Input;

namespace ServiceManager.rmservmgr.ui.windows.serviceManager.helper
{
    public class DelegateCommand<T> : ICommand
    {
        #region Fields
        // Declare two delegate for delay the Execute and CanExecute Command
        private readonly Action<T> executeCommand;
        private readonly Func<T, bool> canExecuteCommand;

        #endregion // Fields

        #region Constructors
        public DelegateCommand(Action<T> execute) : this(execute, null)
        {
        }

        public DelegateCommand(Action<T> execute, Func<T, bool> canExecute)
        {
            this.executeCommand = execute ?? throw new ArgumentNullException("execute is empty.");
            this.canExecuteCommand = canExecute;
        }
        #endregion // Constructors


        #region ICommand members
        public event EventHandler CanExecuteChanged;

        public bool CanExecute(object parameter)
        {
            return canExecuteCommand != null ? canExecuteCommand((T)parameter) : true;
        }

        public void Execute(object parameter)
        {
            executeCommand?.Invoke((T)parameter);
        }

        public void RaiseCanExecuteChanged()
        {
            CanExecuteChanged?.Invoke(this, EventArgs.Empty);
        }

        #endregion // ICommand members
    }
}
