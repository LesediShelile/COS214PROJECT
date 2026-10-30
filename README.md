# TIVIDY

**A generic workflow management system in C++11.** COS 214 final project, University of Pretoria.

[![CI](https://github.com/LesediShelile/COS214PROJECT/actions/workflows/ci.yml/badge.svg)](https://github.com/LesediShelile/COS214PROJECT/actions/workflows/ci.yml)

**Repository:** [github.com/LesediShelile/COS214PROJECT](https://github.com/LesediShelile/COS214PROJECT)

TIVIDY stores workflow *definitions*, creates independent running *instances* from them, routes work to participants, enforces multi-level approvals, reacts to events, talks to external systems and keeps a full history that can be undone. The engine is generic: nothing in the core mentions an industry. Our demonstration scenario is a hospital **emergency patient admission**, from paramedic pickup to initial treatment, and the same engine also runs a legal-case workflow in the desktop demo.

## Contents

- [Quick start](#quick-start)
- [How it works](#how-it-works)
- [Design patterns and where they live](#design-patterns-and-where-they-live)
- [Repository layout](#repository-layout)
- [Testing](#testing)
- [The demo scenario](#the-demo-scenario)
- [Documentation](#documentation)
- [Git workflow](#git-workflow)
- [The team](#the-team)

## Quick start

You need a C++11 compiler (g++ or clang++) and CMake 3.16 or newer. The engine and its tests need nothing else, because [doctest](https://github.com/doctest/doctest) is included in `tests/third_party/`.

```bash
git clone https://github.com/LesediShelile/COS214PROJECT.git
cd COS214PROJECT
git checkout development

cmake -S . -B build
cmake --build build
./build/tividy_tests            # Windows: .\build\tividy_tests.exe
```

Other ways to build and run:

| What | How |
|---|---|
| Engine and tests with plain `make` | `make test` (output goes to `build_make/`) |
| Engine and tests without CMake | `g++ -std=c++11 -Iinclude -Itests/third_party <all src/*.cpp except main.cpp> tests/*.cpp -o tividy_tests` |
| Skip the tests | `cmake -S . -B build -DTIVIDY_BUILD_TESTS=OFF` |
| Desktop GUI demo (needs Qt 6 Widgets) | `cmake -S . -B build -DTIVIDY_BUILD_DESKTOP=ON` then `cmake --build build --target tividy_desktop` |
| Desktop GUI demo in Docker | `docker build -t tividy .` then run the image (it starts `tividy_desktop`) |
| Memory and undefined-behaviour checks | `cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_FLAGS="-fsanitize=address,undefined"` |

The GUI is optional and is never part of the engine library, so the engine builds and tests without Qt.

## How it works

```
 client / GUI
      |
 SecureWorkflowProxy ──> TIVIDYFacade          one simple entry point, with access checks
                              |
        ┌─────────────────────┼──────────────────────────┐
 WorkflowDefinition     WorkflowInstance          CommandInvoker + CommandHistory
 (built by Director,    (own cloned work-item      (undoable actions, each with
  frozen, versioned)     tree, own history)         a TaskMemento snapshot)
                              |
              WorkItem tree: Stage (Sequential / Parallel) ── Task
                              |                                 |
                     Iterators walk it                  TaskState, decorators, assignment
                                                        strategy, approval chain
                              |
              EventPublisher ──> AuditLogger, Notifier, EngineRouter, MetricsCollector
                              |
              ExternalTask ──> ExternalConnector ──> adapters ──> external APIs
```

- **Definition vs instance.** A `WorkflowDefinition` is frozen once built and owns the root of a `WorkItem` tree. Creating an instance clones that tree, so every `WorkflowInstance` has its own state, assignees and data. Running instances keep the definition version they started with.
- **Nested work.** `Task` is the leaf and `SequentialStage` and `ParallelStage` are composites. The engine treats every `WorkItem` the same way and never checks types.
- **Changing behaviour.** `TaskState` controls the task lifecycle, assignment is a Strategy, approval and escalation are a Chain of Responsibility, and Decorators add validation, auditing and priority.
- **Reacting to events.** Tasks publish `WorkflowEvent`s and independent listeners handle routing, audit, notification and metrics.
- **History and recovery.** Every action is a Command with `undo()`, and each keeps a Memento of the work item taken beforehand.
- **External systems.** The engine only knows `ExternalConnector`. Adapters translate it to each external system.

## Design patterns and where they live

All paths are inside `include/` (headers) and `src/` (implementations).

| Pattern | Role in TIVIDY | Main classes |
|---|---|---|
| **Composite** | Tasks nested in stages | `WorkItem`, `Task`, `Stage`, `SequentialStage`, `ParallelStage` |
| **Prototype** | Each instance gets its own copy of the definition's tree | `WorkItem::clone()` on every work item, `WorkflowDefinition` |
| **Iterator** | Walk the tree; find work that is available now | `WorkItemIterator`, `DepthFirstIterator`, `AvailableItemsIterator` |
| **Decorator** | Extra behaviour on a task without new task types | `TaskDecorator`, `ValidatedTask`, `AuditedTask`, `PriorityTask` |
| **State** | Task lifecycle | `TaskState`, `CreatedState`, `AvailableState`, `AssignedState`, `StartedState`, `CompletedState`, `RejectedState`, `CancelledState` |
| **Strategy** | Who gets the next task | `AssignmentStrategy`, `RoleBasedAssignment`, `WorkloadAssignment`, `RuleBasedAssignment` |
| **Chain of Responsibility** | Approval levels and escalation | `ApprovalHandler`, `ApprovalLevel`, `ApprovalRequest`, `ApprovalStatus` |
| **Observer** | Events, audit, notifications, metrics | `EventPublisher`, `WorkflowListener`, `WorkflowEvent`, `EventType`, `AuditLogger`, `Notifier`, `EngineRouter`, `MetricsCollector` |
| **Command** | Recorded, undoable actions | `WorkflowCommand`, `AssignCommand`, `StartCommand`, `CompleteCommand`, `RejectCommand`, `CommandInvoker`, `CommandHistory` |
| **Memento** | Snapshots used by undo | `TaskMemento` |
| **Builder** | Constructing a definition step by step | `WorkflowBuilder`, `DefinitionBuilder`, `Director` |
| **Adapter** | External systems | `ExternalConnector`, `AdapterOne`, `AdapterTwo`, `APIOne`, `APITwo`, `ExternalTask` |
| **Facade** | Single client entry point | `TIVIDYFacade` |
| **Proxy** | Permission and ownership checks in front of the facade | `IWorkflowService`, `SecureWorkflowProxy` |

### Ownership rules

- A `Stage` owns its children, and a `TaskDecorator` owns the item it wraps.
- Iterators and mementos returned by `create...()` functions belong to the caller, who must `delete` them. `CommandInvoker` takes ownership of the commands passed to `executeCommand()`.
- `clone()` copies work state, data and children. Shared configuration (roles, strategies, participants, the event publisher) is pointed to, not copied.
- `Stage::assign/start/complete/reject` throw `std::logic_error`, because those only make sense on a task. `makeAvailable()` and `cancel()` act on the children.

## Repository layout

```
include/      all headers (.h); include/ui/ holds the optional GUI header
src/          all implementations (.cpp)
tests/        doctest suites; tests/third_party/ holds doctest itself
docs/         design documents and diagrams
main.cpp      desktop GUI demo (optional, needs Qt)
CMakeLists.txt, Makefile, Dockerfile
.github/workflows/ci.yml
```

The CI pipeline checks that `src/` holds only `.cpp` files, `include/` only `.h` files, and that the project builds.

## Testing

Tests use [doctest](https://github.com/doctest/doctest). Run them as shown in [Quick start](#quick-start).

| File | What it checks |
|---|---|
| `tests/test_work_items.cpp` | Composite, Prototype and iterators: stages, cloning, cancel, mementos, decorator base |
| `tests/test_decorators.cpp` | `ValidatedTask`, `AuditedTask`, `PriorityTask`, stacking and cloning |
| `tests/test_integration_work_items.cpp` | Composite, Prototype and Decorators together with Commands, undo, Observer and iterators |
| `tests/test_adapter.cpp` | Adapters and external tasks |
| `tests/test_director.cpp` | Building workflow definitions |
| `tests/test_observer_integrations.cpp` | Event publishing and listeners |

## The demo scenario

**Emergency admission.** Paramedic pickup and transport, patient registration, room preparation in parallel with transport, handover, nurse intake, doctor acceptance (with a deadline and reroute), assessment and treatment, room allocation (ICU and surgery need approval) and cleaning. Failure paths cover a delayed ambulance, a doctor who does not respond and a deteriorating patient. The outcome is a patient admitted, transferred, discharged or recorded as deceased, with the full history archived.

**Participants:** paramedics, secretary, nursing staff, doctor, cleaning staff. **External systems:** ambulance dispatch and hospital patient records, reached through adapters.

Only the director, the adapters and their APIs know about the hospital. The same engine runs other processes without changes.

## Documentation

Design documents and diagrams are in [`docs/`](docs/):

- [UML class diagram](docs/UML%20class%20diagram.pdf)
- [Project report](docs/COS214Project%20(2).pdf)
- Activity diagrams: [1](docs/Activity%20Diagram1.jpg) and [3](docs/Activity%20Diagram%203.png)

## Git workflow

- `main` holds stable, reviewed work only. Nobody pushes to it directly.
- `development` is the integration branch. All work is merged here through pull requests.
- Each pattern or task is built on its own branch (`feature/<name>`, `test/<name>`, `fix/<name>`, `docs/<name>`) cut from `development` and merged back by a pull request with base `development`.
- Commits are small and describe one change each.

## The team

Group Council, COS 214, University of Pretoria:

- Kiara Ajodhaparsadh
- Cameron Ambraal
- Lesedi Shelile
- Taya Govender
- Ajba Patel
