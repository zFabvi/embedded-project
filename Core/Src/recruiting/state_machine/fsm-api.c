#include "fsm-api.h"
#include "fsm.h"

/*! \brief Static instance of the FSM handler. */
static struct FSMHandler fsm_handler;

/*! \brief Allowed transitions from STATE_POST. */
static const struct Transition post_transitions[] = {
    { .to = STATE_WAITING, .function = NULL },
    { .to = STATE_FATAL,   .function = NULL }
};

/*! \brief Allowed transitions from STATE_WAITING. */
static const struct Transition waiting_transitions[] = {
    { .to = STATE_LISTENING, .function = NULL },
    { .to = STATE_FATAL,     .function = NULL }
};

/*! \brief Allowed transitions from STATE_LISTENING. */
static const struct Transition listening_transitions[] = {
    { .to = STATE_WARNING, .function = NULL },
    { .to = STATE_PAUSED,  .function = NULL },
    { .to = STATE_FATAL,   .function = NULL }
};

/*! \brief Allowed transitions from STATE_PAUSED. */
static const struct Transition paused_transitions[] = {
    { .to = STATE_LISTENING, .function = NULL },
    { .to = STATE_FATAL,     .function = NULL }
};

/*! \brief Allowed transitions from STATE_WARNING. */
static const struct Transition warning_transitions[] = {
    { .to = STATE_WAITING, .function = NULL },
    { .to = STATE_FATAL,   .function = NULL }
};


/*!
 * \brief Master state table defining all states, routines, defaults, and transition links.
 */
static const struct State system_states[STATE_COUNT] = {
    [STATE_POST] = {
        .id = STATE_POST,
        .function = state_post_fn,
        .next_default = STATE_POST,
        .transitions = post_transitions,
        .num_transitions = EAGLETRT_FSM_TRANSITION_LEN(post_transitions)
    },
    [STATE_WAITING] = {
        .id = STATE_WAITING,
        .function = state_waiting_fn,
        .next_default = STATE_WAITING,
        .transitions = waiting_transitions,
        .num_transitions = EAGLETRT_FSM_TRANSITION_LEN(waiting_transitions)
    },
    [STATE_LISTENING] = {
        .id = STATE_LISTENING,
        .function = state_listening_fn,
        .next_default = STATE_LISTENING,
        .transitions = listening_transitions,
        .num_transitions = EAGLETRT_FSM_TRANSITION_LEN(listening_transitions)
    },
    [STATE_PAUSED] = {
        .id = STATE_PAUSED,
        .function = state_paused_fn,
        .next_default = STATE_PAUSED,
        .transitions = paused_transitions,
        .num_transitions = EAGLETRT_FSM_TRANSITION_LEN(paused_transitions)
    },
    [STATE_WARNING] = {
        .id = STATE_WARNING,
        .function = state_warning_fn,
        .next_default = STATE_WARNING,
        .transitions = warning_transitions,
        .num_transitions = EAGLETRT_FSM_TRANSITION_LEN(warning_transitions)
    },
    [STATE_FATAL] = {
        .id = STATE_FATAL,
        .function = state_fatal_fn,
        .next_default = STATE_FATAL,
        .transitions = NULL,
        .num_transitions = 0
    }
};

void state_post_fn(void *data) {
    (void)data;
}

void state_waiting_fn(void *data) {
    (void)data;
}

void state_listening_fn(void *data) {
    (void)data;
}

void state_paused_fn(void *data) {
    (void)data;
}

void state_warning_fn(void *data) {
    (void)data;
}

void state_fatal_fn(void *data) {
    (void)data;
}

enum FSMReturnCode system_fsm_init(void) {
    return fsm_api_init(&fsm_handler, system_states, STATE_COUNT, STATE_POST);
}

enum FSMReturnCode system_fsm_run(void *data) {
    return fsm_api_run_state(&fsm_handler, data);
}

enum FSMReturnCode system_fsm_transition_to(enum SystemStateID next_state) {
    return fsm_api_transition_to(&fsm_handler, (uint8_t)next_state);
}

uint8_t system_fsm_get_state(void) {
    return fsm_api_get_state(&fsm_handler);
}