# TIVIDY: Generic Workflow Management System (COS 214)

C++11 workflow engine. This branch contains the **work-item core**:
Composite, Prototype and Iterator (plus the Decorator base class).

## Requirements
- A C++11 compiler (g++ 5+, clang 3.4+, MSVC 2015+)
- CMake 3.10+
- doctest 2.4.11 (single header, already in `tests/third_party/`, MIT licence)

## Build, run and test
```
cmake -S . -B build
cmake --build build
./build/tividy_tests        # or: ctest --test-dir build
```
`TIVIDY_USE_STUBS` (default ON) builds against `stubs/` until the State,
Strategy and Memento branches are merged. Then run
`cmake -S . -B build -DTIVIDY_USE_STUBS=OFF`.

## Layout
| Folder | Contents |
|---|---|
| `include/`, `src/` | engine headers and sources |
| `stubs/` | temporary stand-ins for classes owned by other branches |
| `tests/` | doctest suite |
| `docs/` | Doxygen output and UML exports |

## Design notes (work-item core)
- `WorkItem` is the single interface the engine uses. `Task` is the leaf, and
  `SequentialStage` / `ParallelStage` are the composites.
- **Prototype:** `WorkItem::clone()` deep-copies a tree. A frozen
  `WorkflowDefinition` holds the template and each `WorkflowInstance` gets its
  own clone. Roles, strategies, participants and the publisher are shared
  (pointed to), never copied.
- **Ownership:** a `Stage` deletes its children and a `TaskDecorator` deletes
  the item it wraps. Iterators and mementos returned by `create...()` are
  owned by the caller.
- **Iterator:** `getChildren()` lives on `WorkItem` (empty for leaves), so
  iterators walk any tree without `dynamic_cast`.
  `AvailableItemsIterator` yields only available leaves.
- **Stages** refuse `assign/start/complete/reject` (`std::logic_error`). Act on
  their Tasks. After any task finishes, call `activateChildren()` on the root
  to release the next work.

## Documentation
Run `doxygen` in the repository root after the Doxyfile is added (`docs/`).

## Third-party
- doctest by Viktor Kirilov, https://github.com/doctest/doctest, MIT licence.
