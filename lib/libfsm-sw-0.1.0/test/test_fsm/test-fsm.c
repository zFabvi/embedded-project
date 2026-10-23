/*!
 * \file test-fsm.c
 * \date 2026-05-14
 * \author Alessandro Giustina [giustinalessandro@gmail.com]
 *
 * \brief Unit tests for the FSM
 */

#include "unity.h"
#include "eagletrt-fsm-api.h"
#include "fff.h"
DEFINE_FFF_GLOBALS;

FAKE_VOID_FUNC(routine_0, void *);
FAKE_VOID_FUNC(routine_1, void *);
FAKE_VOID_FUNC(routine_2, void *);
FAKE_VOID_FUNC(routine_3, void *);

FAKE_VOID_FUNC(transition_0_1, void *);
FAKE_VOID_FUNC(transition_2_3, void *);

enum StatesID {
    STATE_0,
    STATE_1,
    STATE_2,
    STATE_3,
    STATE_COUNT
};

struct Transition state_0_transitions[] = {
    {
        .function = transition_0_1,
        .to = STATE_1,
    }
};
struct Transition state_1_transitions[] = {
    {
        .function = NULL,
        .to = STATE_2,
    },
    {
        .function = NULL,
        .to = STATE_3,
    }
};
struct Transition state_2_transitions[] = {
    {
        .function = transition_2_3,
        .to = STATE_3,
    }
};
struct State default_states[] = {
    {
        .id = STATE_0,
        .function = routine_0,
        .next_default = STATE_1,
        .transitions = state_0_transitions,
        .num_transitions = EAGLETRT_FSM_TRANSITION_LEN(state_0_transitions),
    },
    {
        .id = STATE_1,
        .function = routine_1,
        .next_default = STATE_2,
        .transitions = state_1_transitions,
        .num_transitions = EAGLETRT_FSM_TRANSITION_LEN(state_1_transitions),
    },
    {
        .id = STATE_2,
        .function = routine_2,
        .next_default = STATE_2, // This is a repeat state
        .transitions = state_2_transitions,
        .num_transitions = EAGLETRT_FSM_TRANSITION_LEN(state_2_transitions),
    },
    {
        .id = STATE_3,
        .function = routine_3,
        .next_default = STATE_3, // This is a repeat state with no transitions
        .transitions = NULL,     // No transitions from this state
        .num_transitions = 0,
    },
};

struct FSMHandler handler;

void setUp(void) {
    RESET_FAKE(routine_0);
    RESET_FAKE(routine_1);
    RESET_FAKE(routine_2);
    RESET_FAKE(routine_3);

    RESET_FAKE(transition_0_1);
    RESET_FAKE(transition_2_3);

    fsm_api_init(&handler, default_states, STATE_COUNT, STATE_0);
}

void tearDown(void) {
}

// Initialization tests

void test_fsm_init_null_handler(void) {

    enum FSMReturnCode rc = fsm_api_init(NULL, default_states, STATE_COUNT, STATE_0);

    TEST_ASSERT_EQUAL_MESSAGE(FSM_RC_NULL_POINTER, rc, "Expected FSM_RC_NULL_POINTER when handler is NULL");
}

void test_fsm_init_null_state_list(void) {

    enum FSMReturnCode rc = fsm_api_init(&handler, NULL, STATE_COUNT, STATE_0);
    TEST_ASSERT_EQUAL_MESSAGE(FSM_RC_NULL_POINTER, rc, "Expected FSM_RC_NULL_POINTER when state list is NULL");
}

void test_fsm_init_invalid_state_count(void) {

    enum FSMReturnCode rc = fsm_api_init(&handler, default_states, 1, STATE_0);
    TEST_ASSERT_EQUAL_MESSAGE(FSM_RC_INVALID_MACHINE, rc, "Expected FSM_RC_INVALID_MACHINE when state count is less than or equal to 1");
}

void test_fsm_init_invalid_initial_state(void) {

    enum FSMReturnCode rc = fsm_api_init(&handler, default_states, STATE_COUNT, STATE_COUNT); // Invalid initial state ID

    TEST_ASSERT_EQUAL_MESSAGE(FSM_RC_INVALID_STATE, rc, "Expected FSM_RC_INVALID_STATE when initial state ID is invalid");
}

void test_fsm_init_null_state_function(void) {

    struct State invalid_state[STATE_COUNT] = {};
    memcpy(invalid_state, default_states, sizeof(default_states));
    invalid_state[1].function = NULL;

    enum FSMReturnCode rc = fsm_api_init(&handler, invalid_state, STATE_COUNT, STATE_0);

    TEST_ASSERT_EQUAL_MESSAGE(FSM_RC_INVALID_MACHINE, rc, "Expected FSM_RC_INVALID_MACHINE when state function is NULL");
}

void test_fsm_init_invalid_next_default_state(void) {

    struct State invalid_state[STATE_COUNT] = {};
    memcpy(invalid_state, default_states, sizeof(default_states));
    invalid_state[1].next_default = STATE_COUNT; // Invalid state ID

    enum FSMReturnCode rc = fsm_api_init(&handler, invalid_state, STATE_COUNT, STATE_0);

    TEST_ASSERT_EQUAL_MESSAGE(FSM_RC_INVALID_MACHINE, rc, "Expected FSM_RC_INVALID_MACHINE when next default state is invalid");
}

void test_fsm_init_invalid_next_default_state_not_in_transitions(void) {

    struct State invalid_state[STATE_COUNT] = {};
    memcpy(invalid_state, default_states, sizeof(default_states));
    invalid_state[0].next_default = STATE_3; // State 3 is not in the transitions of state 0 and state 0 is not a repeat state

    enum FSMReturnCode rc = fsm_api_init(&handler, invalid_state, STATE_COUNT, STATE_0);

    TEST_ASSERT_EQUAL_MESSAGE(FSM_RC_INVALID_MACHINE, rc, "Expected FSM_RC_INVALID_MACHINE when next default state is not in transitions for non-repeat states");
}

void test_fsm_init_null_state_transitions_pointer(void) {

    struct State invalid_state[STATE_COUNT] = {};
    memcpy(invalid_state, default_states, sizeof(default_states));
    invalid_state[1].transitions = NULL;

    enum FSMReturnCode rc = fsm_api_init(&handler, invalid_state, STATE_COUNT, STATE_0);

    TEST_ASSERT_EQUAL_MESSAGE(FSM_RC_INVALID_MACHINE, rc, "Expected FSM_RC_INVALID_MACHINE when state transitions pointer is NULL");
}

void test_fsm_init_invalid_state_ID(void) {

    struct State invalid_state[STATE_COUNT] = {};
    memcpy(invalid_state, default_states, sizeof(default_states));
    invalid_state[1].id = STATE_COUNT; // Invalid state ID

    enum FSMReturnCode rc = fsm_api_init(&handler, invalid_state, STATE_COUNT, STATE_0);

    TEST_ASSERT_EQUAL_MESSAGE(FSM_RC_INVALID_MACHINE, rc, "Expected FSM_RC_INVALID_MACHINE when state ID is invalid");
}

void test_fsm_init_non_consecutive_ID(void) {

    struct State invalid_state[STATE_COUNT] = {};
    memcpy(invalid_state, default_states, sizeof(default_states));
    invalid_state[1].id = 5; // Non-consecutive state ID

    enum FSMReturnCode rc = fsm_api_init(&handler, invalid_state, STATE_COUNT, STATE_0);

    TEST_ASSERT_EQUAL_MESSAGE(FSM_RC_INVALID_MACHINE, rc, "Expected FSM_RC_INVALID_MACHINE when state IDs are not consecutive starting from 0");
}

void test_fsm_init_invalid_transition_ID(void) {

    struct State invalid_state[STATE_COUNT] = {};
    memcpy(invalid_state, default_states, sizeof(default_states));

    // Modify constant array
    struct Transition *ptr = (struct Transition *)invalid_state[1].transitions;

    ptr[0].to = STATE_COUNT;

    enum FSMReturnCode rc = fsm_api_init(&handler, invalid_state, STATE_COUNT, STATE_0);

    TEST_ASSERT_EQUAL_MESSAGE(FSM_RC_INVALID_TRANSITION, rc, "Expected FSM_RC_INVALID_TRANSITION when transition state ID is invalid");

    //Reset transition to valid value for subsequent tests
    ptr[0].to = STATE_2;
}

void test_fsm_init_ok_parameters(void) {

    enum FSMReturnCode rc = fsm_api_init(&handler, default_states, STATE_COUNT, STATE_0);

    TEST_ASSERT_EQUAL_MESSAGE(FSM_RC_OK, rc, "Expected FSM_RC_OK when parameters are valid");
}

// Run state tests (treating trigger event as trusted dependency)

void test_fsm_run_state_null_handler(void) {
    enum FSMReturnCode rc = fsm_api_run_state(NULL, NULL);

    TEST_ASSERT_EQUAL_MESSAGE(FSM_RC_NULL_POINTER, rc, "Expected FSM_RC_NULL_POINTER when handler is NULL");
}

void test_fsm_run_state_non_repeat(void) {
    uint8_t data = 0;
    enum FSMReturnCode rc = fsm_api_run_state(&handler, &data);

    // State function
    TEST_ASSERT_EQUAL_MESSAGE(FSM_RC_OK, rc, "Expected FSM_RC_OK when parameters are valid");
    TEST_ASSERT_EQUAL_MESSAGE(1, routine_0_fake.call_count, "Expected state function to be called once");
    TEST_ASSERT_EQUAL_MESSAGE(&data, routine_0_fake.arg0_val, "Expected state function to be called with correct data");

    // Transition function
    TEST_ASSERT_EQUAL_MESSAGE(1, transition_0_1_fake.call_count, "Expected transition function to be called once");
    TEST_ASSERT_EQUAL_MESSAGE(&data, transition_0_1_fake.arg0_val, "Expected transition function to be called with correct data");
    TEST_ASSERT_EQUAL_MESSAGE(STATE_1, handler.current_state, "Expected current state to be updated to the state specified in the transition");
    TEST_ASSERT_EQUAL_MESSAGE(STATE_2, handler.requested_state, "Expected requested state to be updated to the state specified in the transition");
}

void test_fsm_run_state_repeat(void) {
    handler.current_state = STATE_2; // State 2 is a repeat state
    handler.requested_state = STATE_2;

    uint8_t data = 0;
    enum FSMReturnCode rc = fsm_api_run_state(&handler, &data);

    TEST_ASSERT_EQUAL_MESSAGE(FSM_RC_OK, rc, "Expected FSM_RC_OK when parameters are valid");
    TEST_ASSERT_EQUAL_MESSAGE(1, routine_2_fake.call_count, "Expected state function to be called once");
    TEST_ASSERT_EQUAL_MESSAGE(&data, routine_2_fake.arg0_val, "Expected state function to be called with correct data");
    TEST_ASSERT_EQUAL_MESSAGE(STATE_2, handler.requested_state, "Expected requested state to be updated to the state specified in the transition");
}

void test_fsm_run_state_with_null_transition_function(void) {
    handler.current_state = STATE_1; // State 1 has a transition with a NULL function
    handler.requested_state = STATE_2;

    uint8_t data = 0;
    enum FSMReturnCode rc = fsm_api_run_state(&handler, &data);

    TEST_ASSERT_EQUAL_MESSAGE(FSM_RC_OK, rc, "Expected FSM_RC_OK when parameters are valid");
    TEST_ASSERT_EQUAL_MESSAGE(1, routine_1_fake.call_count, "Expected state function to be called once");
    TEST_ASSERT_EQUAL_MESSAGE(&data, routine_1_fake.arg0_val, "Expected state function to be called with correct data");
    TEST_ASSERT_EQUAL_MESSAGE(STATE_2, handler.current_state, "Expected current state to be updated to the state specified in the transition even if the transition function is NULL");
}

// Trigger event tests

void test_fsm_trigger_event_null_handler(void) {
    enum FSMReturnCode rc = fsm_api_transition_to(NULL, STATE_1);

    TEST_ASSERT_EQUAL_MESSAGE(FSM_RC_NULL_POINTER, rc, "Expected FSM_RC_NULL_POINTER when handler is NULL");
}

void test_fsm_trigger_event_invalid_state(void) {
    enum FSMReturnCode rc = fsm_api_transition_to(&handler, STATE_COUNT); // Invalid state ID

    TEST_ASSERT_EQUAL_MESSAGE(FSM_RC_INVALID_STATE, rc, "Expected FSM_RC_INVALID_STATE when state ID is invalid");
}

void test_fsm_trigger_event_invalid_transition(void) {
    enum FSMReturnCode rc = fsm_api_transition_to(&handler, STATE_3); // No transition from state 0 to state 3

    TEST_ASSERT_EQUAL_MESSAGE(FSM_RC_INVALID_TRANSITION, rc, "Expected FSM_RC_INVALID_TRANSITION when there is no transition to the requested state from the current state");
}

void test_fsm_trigger_event_ok(void) {

    handler.current_state = STATE_2; // Use a repeat state
    handler.requested_state = STATE_2;

    enum FSMReturnCode rc = fsm_api_transition_to(&handler, STATE_3);

    TEST_ASSERT_EQUAL_MESSAGE(FSM_RC_OK, rc, "Expected FSM_RC_OK when parameters are valid");
    TEST_ASSERT_EQUAL_MESSAGE(STATE_3, handler.requested_state, "Expected requested state to be updated to the requested state");
}

void test_fsm_trigger_event_ok_same_state(void) {

    handler.current_state = STATE_2; // Use a repeat state
    handler.requested_state = STATE_2;

    enum FSMReturnCode rc = fsm_api_transition_to(&handler, STATE_2);

    TEST_ASSERT_EQUAL_MESSAGE(FSM_RC_OK, rc, "Expected FSM_RC_OK when parameters are valid");
    TEST_ASSERT_EQUAL_MESSAGE(STATE_2, handler.requested_state, "Expected requested state to remain the same when triggering an event to the same state");
}

int main() {
    UNITY_BEGIN();
    // init tests
    RUN_TEST(test_fsm_init_null_handler);
    RUN_TEST(test_fsm_init_null_state_list);
    RUN_TEST(test_fsm_init_invalid_state_count);
    RUN_TEST(test_fsm_init_invalid_initial_state);
    RUN_TEST(test_fsm_init_null_state_function);
    RUN_TEST(test_fsm_init_invalid_next_default_state);
    RUN_TEST(test_fsm_init_invalid_next_default_state_not_in_transitions);
    RUN_TEST(test_fsm_init_null_state_transitions_pointer);
    RUN_TEST(test_fsm_init_invalid_state_ID);
    RUN_TEST(test_fsm_init_non_consecutive_ID);
    RUN_TEST(test_fsm_init_invalid_transition_ID);
    RUN_TEST(test_fsm_init_ok_parameters);

    // run state tests
    RUN_TEST(test_fsm_run_state_null_handler);
    RUN_TEST(test_fsm_run_state_non_repeat);
    RUN_TEST(test_fsm_run_state_repeat);
    RUN_TEST(test_fsm_run_state_with_null_transition_function);

    // trigger event tests
    RUN_TEST(test_fsm_trigger_event_null_handler);
    RUN_TEST(test_fsm_trigger_event_invalid_state);
    RUN_TEST(test_fsm_trigger_event_invalid_transition);
    RUN_TEST(test_fsm_trigger_event_ok);
    RUN_TEST(test_fsm_trigger_event_ok_same_state);
    return UNITY_END();
}