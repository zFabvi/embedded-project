/*!
 * \file fsm-example.c
 * \date 2026-05-14
 * \author Alessandro Giustina [giustinalessandro@gmail.com]
 *
 * \brief Simple example of the FSM module usage.
 * \details In this example we initialize an FSM with four states and step through
 *          them using both automatic (non-repeat) and triggered (repeat) transitions.
 */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include "eagletrt-fsm-api.h"

void routine_idle(void *data) {
    printf("Idle");
}

void routine_working(void *data) {
    printf("Working");
}

void routine_fault(void *data) {
    printf("Fault handler");
}

void routine_done(void *data) {
    printf("Done");
}

void transition_idle_to_working(void *data) {
    printf("  [transition: Idle -> Working]");
}

void transition_working_to_fault(void *data) {
    printf("  [transition: Working -> Fault]");
}

void transition_fault_to_idle(void *data) {
    printf("  [transition: Fault -> Idle]");
}

void transition_working_to_done(void *data) {
    printf("  [transition: Working -> Done]");
}

void transition_done_to_idle(void *data) {
    printf("  [transition: Done -> Idle]");
}

enum StateID {
    STATE_IDLE,
    STATE_WORKING,
    STATE_FAULT,
    STATE_DONE,
    STATE_COUNT
};

struct Transition idle_transitions[] = {
    { .to = STATE_WORKING, .function = transition_idle_to_working },
};
struct Transition working_transitions[] = {
    { .to = STATE_FAULT, .function = transition_working_to_fault },
    { .to = STATE_DONE, .function = transition_working_to_done },
};
struct Transition fault_transitions[] = {
    { .to = STATE_IDLE, .function = transition_fault_to_idle },
};
struct Transition done_transitions[] = {
    { .to = STATE_IDLE, .function = transition_done_to_idle },
};

struct State states[] = {
    {
        .id = STATE_IDLE,
        .function = routine_idle,
        .next_default = STATE_IDLE, // This is a repeat state
        .transitions = idle_transitions,
        .num_transitions = EAGLETRT_FSM_TRANSITION_LEN(idle_transitions),
    },
    {
        .id = STATE_WORKING,
        .function = routine_working,
        .next_default = STATE_WORKING, // This is a repeat state
        .transitions = working_transitions,
        .num_transitions = EAGLETRT_FSM_TRANSITION_LEN(working_transitions),
    },
    {
        .id = STATE_FAULT,
        .function = routine_fault,
        .next_default = STATE_IDLE,
        .transitions = fault_transitions,
        .num_transitions = EAGLETRT_FSM_TRANSITION_LEN(fault_transitions),
    },
    {
        .id = STATE_DONE,
        .function = routine_done,
        .next_default = STATE_IDLE,
        .transitions = done_transitions,
        .num_transitions = EAGLETRT_FSM_TRANSITION_LEN(done_transitions),
    },
};

int main(void) {
    struct FSMHandler handler;
    fsm_api_init(&handler, states, STATE_COUNT, STATE_IDLE);

    /*
     * Step through a few ticks in IDLE, then trigger transitions.
     * At tick 3 trigger WORKING, at tick 6 trigger FAULT.
     * FAULT is non-repeat so it auto-advances back to IDLE on the next tick.
     */

    for (uint32_t i = 0; i <= 8; i++) {

        if (i == 3) {
            fsm_api_transition_to(&handler, STATE_WORKING);
        }

        if (i == 6) {
            fsm_api_transition_to(&handler, STATE_FAULT);
        }

        printf("Tick %2u  ->  ", i);
        fsm_api_run_state(&handler, NULL);
        printf("\n");
    }

    /*
     * Now trigger WORKING again, then DONE.
     * DONE is non-repeat so it auto-advances back to IDLE on the next tick.
     */

    for (uint32_t i = 9; i <= 15; i++) {

        if (i == 10) {
            fsm_api_transition_to(&handler, STATE_WORKING);
        }

        if (i == 13) {
            fsm_api_transition_to(&handler, STATE_DONE);
        }

        printf("Tick %2u  ->  ", i);
        fsm_api_run_state(&handler, NULL);
        printf("\n");
    }

    return 0;
}