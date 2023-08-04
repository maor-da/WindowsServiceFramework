#pragma once
#include <framework.h>

class SimpleService : public Service
{
public:
	using base_t = Service;
	static const wchar_t* service_name;
	SimpleService();
};