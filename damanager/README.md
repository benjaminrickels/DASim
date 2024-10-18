# `damanager`

## How to Use

Simply run `damanager`. Clients using `DASim` functionality via `damemory` can then connect to the manager. `damanager` can be quit using `Ctrl+C`, killing any remaining clients in the process.

## Customizing Buffer Management

To customize buffer management, implement the abstract `buffer_manager` class (see `include/damanager/buffer_manager.h`). Then create a `unique_ptr` to an instance of your implementation and pass it to the `client_manager` constructor (see `src/main.cc:54`).
