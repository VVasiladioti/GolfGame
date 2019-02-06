#pragma once
#include <vector>
#include <map>

namespace NCL {
	namespace CSC8503 {

		class State; // Predeclare the classes we need
		class StateTransition;

		// Typedefs to make life easier !
		typedef std::multimap<State*, StateTransition*> TransitionContainer;
		typedef TransitionContainer::iterator TransitionIterator;

		class StateMachine	{
		public:
			StateMachine();
			~StateMachine();

			void AddState(State* s);
			void AddTransition(StateTransition* t);

			void Update();

			//virtual bool CanTransition() const = 0;
			State* GetDestinationState() const { return destinationState; }
			State* GetSourceState() const { return sourceState; }

		protected:
			State * activeState;
			std::vector<State*> allStates;
			TransitionContainer allTransitions;

			State* sourceState;
			State* destinationState;
		};
	}
}