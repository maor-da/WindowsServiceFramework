#pragma once
#include <Windows.h>
#include <stdint.h>

#include <concepts>
#include <map>
#include <memory>
#include <string>

#include "Service.h"

template <typename T>
concept is_wstr_name = std::same_as<T, const wchar_t*>;

template <class T>
concept is_service_t = std::is_base_of_v<Service, T> && requires { T::service_name; } &&
					   std::same_as<decltype(T::service_name), const wchar_t*>;

class SCMDispatcher
{
public:
	static std::shared_ptr<SCMDispatcher> instance();

	SC_HANDLE scm_handle();

	// Derived class must have `static const wchar_t* service_name` member
	template <is_service_t T>
	void add()
	{
		// Insert if not exist
		m_ServicesMap.emplace(T::service_name, std::make_shared<T>());
	}

	template <is_service_t T>
	void remove()
	{
		// Remove if exist
		m_ServicesMap.erase(T::service_name);
	}

	template <is_service_t T>
	std::shared_ptr<Service> get()
	{
		if (m_ServicesMap.contains(T::service_name)) {
			return m_ServicesMap[T::service_name];
		}

		return {};
	}

	template <is_service_t T>
	bool run()
	{
		if (m_ServicesMap.contains(T::service_name)) {
			return m_ServicesMap[T::service_name]->run();
		}
		return false;
	}

	template <is_service_t T>
	void stop()
	{
		if (m_ServicesMap.contains(T::service_name)) {
			return m_ServicesMap[T::service_name]->Service::stop();	 // Run the base
		}
		return false;
	}

	template <is_service_t T>
	void pause()
	{
		if (m_ServicesMap.contains(T::service_name)) {
			return m_ServicesMap[T::service_name]->Service::pause();  // Run the base
		}
		return false;
	}

	template <is_service_t T>
	void install()
	{
		if (m_ServicesMap.contains(T::service_name)) {
			return m_ServicesMap[T::service_name]->install();  // Run virtual
		}
		return false;
	}

	template <is_service_t T>
	void uninstall()
	{
		if (m_ServicesMap.contains(T::service_name)) {
			return m_ServicesMap[T::service_name]->uninstall();	 // Run virtual
		}
		return false;
	}

	template <is_service_t T>
	void main(DWORD argc, LPWSTR* argv)
	{
		if (m_ServicesMap.contains(T::service_name)) {
			m_ServicesMap[T::service_name]->main(argc, argv);  // Run virtual
		}
	}

	template <is_service_t T>
	void handler(DWORD control)
	{
		if (m_ServicesMap.contains(T::service_name)) {
			m_ServicesMap[T::service_name]->handler(control);  // Run virtual
		}
	}

	// start all installed services
	void run_all();

	// stop all running services
	void stop_all();

	// install all uninstalled services
	void install_all();

	// uninstall all installed services
	void uninstall_all();

	void dispatch();

private:
	SCMDispatcher();  // The only place that open SCM handle (except utilities)

	static std::shared_ptr<SCMDispatcher> m_Instance;
	std::map<std::wstring_view, std::shared_ptr<Service>> m_ServicesMap;
	SC_HANDLE m_SCM = NULL;
};
