#include "Service.h"

#include <filesystem>

#include "RAII.h"
#include "framework.h"

void Service::update_status(DWORD state, DWORD exitCode, DWORD waitHint)
{
	cfg.status.dwCurrentState  = state;
	cfg.status.dwWin32ExitCode = exitCode;
	cfg.status.dwWaitHint	   = waitHint;

	if (state == SERVICE_START_PENDING) {
		cfg.status.dwControlsAccepted = 0;	// disable controls
	} else {
		cfg.status.dwControlsAccepted = acceptedControls;
	}

	if ((state == SERVICE_RUNNING) || (state == SERVICE_STOPPED)) {
		m_Checkpoint = 0;
	} else {
		// report the progress for the current pending state
		m_Checkpoint++;
	}
	cfg.status.dwCheckPoint = m_Checkpoint;

	if (!SetServiceStatus(cfg.status_handle, &cfg.status)) {
		// log.warning("SetServiceStatus failed (%X)", GetLastError());
	}
}

bool Service::is_installed()
{
	if (m_Handle || s.get_state() != decltype(s)::state_t::uninstalled) {
		return true;
	}

	try {
		auto t	 = s.transit(decltype(s)::state_t::installed);
		auto svc = get_handle();

		if (!svc) {
			if (GetLastError() == ERROR_SERVICE_DOES_NOT_EXIST) {
				return false;
			}
			return false;  // return c++23 expected for the error
		}
		t.commit();
		return true;
	} catch (...) {
	}

	return false;
}

SC_HANDLE Service::get_handle()
{
	if (m_Handle) {
		return m_Handle;
	}

	SC_HANDLE scm = SCMDispatcher::instance()->scm_handle();
	if (scm) {
		m_Handle = OpenServiceW(scm,							  // SCM database
								cfg.configuration.lpServiceName,  // name of service
								SERVICE_ALL_ACCESS);			  // full access
	}

	return m_Handle;
}

bool Service::start()
{
	THREAD_LOCAL_GAURD(true);
	try {
		auto t = s.transit(decltype(s)::state_t::running);
		update_status(SERVICE_START_PENDING, NO_ERROR, 3000);
		if (!start()) {	 // Call user override if exist
			return false;
		}
		update_status(SERVICE_RUNNING, NO_ERROR, 3000);
		t.commit();
		return true;
	} catch (...) {
	}

	return false;
}

bool Service::stop()
{
	THREAD_LOCAL_GAURD(true);
	try {
		auto t = s.transit(decltype(s)::state_t::stopped);
		update_status(SERVICE_STOP_PENDING, NO_ERROR, 3000);
		if (!stop()) {	// Call user override if exist
			return false;
		}
		update_status(SERVICE_STOPPED, NO_ERROR, 3000);
		t.commit();
		return true;
	} catch (...) {
	}

	return false;
}

bool Service::pause()
{
	THREAD_LOCAL_GAURD(true);
	try {
		auto t = s.transit(decltype(s)::state_t::paused);
		update_status(SERVICE_PAUSE_PENDING, NO_ERROR, 3000);
		if (!pause()) {	 // Call user override if exist
			return false;
		}
		update_status(SERVICE_RUNNING, NO_ERROR, 3000);
		t.commit();
		return true;
	} catch (...) {
	}

	return false;
}

bool Service::resume()
{
	THREAD_LOCAL_GAURD(true);
	try {
		auto t = s.transit(decltype(s)::state_t::running);
		update_status(SERVICE_CONTINUE_PENDING, NO_ERROR, 3000);
		if (!resume()) {  // Call user override if exist
			return false;
		}
		update_status(SERVICE_RUNNING, NO_ERROR, 3000);
		t.commit();
		return true;
	} catch (...) {
	}

	return false;
}

bool Service::run()
{
	if (!is_installed()) {
		return false;
	}

	switch (s.get_state()) {
		case decltype(s)::state_t::installed:
			return Service::start();
		case decltype(s)::state_t::paused:
			return Service::resume();

		default:
			return false;
	}
}

std::filesystem::path GetServicePath()
{
	wchar_t modulePath[MAX_PATH] = {0};
	auto module					 = GetModuleHandleA(nullptr);
	auto length					 = GetModuleFileNameW(module, modulePath, MAX_PATH);
	if (GetLastError()) {
		return {};
	}
	return modulePath;
}

bool Service::install()
{
	try {
		auto t = s.transit(decltype(s)::state_t::installed);
		if (is_installed()) {
			t.commit();
			return true;
		}

		do {
			SC_HANDLE scm = SCMDispatcher::instance()->scm_handle();
			if (!scm) {
				break;
			}

			if (!cfg.configuration.lpBinaryPathName) {
				m_BinaryPath					   = GetServicePath().native();
				cfg.configuration.lpBinaryPathName = m_BinaryPath.c_str();
			}

			// Check if executable exist
			auto fileHandle = CreateFileW(cfg.configuration.lpBinaryPathName,
										  GENERIC_READ,
										  FILE_SHARE_READ,
										  NULL,
										  OPEN_EXISTING,
										  FILE_ATTRIBUTE_NORMAL,
										  NULL);

			if (fileHandle == INVALID_HANDLE_VALUE) {
				// The file isn't exist
				// log.error("File not exist: %ls\n", cfg.configuration.lpBinaryPathName);
				break;
			}

			// The file exist
			CloseHandle(fileHandle);

			m_Handle = CreateServiceW(scm,									 // SCM database
									  cfg.configuration.lpServiceName,		 // name of service
									  cfg.configuration.lpDisplayName,		 // service name to display
									  cfg.configuration.dwDesiredAccess,	 // desired access
									  cfg.configuration.dwServiceType,		 // service type
									  cfg.configuration.dwStartType,		 // start type
									  cfg.configuration.dwErrorControl,		 // error control type
									  cfg.configuration.lpBinaryPathName,	 // path to service's binary
									  cfg.configuration.lpLoadOrderGroup,	 // load ordering group
									  cfg.configuration.lpdwTagId,			 // tag identifier
									  cfg.configuration.lpDependencies,		 // dependencies
									  cfg.configuration.lpServiceStartName,	 // LocalSystem account
									  cfg.configuration.lpPassword);		 // password

			if (!m_Handle) {
				// log.error("CreateServiceW failed (%d)\n", GetLastError());
				break;
			}

			t.commit();
			return true;

		} while (false);

	} catch (...) {
	}

	return false;

	return false;
}

bool Service::uninstall()
{
	try {
		auto t = s.transit(decltype(s)::state_t::uninstalled);
		if (!is_installed()) {
			return true;
		}

		if (DeleteService(m_Handle)) {
			t.commit();
			return true;
		}

	} catch (...) {
	}

	return false;
}

void __stdcall Service::main(DWORD argc, LPWSTR* argv)
{
	cfg.status_handle = RegisterServiceCtrlHandlerW(cfg.configuration.lpServiceName, cfg.function_handler);

	if (!cfg.status_handle) {
		// log.error("RegisterServiceCtrlHandlerW failed");
		return;
	}

	cfg.status.dwServiceSpecificExitCode = 0;
	cfg.status.dwServiceType			 = cfg.configuration.dwServiceType;

	// consider to use conditinal variable or waitonaddress
	cfg.stop_event = CreateEventW(NULL,	  // default security attributes
								  TRUE,	  // manual reset event
								  FALSE,  // not signaled
								  NULL);  // no name

	if (cfg.stop_event == NULL) {
		update_status(SERVICE_STOPPED, GetLastError(), 0);
		return;
	}

	// at this point we registered to the SCM with handler and created a stop event

	Service::run();
}

void __stdcall Service::handler(DWORD control)
{
	switch (control) {
		case SERVICE_CONTROL_STOP:
			// log.debug("stop signal");
			SetEvent(cfg.stop_event);
			update_status(cfg.status.dwCurrentState, NO_ERROR, 0);

			break;
		case SERVICE_CONTROL_PAUSE:
			// log.debug("pause signal");
			break;
		case SERVICE_CONTROL_POWEREVENT:
			// log.debug("power event signal");
			break;
		case SERVICE_CONTROL_SESSIONCHANGE:
			// log.debug("session change signal");
			break;

		default:
			break;
	}
}
