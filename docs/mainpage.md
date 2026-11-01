# TIVIDY
### Generic Workflow Management Engine

---

## Welcome

TIVIDY is a generic C++11 workflow management engine developed as part of the COS214 Software Modelling project.

It provides a flexible environment for constructing, managing and executing workflows through a menu-driven Terminal User Interface (TUI).

The system is designed around reusable workflow components, extensible task definitions and object-oriented design principles.

## System Overview

TIVIDY allows users to interact with workflow definitions and independent workflow instances.

Its functionality includes:

- **Workflow Management** – Create, organise and manage workflow structures.
- **Task Management** – Add, configure and manage individual workflow tasks.
- **Workflow Execution** – Execute workflows while tracking task states and progress.
- **Scenario Management** – Explore predefined industry scenarios and create custom scenarios.
- **Instance Management** – Create and manage independent instances of workflow definitions.
- **Workflow Inspection** – Examine workflow structures and their component relationships.
- **Execution History** – Review recorded workflow operations.

## Architecture

The system follows an object-oriented architecture, emphasising:

- Modularity and separation of responsibilities.
- Reusable and extensible workflow components.
- Encapsulation and abstraction.
- Flexible task composition and execution.
- Application of established software design patterns.

Explore the **Class Hierarchy**, **Class Index** and **File List** to understand the system's implementation.

## Project Structure

| Directory | Description |
|---|---|
| `include/` | Header files, interfaces and class declarations. |
| `src/` | Source files and system implementations. |
| `tests/` | Automated tests and test-related utilities. |
| `docs/` | Doxygen configuration and project documentation. |

## Technology

| Technology | Purpose |
|---|---|
| C++11 | Core programming language. |
| Doxygen | API and source-code documentation. |
| Graphviz | Class, dependency and collaboration diagrams. |
| Doctest | Automated testing framework. |

## Building the Project

Build the project from the repository root using:

```bash
make
```

Refer to the project README for the complete build instructions and available commands.

## Running Tests

TIVIDY includes automated tests to verify system functionality.

Refer to the project build configuration and README for instructions on building and running the test suite.

## Generating Documentation

Documentation is generated using Doxygen.

From the project root, execute:

```bash
doxygen Doxyfile
```

Once generation is complete, open:

```text
docs/generated/html/index.html
```

## Exploring the Documentation

Use the navigation panel to explore:

- **Modules:** Related functionality grouped into logical components.
- **Classes:** Class definitions, methods and attributes.
- **Class Hierarchy:** Inheritance relationships between classes.
- **Class Members:** Available attributes and operations.
- **Files:** Source and header files.
- **File Members:** Functions, variables and definitions.

---

*TIVIDY | COS214 Software Modelling Project | C++11*