#include "framework.h"

#include <memory>
#include <mutex>

static std::mutex _mtx;	 // limit scope
std::shared_ptr<SCMDispatcher> SCMDispatcher::m_Instance = nullptr;

std::shared_ptr<SCMDispatcher> SCMDispatcher::instance()
{
	std::lock_guard<std::mutex> g(_mtx);
	if (!m_Instance) {
		m_Instance.reset(new SCMDispatcher);
	}

	return m_Instance;
}

SC_HANDLE SCMDispatcher::scm_handle()
{
	return m_SCM;
}

void SCMDispatcher::run_all()
{
	for (auto& svc : m_ServicesMap) {
		svc.second->Service::run();
	}
}

void SCMDispatcher::stop_all()
{
	for (auto& svc : m_ServicesMap) {
		svc.second->Service::stop();
	}
}

void SCMDispatcher::install_all()
{
	for (auto& svc : m_ServicesMap) {
		svc.second->install();
	}
}

void SCMDispatcher::uninstall_all()
{
	for (auto& svc : m_ServicesMap) {
		svc.second->uninstall();
	}
}

void SCMDispatcher::dispatch()
{
	if (m_ServicesMap.size() == 0) {
		return;	 // No service was registered
	}

	auto table = std::make_unique<SERVICE_TABLE_ENTRYW[]>(m_ServicesMap.size() + 1);

	int i	   = 0;
	for (auto& svc : m_ServicesMap) {
		table[i++] = {const_cast<LPWSTR>(svc.first.data()), svc.second->cfg.function_main};
	}

	if (!StartServiceCtrlDispatcherW(table.get())) {
		// log.fatal("Service is forcely closed");
	}
}

inline SCMDispatcher::SCMDispatcher()  // has to be singleton since the SCM call to static main function
{
#ifdef _DEBUG
	while (!IsDebuggerPresent()) {
		Sleep(1000);
	}
#endif	// _DEBUG

	m_SCM = OpenSCManagerW(NULL,					// local machine
						   NULL,					// local database
						   SC_MANAGER_ALL_ACCESS);	// access required
}
