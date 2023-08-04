#pragma once
#include <framework.h>

class KernelDriverSvc : public Service
{
public:
	using base_t = Service;
	static const wchar_t* service_name;
	KernelDriverSvc();
};