using System;
using System.Diagnostics;
using System.IO;
using System.IO.Pipes;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace ServiceManager.rmservmgr.common.components
{
    public class NamedPipeClient
    {
        // Named pipe should associate user session id for supporting multiple users login at the same time by RDP(Windows server).
        private static string PipeName = "544336d7-9086-4369-a9d0-3691ea290376" + "_sid_" + Process.GetCurrentProcess().SessionId.ToString();
      //  private static string UUID = "8986207c-5161-436a-abe9-dfc365c89820";

        public static void Start(string jsonContent)
        {
            NamedPipeClientStream pipeClient = null;
            try
            {
                pipeClient = new NamedPipeClientStream(".", PipeName,
                    PipeDirection.InOut, PipeOptions.None);

                //  string receivedStr = string.Empty;

                Console.WriteLine("Connecting to server...\n");
                pipeClient.Connect(5000);

                StreamReader stringReader = new StreamReader(pipeClient);
                StreamWriter streamWriter = new StreamWriter(pipeClient);
                streamWriter.AutoFlush = true;

                // write data
                streamWriter.WriteLine(jsonContent);

                //  receivedStr = stringReader.ReadLine();
                //// Validate the server's signature string
                //if (receivedStr.Equals(UUID, StringComparison.CurrentCultureIgnoreCase))
                //{
                //    // write data
                //    streamWriter.WriteLine(jsonContent);
                //}
                //else
                //{
                //    Console.WriteLine("Server could not be verified.");
                //}
            }
            catch (Exception ex)
            {
                Console.WriteLine(ex.Message.ToString());
            }
            finally
            {
                if (null != pipeClient)
                {
                    if (pipeClient.IsConnected)
                    {
                        pipeClient.Flush();
                        pipeClient.Dispose();
                    }
                }
            }
        }

        public enum Intent
        {
            LeaveCopy,
            SyncFileAfterEdit,
        }

        public class Bundle<T>
        {
            public Intent Intent;
            public T obj;
        }
    }
}
