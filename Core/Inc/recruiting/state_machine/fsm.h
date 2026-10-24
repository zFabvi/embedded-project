#ifndef FSM_H
#define FSM_H

/*!
 * \brief Routine for the POST (Power-On Self Test) state.
 * 
 * Checks if the sensors are working properly. Transitions to STATE_WAITING if valid,
 * or to STATE_FATAL if an error is detected.
 * 
 * \param data Pointer to user-defined context data.
 */
void state_post_fn(void *data);

/*!
 * \brief Routine for the Waiting state.
 * 
 * Keeps CLI active and sensor reading off. Drives the board LED (500ms period, 15% duty cycle)
 * and external LED breathing effect (2s period). Transitions to STATE_LISTENING on button press
 * or "run" command.
 * 
 * \param data Pointer to user-defined context data.
 */
void state_waiting_fn(void *data);

/*!
 * \brief Routine for the Listening state.
 * 
 * Disables CLI, reads sensors, and transmits data every 100ms. Drives the board LED (200ms period,
 * 80% duty cycle) and updates the external LED PWM based on sensor difference.
 * Transitions to STATE_WARNING if threshold is exceeded for 5s, or to STATE_PAUSED on button press.
 * 
 * \param data Pointer to user-defined context data.
 */
void state_listening_fn(void *data);

/*!
 * \brief Routine for the Paused state.
 * 
 * Enables CLI and suspends sensor reading. Drives the board LED (2000ms period, 50% duty cycle)
 * and external LED breathing effect. Transitions to STATE_LISTENING on button press or "run" command.
 * 
 * \param data Pointer to user-defined context data.
 */
void state_paused_fn(void *data);

/*!
 * \brief Routine for the Warning state.
 * 
 * Disables CLI and LEDs while keeping sensor reading active. Transmits "WARNING" over serial
 * every 200ms. Transitions to STATE_WAITING if button is held for 2s, or to STATE_FATAL after 10s.
 * 
 * \param data Pointer to user-defined context data.
 */
void state_warning_fn(void *data);

/*!
 * \brief Routine for the Fatal error state.
 * 
 * Disables CLI and sensor reading. Blinks board LED rapidly (40ms period, 50% duty cycle)
 * and transmits "ERROR" messages every 100ms. Requires an MCU reset to exit.
 * 
 * \param data Pointer to user-defined context data.
 */
void state_fatal_fn(void *data);

#endif