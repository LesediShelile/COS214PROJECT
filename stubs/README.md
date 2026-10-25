# stubs/

Temporary stand-ins so Composite / Prototype / Iterator can be built and tested
before the State, Strategy and Participant branches are merged.
DELETE this folder and build with `-DTIVIDY_USE_STUBS=OFF` once they land.

Contract my code relies on (agree with the owners):
- `TaskState` provides `virtual bool isFinal() const` and
  `virtual bool isAvailable() const` (so `Task::isFinished()` / `isAvailable()`
  need no string comparison or type checks).
- `TaskState::clone()` returns a new, independent state object.
- A state ends a transition with `task->handleChange(new NextState())` as its
  last statement.
- `AssignmentStrategy::selectParticipant(Task*, const std::vector<Participant*>&)`.
- `TaskMemento` is the team's real `include/TaskMemento.h` (friend class Task,
  members `state`, `assignee`, `data`); only its .cpp is stubbed here.
