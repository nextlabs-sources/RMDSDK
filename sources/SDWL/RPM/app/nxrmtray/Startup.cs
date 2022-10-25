using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using Microsoft.VisualBasic.ApplicationServices;

namespace ServiceManager
{
    /// <summary>
    /// Create single instance application wrapper System.Application using WindowsFormsApplicationBase.
    ///  Note: must set the 'Build Action' of ServiceManagerApp.xaml property as 'Page'.
    /// </summary>
    public class Startup
    {
        [STAThread]
        public static void Main(string[] args)
        {
            SingleInstanceAppWrapper wrapper = new SingleInstanceAppWrapper();
            try
            {
                wrapper.Run(args);
            }
            catch (Exception e)
            {
                Console.WriteLine(e.ToString());
            }
        }
    }

    public class SingleInstanceAppWrapper: WindowsFormsApplicationBase
    {
        private ServiceManagerApp app;
        public SingleInstanceAppWrapper()
        {
            // Enable single instance mode
            this.IsSingleInstance = true;
        }

        protected override bool OnStartup(StartupEventArgs eventArgs)
        {
            app = new ServiceManagerApp();

            //
            // Set shutdown mode as 'OnLastWindowClose' only in order to test conviniently,
            // actually we should set 'OnExplicitShutdown' mode later.
            //
            app.ShutdownMode = System.Windows.ShutdownMode.OnExplicitShutdown; 
            app.InitializeComponent();
            app.Run();

            return false;
        }

        // Callback this when second or more instance get started, and the first is still running,
        // Good Point to handle second command line here
        protected override void OnStartupNextInstance(StartupNextInstanceEventArgs eventArgs)
        {
            if (app != null && eventArgs.CommandLine.Count > 0)
            {
                app.SignalExternalCommandLineArgs(eventArgs.CommandLine);
            }
        }

    }

}
