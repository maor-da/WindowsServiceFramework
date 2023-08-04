#include "KernelDriverSvc.h"


const wchar_t* KernelDriverSvc::service_name = L"simple_driver";

void __stdcall KernelDriverSvceMain(DWORD argc, LPWSTR* argv)
{
	SCMDispatcher::instance()->main<KernelDriverSvc>(argc, argv);
}

void __stdcall KernelDriverSvcHandler(DWORD control)
{
	SCMDispatcher::instance()->handler<KernelDriverSvc>(control);
}

KernelDriverSvc::KernelDriverSvc()
{
	cfg.function_main				  = KernelDriverSvceMain;
	cfg.function_handler			  = KernelDriverSvcHandler;
	cfg.configuration.lpServiceName	  = service_name;
	cfg.configuration.dwDesiredAccess = SERVICE_ALL_ACCESS;
	cfg.configuration.dwServiceType	  = SERVICE_WIN32_SHARE_PROCESS;
	cfg.configuration.dwStartType	  = SERVICE_DEMAND_START;
	cfg.configuration.dwErrorControl  = SERVICE_ERROR_NORMAL;

	acceptedControls				  = SERVICE_ACCEPT_STOP; /* | SERVICE_ACCEPT_PAUSE_CONTINUE |
														   SERVICE_ACCEPT_POWEREVENT | SERVICE_ACCEPT_SESSIONCHANGE |
														   SERVICE_ACCEPT_TIMECHANGE*/
}