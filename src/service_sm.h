#pragma once

#include <cpp-utils/StateMachine.h>

enum class ServiceStates : uint8_t {
	uninstalled = 0,
	installed,
	running,
	stopped,
	paused,
	COUNT  // the number of states
};

class ServiceStateMachine : public _STATEMACHINE<ServiceStates>
{
public:
	using base_t = _STATEMACHINE<state_t>;

	ServiceStateMachine() : base_t(&TransitionTable[0][0], state_t::uninstalled) {}

private:
	const uint8_t TransitionTable[(uint8_t)state_t::COUNT][(uint8_t)state_t::COUNT] = {
		// clang-format off
		// Desired state
	  // 0  1  2  3  4    // current state
		{1, 1, 0, 0, 0},	 // 0
		{1, 0, 1, 1, 0},	 // 1
		{0, 0, 0, 1, 1},	 // 2
		{1, 0, 1, 0, 0},	 // 3
		{0, 0, 1, 1, 0},	 // 4 LanmanWorkstation service using pause
		// clang-format on
	};
};
