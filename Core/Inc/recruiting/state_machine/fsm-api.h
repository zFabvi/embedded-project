#ifndef FSM_API_H
#define FSM_API_H

#include "eagletrt-fsm-api.h"

/*!
 * \brief System state identifiers.
 * \warning Must start from 0 and be strictly consecutive to match the state array indices.
 */
enum SystemStateID {
    STATE_POST = 0,
    STATE_WAITING,
    STATE_LISTENING,
    STATE_PAUSED,
    STATE_WARNING,
    STATE_FATAL,
    STATE_COUNT
};

/*!
 * \brief Initializes the system Finite State Machine.
 * 
 * Sets up the internal FSM handler with the state table and starts in STATE_POST.
 * 
 * \retval FSM_RC_OK Initialization completed successfully.
 * \retval FSM_RC_INVALID_MACHINE The state table configuration is invalid.
 * \retval FSM_RC_INVALID_STATE The initial state is out of bounds.
 * \retval FSM_RC_NULL_POINTER A null pointer was passed to the initializer.
 */
enum FSMReturnCode system_fsm_init(void);

/*!
 * \brief Executes one cycle of the current state routine and handles transitions.
 * 
 * \param data Pointer to user-defined context data passed to state and transition functions.
 * 
 * \retval FSM_RC_OK State routine completed successfully.
 * \retval FSM_RC_INVALID_TRANSITION An unsupported state transition was requested.
 * \retval FSM_RC_NULL_POINTER The internal handler or state table is null.
 */
enum FSMReturnCode system_fsm_run(void *data);

/*!
 * \brief Requests a transition to a new state at the end of the current state execution.
 * 
 * \param next_state The target state ID to transition to.
 * 
 * \retval FSM_RC_OK Transition request registered successfully.
 * \retval FSM_RC_INVALID_TRANSITION Transition to the specified state is not allowed from the current state.
 * \retval FSM_RC_INVALID_STATE The requested state ID is invalid.
 * \retval FSM_RC_NULL_POINTER The internal handler is null.
 */
enum FSMReturnCode system_fsm_transition_to(enum SystemStateID next_state);

/*!
 * \brief Retrieves the currently active state ID.
 * 
 * \return uint8_t The ID of the current state.
 */
uint8_t system_fsm_get_state(void);

#endif