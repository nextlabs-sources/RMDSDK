using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace ServiceManager.rmservmgr.ui.windows.chooseServer.model
{
    public class UrlDataModel
    {
        private int id;
        private string url;

        public int ID
        {
            get => id; set => id = value; 
        }
        public string listUrl
        {
            get => url; set => url = value;
        }
        public UrlDataModel(int idPar, string urlPar)
        {
            this.ID = idPar;
            this.listUrl = urlPar;
        }
    }
}
