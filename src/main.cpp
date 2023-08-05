#include "KernelDriverSvc.h"
#include "SimpleService.h"
#include "framework.h"

int wmain(DWORD argc, LPWSTR* argv)
{
	auto disp = SCMDispatcher::instance();
	disp->add<SimpleService>();
	disp->add<KernelDriverSvc>();

	if (lstrcmpi(argv[1], L"install") == 0) {
		disp->install_all();
	}
	else if (lstrcmpi(argv[1], L"uninstall") == 0) {
		disp->uninstall_all();
	}
	else {
		disp->dispatch();
	}

	return 0;
}
