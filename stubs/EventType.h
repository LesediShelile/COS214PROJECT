#ifndef EVENTTYPE_H
#define EVENTTYPE_H
/** STUB: include/EventType.h is not in development yet (Lesedi's WorkflowEvent.h
 *  includes it). WORKFLOW_FINISHED is used by WorkflowInstance.cpp;
 *  TASK_ACTION is what AuditedTask publishes. Replace with the real enum and
 *  keep (or rename) TASK_ACTION in AuditedTask.cpp. */
enum class EventType { TASK_ACTION, WORKFLOW_FINISHED };
#endif
