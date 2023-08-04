#pragma once
#include <Windows.h>
template <typename I, typename R>
class WinAPI
{
public:
	// RAII(bool (*init)(), void (*release)()) : m_release(release)
	//{
	//	m_success = init();
	//}

	WinAPI(I init, R release) : m_LambdaRelease(release)
	{
		m_Success = init(m_NtStatus);
	}

	~WinAPI()
	{
		if (m_Success) {
			m_LambdaRelease();
		}
	}

	bool success()
	{
		return m_Success;
	}

	bool error()
	{
		return !m_Success;
	}

	NTSTATUS nt_status()
	{
		return m_NtStatus;
	}

private:
	bool m_Success = false;
	R m_LambdaRelease;
	NTSTATUS m_NtStatus = 0;
};

template <class T>
class TempValue
{
public:
	TempValue(T& value, T temp) : storage(value), old_value(value)
	{
		storage = temp;
	}

	~TempValue()
	{
		storage = old_value;
	}

	T get_old_value()
	{
		return old_value;
	}

private:
	T& storage;
	T old_value;
};

#ifndef THREAD_LOCAL_GAURD

#define THREAD_LOCAL_GAURD(ret)                                                            \
	thread_local static bool __THREAD_LOCAL_GAURD = false;                                 \
	auto tmp__THREAD_LOCAL_GAURD				  = TempValue(__THREAD_LOCAL_GAURD, true); \
	if (tmp__THREAD_LOCAL_GAURD.get_old_value()) {                                         \
		return ret;                                                                        \
	}
#endif	// !THREAD_LOCAL_GAURD
