using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace nxcommondialog.helper
{
    class CommonUtils
    {
        public static Bitmap GetFileIcon(string filePath, string iconPath)
        {
            try
            {
                if (!string.IsNullOrEmpty(iconPath) && System.IO.File.Exists(iconPath))
                {
                    return new Bitmap(iconPath);
                }
                else
                {
                    // try to extract associated icon by file path
                    Icon fileicon = System.Drawing.Icon.ExtractAssociatedIcon(filePath);
                    if (fileicon != null)
                    {
                        return fileicon.ToBitmap();
                    }
                }
            }
            catch (Exception e)
            {
                Trace.WriteLine(e.Message);
            }

            // Using default icon
            Bitmap bitmap = nxcommondialog.Properties.Resources.commonicon;
            return bitmap;
        }
    }
}
