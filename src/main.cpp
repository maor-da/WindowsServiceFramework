#include "KernelDriverSvc.h"
#include "SimpleService.h"
#include "framework.h"

void main()
{
	auto disp = SCMDispatcher::instance();
	disp->add<SimpleService>();
	disp->add<KernelDriverSvc>();
	disp->install_all();
	disp->dispatch();

	// disp->uninstall_all();
}
