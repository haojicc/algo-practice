# Queue Library Design

This project includes two queue implementations:

- `utils/queue_array.h` / `utils/queue_array.c`: array-backed circular queue with optional fixed capacity and dynamic expansion
- `utils/queue_list.h` / `utils/queue_list.c`: linked list queue with optional fixed capacity

## Features

- enqueue / dequeue
- front / back (peek)
- isEmpty / isFull
- size / capacity
- clear
- generic support via `void *` and runtime type identifiers
- type mismatch detection on enqueue
- dynamic expansion in array-backed queue
- circular buffer behavior in the array-backed queue
- deep copy and shallow copy semantics
- error codes for boundary and memory errors
- read-only shallow copy support
- unit tests in `src/tests/test_queue.c`

## Complexity

### Array-backed queue
- enqueue: O(1) amortized for dynamic queue, O(1) otherwise
- dequeue: O(1)
- front/back: O(1)
- isEmpty/isFull/size/capacity: O(1)
- clear: O(n)
- clone_deep: O(n)
- clone_shallow: O(1)

### List-backed queue
- enqueue: O(1)
- dequeue: O(1)
- front/back: O(1)
- isEmpty/isFull/size/capacity: O(1)
- clear: O(n)
- clone_deep: O(n)
- clone_shallow: O(1)

## Deep vs Shallow Copy

- Deep copy allocates new storage and duplicates all contained elements. The new queue is independent and can be modified safely.
- Shallow copy shares the original queue's underlying storage. It is useful for read-only inspection, but mutation and destruction of either instance can lead to undefined behavior.

## Build

- Use `cmake` from the project root to configure and build the project
- Use the provided `Makefile` to build directly with `cc`

## Example

See `src/tests/test_queue.c` for sample usage of both queue implementations.
