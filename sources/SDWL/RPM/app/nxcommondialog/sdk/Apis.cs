using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace CommonDialog.sdk
{
    class Apis
    {
        public static uint Version
        {
            get { return Boundary.GetSDKVersion(); }
        }

        public static void SdkLibInit()
        {
            Boundary.SdkLibInit();
        }

        public static void SdkLibCleanup()
        {
            Boundary.SdkLibCleanup();
        }

        public static Session CreateSession(string TempPath)
        {
            IntPtr hSession = IntPtr.Zero;
            uint rt = Boundary.CreateSDKSession(TempPath, out hSession);
            if (rt != 0)
            {
                ExceptionFactory.BuildThenThrow("CreateSession", rt);
            }
            return new Session(hSession);
        }

        /// <summary>
        /// This used for local mainWindow and viewer to get user.
        /// </summary>
        public static bool GetCurrentLoggedInUser(out Session Session)
        {
            try
            {
                IntPtr hSession = IntPtr.Zero;
                IntPtr hUser = IntPtr.Zero;
                uint rt = Boundary.GetCurrentLoggedInUser(out hSession, out hUser);

                if (hUser == IntPtr.Zero) // User not logged in.
                {
                    Session = new Session(hSession);
                }
                else // User has logged in.
                {
                    User user = new User(hUser);
                    Session = new Session(hSession, user);
                }

                return rt == 0;
            }
            catch (Exception)
            {
                Session = null;
            }

            return false;
        }

        public static bool WaitInstanceInitFinish()
        {
            try
            {
                uint rt = Boundary.WaitInstanceInitFinish();
                return rt == 0;
            }
            catch (Exception)
            {
            }
            return false;
        }
    }
}
