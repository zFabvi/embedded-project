/*!
 * \file eagletrt-fsm.h
 * \date 2026-05-14
 * \author Alessandro Giustina [giustinalessandro@gmail.com]
 * \author Alessandro Bridi [ale.bridi15@gmail.com]
 * 
 * \brief Finite State Machine implementation
 */

#ifndef EAGLETRT_FSM_H
#define EAGLETRT_FSM_H

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdbool.h>

/*! 
 * \brief Type definition for a state function 
 *
 * \param data A pointer to user-defined data that can be passed to the state function, it must not contain any pointers to the FSM handler or its members.
 */
typedef void (*state_function)(void *data);

/*! 
 * \brief Type definition for a transition function
 *
 * \param data A pointer to user-defined data that can be passed to the transition function, it must not contain any pointers to the FSM handler or its members.
 * \warning Transition functions must not call fsm_api_* functions.
 *  They are edge actions; request follow-up transitions from a state function.
 */
typedef void (*transition_function)(void *data);

/*!
 * \brief Return codes for FSM operations
 */
enum FSMReturnCode {
    FSM_RC_OK,                 /*!< Operation completed successfully */
    FSM_RC_INVALID_MACHINE,    /*!< The FSM structure is invalid */
    FSM_RC_INVALID_STATE,      /*!< The specified state is invalid */
    FSM_RC_INVALID_TRANSITION, /*!< The specified transition is invalid */
    FSM_RC_NULL_POINTER,       /*!< The function recieved a null pointer*/
};

/*!
 * \brief Structure representing a transition in the FSM
 */
struct Transition {
    uint8_t to;                   /*!< The state ID of the state to which to move*/
    transition_function function; /*!< OPTIONAL: the transition function to run in between the state change*/
};

/*!
 * \brief Structure representing a state in the FSM
 * \warning To avoid null pointer dereferencing please make sure that num_transitions is correct
 * if the array has yet to decay use the macro
 */
struct State {
    uint8_t id;              /*!< The ID of the state*/
    state_function function; /*!< The function to be run at the state*/
    uint8_t next_default;    /*!< The default next state, this is the state to go to if no transition is triggered*/

    const struct Transition *transitions; /*!< The transitions from this state*/
    uint8_t num_transitions;              /*!< The number of transitions from this state*/
};

/*!
 * \brief Structure representing the FSM handler
 * 
 * \warning The machine_states pointer must be statically allocated
 */
struct FSMHandler {
    uint8_t current_state;            /*!< The current state in which the FSM is*/
    volatile uint8_t requested_state; /*!< The requested state to go to at the end of the current state*/

    const struct State *machine_states; /*!< The states of the FSM*/
    uint8_t state_count;                /*!< The number of states in the FSM*/
};

/*!
 * \brief Macro to determine transition array length
 * \warning Please make sure that the array has not decayed to a pointer before using this funciton
 */
#define EAGLETRT_FSM_TRANSITION_LEN(a) (sizeof(a) / sizeof(struct Transition))

#endif /* EAGLETRT_FSM_H */