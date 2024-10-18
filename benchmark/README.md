# Benchmark

## How To Use

Simply run `benchmark` with the desired parameters. Alternatively, `benchmarks/` contains some pre-defined benchmarks.

## Usage

_`benchmark`_ `client_path hugepages? xpmem? stepsize vpagesize pageout?`

- `client_path`: Path to client executable. When run from `build/` directory is simply `client`.
- `hugepages?`: Whether to map huge or base pages in the manager.
- `xpmem?`: Whether to benchmark XPMEM+userfault or normal OS memory management performance
- `stepsize`: Step-size to use for memory traversal and access.
- `vpagesize`: Virtual page size which is used for memory management operations
- `pageout?`: Whether to page-out (or, more specifically, migrate) data on page-out or only perform the page out preparation operations.
