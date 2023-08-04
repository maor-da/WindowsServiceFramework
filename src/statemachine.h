#pragma once

#include <stdint.h>

#include <cassert>
#include <mutex>

template <class T>
class _STATEMACHINE
{
public:
	using state_t = T;
	class Transition;

	_STATEMACHINE(const uint8_t* TransitionTable, state_t startState)
		: m_CurrentState(startState),
		  m_NextState(startState),
		  m_TransitionTable(TransitionTable){

		  };

	virtual ~_STATEMACHINE() {}

	inline state_t get_state()
	{
		return m_CurrentState;
	}

	bool validate_transition(state_t newState)
	{
		uint32_t transition = (uint8_t)m_CurrentState * (uint8_t)state_t::COUNT + (uint8_t)newState;
		return *(m_TransitionTable + transition);
	}

	// Cannot start transition within transition, the transition finished
	// when the transition object is out of scope.
	Transition transit(state_t state)
	{
		// same thread reenter will cause a deadlock
		if (in_transition && Transition::in_transition) {
			throw "Transition within transition";
		}
		return Transition(*this, state);
	}

protected:
	class Transition : private std::lock_guard<std::mutex>
	{
	public:
		using base_t = std::lock_guard<std::mutex>;

		Transition(_STATEMACHINE& sm, state_t newState) : base_t(sm.m_Mtx), m_SM(sm)
		{
			if (!m_SM.validate_transition(newState)) {
				throw "Invalid transition";
			}
			in_transition	   = true;
			m_SM.in_transition = true;
			// printf("start transition\n");
			m_SM.m_NextState = newState;
		}
		~Transition()
		{
			if (m_Commited) {
				// printf("Finish transition\n");
				m_SM.m_CurrentState = m_SM.m_NextState;
			} else {
				// printf("Revert transition\n");
				m_SM.m_NextState = m_SM.m_CurrentState;
			}

			in_transition	   = false;
			m_SM.in_transition = false;
		}

		void commit()
		{
			m_Commited = true;
		}

	private:
		bool m_Commited = false;
		_STATEMACHINE& m_SM;
		thread_local static inline bool in_transition = false;
		friend _STATEMACHINE;
	};

private:
	std::mutex m_Mtx;
	state_t m_CurrentState;
	state_t m_NextState;
	const uint8_t* const m_TransitionTable;

	// instance indicator
	bool in_transition = false;
	friend Transition;
};
