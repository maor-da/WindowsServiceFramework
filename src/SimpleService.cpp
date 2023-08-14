#include "SimpleService.h"

#include "framework.h"

const wchar_t* SimpleService::service_name = L"simple_service";

void __stdcall SimpleServiceMain(DWORD argc, LPWSTR* argv)
{
	SCMDispatcher::instance()->main<SimpleService>(argc, argv);
}

void __stdcall SimpleServiceHandler(DWORD control)
{
	SCMDispatcher::instance()->handler<SimpleService>(control);
}

SimpleService::SimpleService()
{
	cfg.function_main				  = SimpleServiceMain;
	cfg.function_handler			  = SimpleServiceHandler;
	cfg.configuration.lpServiceName	  = service_name;
	cfg.configuration.dwDesiredAccess = SERVICE_ALL_ACCESS;
	cfg.configuration.dwServiceType	  = SERVICE_WIN32_SHARE_PROCESS;
	cfg.configuration.dwStartType	  = SERVICE_DEMAND_START;
	cfg.configuration.dwErrorControl  = SERVICE_ERROR_NORMAL;

	cfg.accepted_controls			  = SERVICE_ACCEPT_STOP; /* | SERVICE_ACCEPT_PAUSE_CONTINUE |
													   SERVICE_ACCEPT_POWEREVENT | SERVICE_ACCEPT_SESSIONCHANGE |
													   SERVICE_ACCEPT_TIMECHANGE*/
}
