#include "stdafx.h"
#include "PDP/pdp.h"
#include "RPM\drvcore_mgr.h"
#include "SDLAPI_forInstaller.h"

SDWLResult SDWLibSetRPMServiceStop(bool enable)
{
    drvcore_mgr mgr;

    return mgr.set_service_stop_no_security(enable);
}

SDWLResult SDWLibStopRPMService()
{
    drvcore_mgr mgr;

    return mgr.stop_service_no_security();
}

SDWLResult SDWLibStopPDP()
{
    NX::SDWPDP pdp;

    return pdp.StopPDPMan();
}
