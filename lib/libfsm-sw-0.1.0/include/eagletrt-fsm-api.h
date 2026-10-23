/*!
 * \file eagletrt-fsm-api.h
 * \date 2026-05-14
 * \author Alessandro Giustina [giustinalessandro@gmail.com]
 * \author Alessandro Bridi [ale.bridi15@gmail.com]
 * 
 * \brief Finite State Machine public facing API
 */

#ifndef EAGLETRT_FSM_API_H
#define EAGLETRT_FSM_API_H

#include "eagletrt-fsm.h"

/*!
 * \brief Initialization function for the FSM module
 * 
 * \attention the state_list pointer must be statically allocated and NEVER go out of scope as it is used directly and not copied
 * 
 * \param handler The pointer to the FSMHandler of the instance
 * \param state_list The pointer to the populated list of states
 * \param state_count The size of the state list
 * \param initial_state The initial state of the FSM
 * 
 * \retval FSM_RC_OK Initialization successful
 * \retval FSM_RC_INVALID_MACHINE The FSM structure is invalid 
 * \retval FSM_RC_INVALID_STATE The given state is invalid
 * \retval FSM_RC_NULL_POINTER The function recieved a null pointer
 */
enum FSMReturnCode fsm_api_init(struct FSMHandler *handler, const struct State *state_list, uint8_t state_count, uint8_t initial_state);

/*!
 * \brief Routine for the FSM
 *
 * \param handler The pointer to the FSMHandler of the instance
 * \param data The data to be passed to the state and to the transition function
 * 
 * \retval FSM_RC_OK The routine completed succesfully
 * \retval FSM_RC_INVALID_TRANSITION The module attempted an invalid transition
 * \retval FSM_RC_NULL_POINTER The function recieved a null pointer
 */
enum FSMReturnCode fsm_api_run_state(struct FSMHandler *handler, void *data);

/*!
 * \brief Function to trigger a change of state in the FSM, this will take place at the 
 * end of the current state execution, if the transition is valid
 * 
 * \param handler The pointer to the FSMHandler of the instance
 * \param state_id The ID of the state to transition to
 * 
 * \retval FSM_RC_OK The next state has been updated successfully
 * \retval FSM_RC_INVALID_TRANSITION The asked transition is not supported
 * \retval FSM_RC_INVALID_STATE The given state is invalid
 * \retval FSM_RC_NULL_POINTER The function recieved a null pointer
 */
enum FSMReturnCode fsm_api_transition_to(struct FSMHandler *handler, uint8_t state_id);

/*!
 * \brief Function that returns the ID of the current state
 * 
 * \param handler The pointer to the FSMHandler of the instance
 * 
 * \returns uint8_t the ID of the current state, 0 also if the handler is NULL
 */
uint8_t fsm_api_get_state(const struct FSMHandler *handler);

#endif /* EAGLETRT_FSM_API_H */