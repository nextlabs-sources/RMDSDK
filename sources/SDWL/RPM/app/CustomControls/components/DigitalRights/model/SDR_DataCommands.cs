using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Windows.Input;

namespace CustomControls.components.DigitalRights.model
{
    /// <summary>
    /// SelectDigitalRights.xaml  DataCommands
    /// </summary>
    public class SDR_DataCommands
    {
        private static RoutedCommand changeWaterMark;
        private static RoutedCommand changeExpiry;
        static SDR_DataCommands()
        {
            changeWaterMark = new RoutedCommand(
              "ChangeWaterMark", typeof(SDR_DataCommands));

            changeExpiry = new RoutedCommand(
              "ChangeExpiry", typeof(SDR_DataCommands));
        }
        /// <summary>
        /// SelectDigitalRights.xaml change waterMark button command
        /// </summary>
        public static RoutedCommand ChangeWaterMark
        {
            get { return changeWaterMark; }
        }
        /// <summary>
        /// SelectDigitalRights.xaml change expiry button command
        /// </summary>
        public static RoutedCommand ChangeExpiry
        {
            get { return changeExpiry; }
        }
      
    }
}
