#pragma once
#include <Windows.h>
#include <stdint.h>

#include <string>

#include "service_sm.h"

class SCMDispatcher;

class Service
{
public:
	struct config {
		LPSERVICE_MAIN_FUNCTIONW function_main;
		LPHANDLER_FUNCTION function_handler;
		SERVICE_STATUS status;
		SERVICE_STATUS_HANDLE status_handle;
		HANDLE stop_event;
		struct {
			LPCWSTR lpServiceName;
			LPCWSTR lpDisplayName;
			DWORD dwDesiredAccess;
			DWORD dwServiceType;
			DWORD dwStartType;
			DWORD dwErrorControl;
			LPCWSTR lpBinaryPathName;
			LPCWSTR lpLoadOrderGroup;
			LPDWORD lpdwTagId;
			LPCWSTR lpDependencies;
			LPCWSTR lpServiceStartName;
			LPCWSTR lpPassword;
		} configuration;
	};

	virtual bool run();
	virtual bool stop();

protected:	// access by derived
	DWORD acceptedControls = 0;
	config cfg{ 0 };
	ServiceStateMachine s;

private:
	DWORD m_Checkpoint = 0;
	SC_HANDLE m_Handle = NULL;
	std::wstring m_BinaryPath;

	void update_status(DWORD state, DWORD exitCode, DWORD waitHint);
	bool is_installed();
	SC_HANDLE get_handle();
	void idle();

	// derived can override without calling it directly
	// base will call it
	virtual bool start();
	virtual bool pause();
	virtual bool resume();

	// overriding this will not called through the base
	virtual bool install();
	virtual bool uninstall();
	virtual void __stdcall main(DWORD argc, LPWSTR* argv);
	virtual void __stdcall handler(DWORD control);

	friend SCMDispatcher;
};
