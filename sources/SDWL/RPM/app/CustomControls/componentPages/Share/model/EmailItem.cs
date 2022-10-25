using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace CustomControls.pages.Share
{
    public class EmailItem
    {
        private string emails;
        private EmailStatus emailStatus;

        public EmailItem(string emails, EmailStatus emailStatus)
        {
            this.emails = emails;
            this.emailStatus = emailStatus;
        }

        public string Emails
        {
            get { return emails; }
            set { emails = value; }
        }
        public EmailStatus EmailStatus
        {
            get { return emailStatus; }
            set { emailStatus = value; }
        }
    }
}
