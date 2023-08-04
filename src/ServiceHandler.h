#pragma once
#include <Windows.h>

#include <memory>
#include <string>

// Handle a service which is owned by the SCM
// therefore we can't garentee it's status
class ServiceHandler
{
public:
	ServiceHandler()
	{
		m_SCM = OpenSCManagerW(NULL,					// local machine
							   NULL,					// local database
							   SC_MANAGER_ALL_ACCESS);	// access required

		if (!m_SCM) {
			// log.error("OpenSCManagerW failed (%d)\n", GetLastError());
			throw("Failed to open SCM");
		}
	}

	ServiceHandler(std::wstring_view name) : ServiceHandler()
	{
		m_ServiceHandle = OpenServiceW(m_SCM,				 // SCM database
									   name.data(),			 // name of service
									   SERVICE_ALL_ACCESS);	 // full access

		if (m_ServiceHandle == NULL) {
			// log.error("OpenService failed (%d)\n", GetLastError());
			CloseServiceHandle(m_SCM);
			throw("Failed to open service");
		}
	}

	~ServiceHandler()
	{
		if (m_SCM) {
			CloseServiceHandle(m_SCM);
		}

		if (m_ServiceHandle) {
			CloseServiceHandle(m_ServiceHandle);
		}
	}

	SERVICE_STATUS_PROCESS get_status()
	{
		SERVICE_STATUS_PROCESS status;
		DWORD size;

		do {
			if (!m_ServiceHandle) {
				// log.error("Service handle hasn't initialized\n");
				break;
			}

			if (!QueryServiceStatusEx(m_ServiceHandle,				   // handle to service
									  SC_STATUS_PROCESS_INFO,		   // information level
									  (LPBYTE)&status,				   // address of structure
									  sizeof(SERVICE_STATUS_PROCESS),  // size of structure
									  &size))						   // size needed if buffer is too small
			{
				// log.error("QueryServiceStatusEx failed (%d)\n", GetLastError());
				break;
			}

			return status;

		} while (false);

		return {};
	}

	bool stop_dependents()
	{
		std::unique_ptr<uint8_t[]> dependencies = nullptr;
		DWORD size								= 0;
		DWORD servicesCount						= 0;

		do {
			if (EnumDependentServicesW(m_ServiceHandle,
									   SERVICE_ACTIVE,
									   (LPENUM_SERVICE_STATUSW)dependencies.get(),
									   0,
									   &size,
									   &servicesCount)) {
				// There is no dependent services
				return true;
			} else if (GetLastError() != ERROR_MORE_DATA) {
				// log.error("EnumDependentServicesW failed (%d)\n", GetLastError());
				break;
			}

			dependencies = std::make_unique<uint8_t[]>(size);

			if (!EnumDependentServicesW(m_ServiceHandle,
										SERVICE_ACTIVE,
										(LPENUM_SERVICE_STATUSW)dependencies.get(),
										size,
										&size,
										&servicesCount)) {
				// log.error("Second EnumDependentServicesW failed (%d)\n", GetLastError());
				break;
			}

			LPENUM_SERVICE_STATUSW ess = reinterpret_cast<LPENUM_SERVICE_STATUSW>(dependencies.get());
			for (DWORD i = 0; i < servicesCount; i++) {
				try {
					ServiceHandler depService(ess[i].lpServiceName);
					depService.stop();
				} catch (const std::exception& e) {
					// log.error("depService exception: %s", e.what());
					return false;
				}
			}

			return true;

		} while (false);

		return false;
	}

	bool stop()
	{
		SERVICE_STATUS_PROCESS status;
		SERVICE_STATUS_PROCESS ssp;

		do {
			status = get_status();
			if (!status.dwCurrentState) {
				// log.error("Cannnot get status\n");
				break;
			}

			if (status.dwCurrentState == SERVICE_STOPPED) {
				// log.error("Service already stopped\n");
				return true;
			}

			// If its pending to stop wait for it
			if (status.dwCurrentState == SERVICE_STOP_PENDING) {
				if (!wait_pending(SERVICE_STOP_PENDING)) {
					// log.error("Wait for service to stop failed\n");
				} else
					status = get_status();
				if (status.dwCurrentState == SERVICE_STOPPED) {
					return true;
				}
				// log.error("Service didn't stopped correctly, try again\n");
			}

			if (!stop_dependents()) {
				// log.error("Failed to stop dependencies\n");
				break;
			}

			if (!ControlService(m_ServiceHandle, SERVICE_CONTROL_STOP, (LPSERVICE_STATUS)&ssp)) {
				// log.error("ControlService stop failed (%d)\n", GetLastError());
				break;
			}

			if (!wait_pending(SERVICE_STOP_PENDING)) {
				// log.error("Wait for service to start failed\n");
				break;
			}

			status = get_status();
			if (status.dwCurrentState != SERVICE_STOPPED) {
				// log.error("Cannot start the service correctly (%d)\n", status.dwWin32ExitCode);
				break;
			}

			return true;

		} while (false);

		return false;
	}

	bool start()
	{
		SERVICE_STATUS_PROCESS status;

		do {
			status = get_status();
			if (!status.dwCurrentState) {
				// log.error("Cannnot get status\n");
				break;
			}

			// If its pending to stop wait for it
			if (status.dwCurrentState == SERVICE_STOP_PENDING) {
				if (!wait_pending(SERVICE_STOP_PENDING)) {
					// log.error("Wait for service to stop failed\n");
					break;
				}
			}

			if (status.dwCurrentState != SERVICE_STOPPED) {
				// log.error("Service already running\n");
				return true;  // could be paused, but it's already started
			}

			if (!StartServiceW(m_ServiceHandle,	 // handle to service
							   0,				 // number of arguments
							   NULL))			 // no arguments
			{
				// log.error("StartService failed (%d)\n", GetLastError());
				break;
			}

			if (!wait_pending(SERVICE_START_PENDING)) {
				// log.error("Wait for service to start failed\n");
				break;
			}

			status = get_status();
			if (status.dwCurrentState != SERVICE_RUNNING) {
				// log.error("Cannot start the service correctly (%d)\n", status.dwWin32ExitCode);
				break;
			}

			return true;

		} while (false);

		return false;
	}

	bool pause() {}

	bool run() {}

	bool restart()
	{
		return (stop() && start());
	}
	bool set_dacl() {}
	bool get_dacl() {}

	bool open(std::wstring_view name)
	{
		if (m_ServiceHandle) {
			close();
		}

		m_ServiceHandle = OpenServiceW(m_SCM,				 // SCM database
									   name.data(),			 // name of service
									   SERVICE_ALL_ACCESS);	 // full access

		if (m_ServiceHandle == NULL) {
			// log.error("OpenService failed (%d)\n", GetLastError());
		}
	}

	bool close()
	{
		if (m_ServiceHandle) {
			CloseServiceHandle(m_ServiceHandle);
			m_ServiceHandle = NULL;
		}
	}

private:
	SC_HANDLE m_SCM			  = NULL;
	SC_HANDLE m_ServiceHandle = NULL;

	bool wait_pending(DWORD state)
	{
		SERVICE_STATUS_PROCESS status = get_status();
		auto startTick				  = GetTickCount64();
		auto checkPoint				  = status.dwCheckPoint;
		DWORD wait					  = 0;

		switch (state) {
			case SERVICE_START_PENDING:
			case SERVICE_STOP_PENDING:
			case SERVICE_CONTINUE_PENDING:
			case SERVICE_PAUSE_PENDING:
				while (status.dwCurrentState == state) {
					// Bound the 10th of hint between 1 - 10 seconds
					wait = status.dwWaitHint / 10;
					if (wait > 10000) {
						wait = 10000;
					} else if (wait < 1000) {
						wait = 1000;
					}

					Sleep(wait);

					status = get_status();
					if (!status.dwCurrentState) {
						// log.error("Cannnot get status\n");
						break;
					}

					if (status.dwCurrentState != state) {
						return true;
					}

					if (status.dwCheckPoint > checkPoint) {
						startTick  = GetTickCount64();
						checkPoint = status.dwCheckPoint;
					} else {
						if (GetTickCount64() - startTick > status.dwWaitHint) {
							// log.error("Timeout waiting\n");
							break;
						}
					}
				}

				return true;

			default:
				break;
		}

		return false;
	}
};