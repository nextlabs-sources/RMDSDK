using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace CommonDialog.sdk
{
    // 4 level design:
    //  Compoent | Domain | Method | ErrorCode
    // i.e:   RmRestApi:MyVault:UploadFile:403
    enum ExceptionComponent
    {
        UNDEFINED = 0X001,
        // Logic error senarios: 
        //  json parse error,
        LogicError = 0x002,
        /////////
        RMSDK = 0x100,
        RMSDK_REST_API = 0X101,
        /////////
        UI = 0X200,
        /////////
        NETWORK_IO = 0X300,
        /////////
        DATABASE = 0X400,
        DATABASE_CACHE = 0X401,
        DATABASE_ENGINE = 0X402,
        /////////
        FEATURE_PROVIDER = 0X500,
        /////////
        FILE_SYSTEM = 0X600,


    }


    class SkydrmException : Exception
    {
        private ExceptionComponent component;

        public SkydrmException() : this("unknown", ExceptionComponent.UNDEFINED)
        {
        }

        public SkydrmException(string message) :
            this(message, ExceptionComponent.UNDEFINED)
        {
        }

        public SkydrmException(string message, ExceptionComponent component) : base(message)
        {
            this.component = component;
        }

        public ExceptionComponent Component { get => component; }

        // network io exception is a very common for our local mode app,
        // maybe this is a good hint for ui-codes to notify user, "you encountered a network problem"
        // by far, rm-sdk may be easy to face it when talking with server through tcp/ip
        public virtual bool IsNetworkIoException { get => component == ExceptionComponent.NETWORK_IO; }

        // designed for error handler to show the error details
        // require each derived must format it self one
        public virtual string DisplayMessage()
        {
            return Message;
        }

        // desgined for log to record detailed messages
        // require each derived must format it self one
        public virtual string LogUsedMessage()
        {
            return "Domain:" + component.ToString() + " Msg:" + Message;
        }

    }
}
