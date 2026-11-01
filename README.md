<div align="center">

# 🗂️ TIVIDY

### A generic workflow management system in C++11

*One reusable engine. Any industry. Fourteen GoF design patterns.*

[![CI](https://github.com/LesediShelile/COS214PROJECT/actions/workflows/ci.yml/badge.svg)](https://github.com/LesediShelile/COS214PROJECT/actions/workflows/ci.yml)
![C++11](https://img.shields.io/badge/C%2B%2B-11-blue?logo=cplusplus)
![CMake](https://img.shields.io/badge/build-CMake%203.16%2B-064F8C?logo=cmake)
![Tests](https://img.shields.io/badge/tests-295%20passing-brightgreen)
![Patterns](https://img.shields.io/badge/GoF%20patterns-14-orange)
![Docker](https://img.shields.io/badge/docker-ready-2496ED?logo=docker&logoColor=white)

COS 214 final project · University of Pretoria

**[📦 Repository](https://github.com/LesediShelile/COS214PROJECT)**

</div>

---

TIVIDY stores workflow *definitions*, creates independent running *instances* from them, routes work to participants, enforces multi-level approvals, reacts to events, talks to external systems and keeps a full history that can be undone.

The engine is generic: **nothing in the core mentions an industry.** We prove it by running two completely different industries on the same engine: a hospital **emergency patient admission** and a **legal case**. Only the workflow definition, the roles and the data change.

## 📑 Table of Contents

- [🚀 Quick start](#-quick-start)
- [🔧 How it works](#-how-it-works)
- [🧩 Design patterns and where they live](#-design-patterns-and-where-they-live)
- [👥 Who built what](#-who-built-what)
- [📁 Repository layout](#-repository-layout)
- [🧪 Testing](#-testing)
- [🏥 The demo scenarios](#-the-demo-scenarios)
- [🐳 Docker and CI](#-docker-and-ci)
- [📚 Documentation](#-documentation)
- [🌿 Git workflow](#-git-workflow)
- [🙌 The team](#-the-team)

## 🚀 Quick start

You need a C++11 compiler (g++ or clang++) and CMake 3.16 or newer. The engine and its tests need nothing else, because [doctest](https://github.com/doctest/doctest) is included in `tests/third_party/`.

```bash
git clone https://github.com/LesediShelile/COS214PROJECT.git
cd COS214PROJECT
git checkout development

cmake -S . -B build
cmake --build build
./build/tividy_tests            # Windows: .\build\tividy_tests.exe
```

Expected result: `test cases: 295 | 295 passed | 0 failed`.

Other ways to build and run:

| What | How |
|---|---|
| Engine and tests with plain `make` | `make test` (output goes to `build_make/`) |
| Skip the tests | `cmake -S . -B build -DTIVIDY_BUILD_TESTS=OFF` |
| Desktop GUI demo (needs Qt 6 Widgets) | `cmake -S . -B build -DTIVIDY_BUILD_DESKTOP=ON` then `cmake --build build --target tividy_desktop` |
| Desktop GUI demo in Docker | `docker build -t tividy .` then run the image (it starts `tividy_desktop`) |
| Memory and undefined-behaviour checks | `cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_FLAGS="-fsanitize=address,undefined"` |

> The GUI is optional and is never part of the engine library, so the engine builds and tests without Qt.

## 🔧 How it works

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

## 🧩 Design patterns and where they live

All paths are inside `include/` (headers) and `src/` (implementations).

| Pattern | Role in TIVIDY | Main classes |
|---|---|---|
| 🌳 **Composite** | Tasks nested in stages | `WorkItem`, `Task`, `Stage`, `SequentialStage`, `ParallelStage` |
| 🧬 **Prototype** | Each instance gets its own copy of the definition's tree | `WorkItem::clone()` on every work item, `WorkflowDefinition` |
| 🔁 **Iterator** | Walk the tree; find work that is available now | `WorkItemIterator`, `DepthFirstIterator`, `AvailableItemsIterator` |
| 🎁 **Decorator** | Extra behaviour on a task without new task types | `TaskDecorator`, `ValidatedTask`, `AuditedTask`, `PriorityTask` |
| 🚦 **State** | Task lifecycle | `TaskState`, `CreatedState`, `AvailableState`, `AssignedState`, `StartedState`, `CompletedState`, `RejectedState`, `CancelledState` |
| 🎯 **Strategy** | Who gets the next task | `AssignmentStrategy`, `RoleBasedAssignment`, `WorkloadAssignment`, `RuleBasedAssignment` |
| 🔗 **Chain of Responsibility** | Approval levels and escalation | `ApprovalHandler`, `ApprovalLevel`, `ApprovalRequest`, `ApprovalStatus` |
| 📡 **Observer** | Events, audit, notifications, metrics | `EventPublisher`, `WorkflowListener`, `WorkflowEvent`, `EventType`, `AuditLogger`, `Notifier`, `EngineRouter`, `MetricsCollector` |
| ⌨️ **Command** | Recorded, undoable actions | `WorkflowCommand`, `AssignCommand`, `StartCommand`, `CompleteCommand`, `RejectCommand`, `CommandInvoker`, `CommandHistory` |
| 💾 **Memento** | Snapshots used by undo | `TaskMemento` |
| 🏗️ **Builder** | Constructing a definition step by step | `WorkflowBuilder`, `DefinitionBuilder`, `Director` |
| 🔌 **Adapter** | External systems | `ExternalConnector`, `AdapterOne`, `AdapterTwo`, `APIOne`, `APITwo`, `ExternalTask` |
| 🏛️ **Facade** | Single client entry point | `TIVIDYFacade` |
| 🛡️ **Proxy** | Permission and ownership checks in front of the facade | `IWorkflowService`, `SecureWorkflowProxy` |

### Ownership rules

- A `Stage` owns its children, and a `TaskDecorator` owns the item it wraps.
- Iterators and mementos returned by `create...()` functions belong to the caller, who must `delete` them. `CommandInvoker` takes ownership of the commands passed to `executeCommand()`.
- `clone()` copies work state, data and children. Shared configuration (roles, strategies, participants, the event publisher) is pointed to, not copied.
- `Stage::assign/start/complete/reject` throw `std::logic_error`, because those only make sense on a task. `makeAvailable()` and `cancel()` act on the children.

## 👥 Who built what

| Member | Patterns and areas |
|---|---|
| **Kiara Ajodhaparsadh** | System design, UML, CI pipeline, Command, Memento, Facade, Proxy, workflow definition and instance classes, integration |
| **Cameron Ambraal** | State, Strategy, Chain of Responsibility, scenario write-ups, presentation |
| **Lesedi Shelile** | Observer, Adapter, Docker, desktop GUI |
| **Taya Govender** | Builder, Iterator, UML diagram |
| **Ajba Patel** | Composite, Prototype, Decorator, their unit and integration tests, README, pamphlet |

## 📁 Repository layout

```
include/      all headers (.h); include/ui/ holds the optional GUI header
src/          all implementations (.cpp)
tests/        doctest suites; tests/third_party/ holds doctest itself
docs/         design documents and diagrams
coverage_report/   generated lcov HTML coverage report
main.cpp      desktop GUI demo (optional, needs Qt)
CMakeLists.txt, Makefile, Dockerfile
.github/workflows/ci.yml
```

## 🧪 Testing

Tests use [doctest](https://github.com/doctest/doctest). Run them as shown in [Quick start](#-quick-start).

**295 test cases and 1098 assertions, all passing.** Every test file below is compiled into one executable, `tividy_tests`.

### Core structure: Composite, Prototype, Iterator, Decorator

| File | Tests | What it checks |
|---|---:|---|
| `tests/test_work_items.cpp` | 16 | Stages, cloning, cancel, mementos, decorator base, iterators |
| `tests/test_decorators.cpp` | 7 | `ValidatedTask`, `AuditedTask`, `PriorityTask`, stacking and cloning |
| `tests/test_integration_work_items.cpp` | 5 | Composite, Prototype and Decorators together with Commands, undo, Observer and iterators |

### Behaviour: State, Strategy, Chain of Responsibility

| File | Tests | What it checks |
|---|---:|---|
| `tests/test_state_strategy_chain.cpp` | 8 | Task lifecycle, assignment strategies, approval levels |
| `tests/test_task_state_defaults.cpp` | 7 | Default `TaskState` rejects illegal transitions |
| `tests/test_cancelledState.cpp` | 3 | The final Cancelled state |

### Building and history: Builder, Command, Memento

| File | Tests | What it checks |
|---|---:|---|
| `tests/test_director.cpp` | 5 | Building workflow definitions step by step |
| `tests/test_commandMementoWorkflow.cpp` | 25 | Commands, undo, mementos, workflow definition and instance |

### Events and outside systems: Observer, Adapter

| File | Tests | What it checks |
|---|---:|---|
| `tests/test_observer_integrations.cpp` | 29 | Event publishing, listeners, audit and metrics |
| `tests/test_notifier.cpp` | 6 | Notification text for every event type |
| `tests/test_engine_router.cpp` | 4 | Timeouts forwarded to the approval chain |
| `tests/test_adapter.cpp` | 12 | Adapters and their APIs |
| `tests/test_external_task.cpp` | 11 | External tasks and connectors |

### Entry points: Facade and Proxy

| File | Tests | What it checks |
|---|---:|---|
| `tests/test_facade.cpp` | 70 | `TIVIDYFacade`: definitions, instances, task actions |
| `tests/test_facadeCommand.cpp` | 18 | Command and Memento undo reached through the Facade |
| `tests/test_FacadeCoR.cpp` | 28 | Approval chain reached through the Facade |
| `tests/test_proxy.cpp` | 41 | `SecureWorkflowProxy` permission and ownership checks |

### Coverage

The CMake build is compiled with `--coverage`, and a generated lcov HTML report is committed in [`coverage_report/`](coverage_report/index.html). Open `coverage_report/index.html` in a browser to see line and function coverage per file.

## 🏥 The demo scenarios

**Hospital emergency admission.** Paramedic pickup and transport, patient registration, room preparation in parallel with transport, handover, nurse intake, doctor acceptance (with a deadline and reroute), assessment and treatment, room allocation (ICU and surgery need approval) and cleaning. Failure paths cover a delayed ambulance, a doctor who does not respond and a deteriorating patient. The outcome is a patient admitted, transferred, discharged or recorded as deceased, with the full history archived.

**Legal case.** Client intake, conflict check, parallel discovery (evidence and legal research), drafting, partner review, filing, hearing, judgment, invoicing and archiving. It includes a rejected-pleadings exception path and a cancelled filing.

**Participants:** paramedics, secretary, nursing staff, doctors, cleaning staff, lawyers, partners. **External systems:** ambulance dispatch and hospital patient records, reached through adapters.

Only the definitions, the adapters and their APIs know about an industry. The engine runs both processes without changes. The desktop GUI also has a **Playground** tab where you build your own tasks and stages and drive them yourself.

## 🐳 Docker and CI

- **Docker.** The `Dockerfile` starts from Ubuntu 24.04, installs the compiler, CMake and Qt 6, builds only the desktop target and launches `tividy_desktop` with the `xcb` display platform. Tests are not part of the image.
- **CI.** The GitHub Actions workflow in `.github/workflows/ci.yml` runs on every push and pull request to `development` and `main`. It starts from a clean Ubuntu 24.04 machine, installs the dependencies, configures the project and builds the desktop application.

## 📚 Documentation

Design documents and diagrams are in [`docs/`](docs/):

- [UML class diagram](docs/UML%20class%20diagram.pdf)
- [Project report](docs/COS214Project%20(2).pdf)
- Activity diagrams: [1](docs/Activity%20Diagram1.jpg) and [3](docs/Activity%20Diagram%203.png)

## 🌿 Git workflow

- `main` holds stable, reviewed work only. Nobody pushes to it directly.
- `development` is the integration branch. All work is merged here through pull requests.
- Each pattern or task is built on its own branch (`feature/<name>`, `test/<name>`, `fix/<name>`, `docs/<name>`) cut from `development` and merged back by a pull request with base `development`.
- Commits are small and describe one change each.

## 🙌 The team

Group Council, COS 214, University of Pretoria:

- Kiara Ajodhaparsadh
- Cameron Ambraal
- Lesedi Shelile
- Taya Govender
- Ajba Patel

<div align="center">

Made for COS 214 · University of Pretoria

</div>
