using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace ServiceManager.rmservmgr.sdk
{
    public class Tenant
    {
        private IntPtr hTenant;
        public Tenant(IntPtr hTenant)
        {
            this.hTenant = hTenant;
        }

        ~Tenant()
        {
            DeleteTenant();
        }


        public IntPtr Handle { get { return hTenant; } }


        public string Name
        {
            get
            {
                string name;
                Boundary.SDK_SDWL_GetTenant(hTenant, out name);
                return name;
            }
        }

        public string RouterURL
        {
            get
            {
                string url;
                Boundary.SDK_SDWL_GetRouterURL(hTenant, out url);
                return url;
            }
        }

        public string RMSURL
        {
            get
            {
                string url;
                Boundary.SDK_SDWL_GetRouterURL(hTenant, out url);
                return url;
            }
        }

        public void DeleteTenant()
        {
            if (hTenant == IntPtr.Zero)
            {
                return;
            }
            // call api
            Boundary.SDK_SDWL_ReleaseTenant(hTenant);
            // set zeor
            hTenant = IntPtr.Zero;
        }
    }
}
