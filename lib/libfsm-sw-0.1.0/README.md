# libfsm-sw
A lightweight, event-driven finite state machine library for embedded C applications.

---
# Core Concepts

## State

A state contains:

* An identifier
* A routine function
* A default next state
* A transition array

Example:

```c
struct State {
    uint32_t id;
    void (*function)(void *data);
    uint32_t next_default;
    struct Transition *transitions;
    uint32_t num_transitions;
};
```

---

## Transition

A transition defines a valid move from one state to another.

Example:

```c
struct Transition {
    uint32_t to;
    void (*function)(void *data);
};
```

---

# State Execution Model

Each call to:

```c
fsm_api_run_state(&handler, user_data);
```

performs the following:

1. Executes the current state's routine
2. Checks for pending triggered events
3. Executes transition callback (if present)
4. Changes state accordingly
5. Handles automatic progression for non-repeat states

---

## Triggering Events

Events can be triggered to request a state change:
```c
fsm_api_transition_to(&handler, target_state_id);
```
The library checks if the transition is valid based on the current state and its transition array, then updates the requested state accordingly. Once the current state routine finishes executing, the library will check for any pending requested state changes and update the current state if necessary.

---

