using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Input;
using System.Windows.Interactivity;

namespace ServiceManager.rmservmgr.ui.windows.serviceManager.helper
{
    /// <summary>
    /// Custom Evnet Command with EventArgs
    /// </summary>
    class MyEventCommand : TriggerAction<DependencyObject>
    {
        public ICommand Command
        {
            get { return (ICommand)GetValue(CommandProperty); }
            set { SetValue(CommandProperty, value); }
        }

        // Using a DependencyProperty as the backing store for MsgName.  This enables animation, styling, binding, etc...
        public static readonly DependencyProperty CommandProperty =
            DependencyProperty.Register("Command", typeof(ICommand), typeof(MyEventCommand), new PropertyMetadata(null));

        /// <summary>
        /// Binding command parameter, in default, is Event parameters if don't bind           
        /// </summary>
        public object CommandParateter
        {
            get { return (object)GetValue(CommandParateterProperty); }
            set { SetValue(CommandParateterProperty, value); }
        }

        // Using a DependencyProperty as the backing store for CommandParateter.  This enables animation, styling, binding, etc...
        public static readonly DependencyProperty CommandParateterProperty =
            DependencyProperty.Register("CommandParateter", typeof(object), typeof(MyEventCommand), new PropertyMetadata(null));

        /// <summary>
        /// If don't bind,then is event parameters in default; or else, is binded parameters.
        /// </summary>
        /// <param name="parameter"></param>
        protected override void Invoke(object parameter)
        {
            if (CommandParateter != null)
                parameter = CommandParateter;
            var cmd = Command;
            if (cmd != null)
                cmd.Execute(parameter);
        }
    }
}
