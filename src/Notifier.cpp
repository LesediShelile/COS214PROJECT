
#include "../include/Notifier.h"
#include "../include/WorkflowEvent.h"
#include "../include/EventType.h"

#include <sstream>

Notifier::Notifier() {}

void Notifier::update(WorkflowEvent* event) {
    if (event == nullptr) {
        return;
    }

    std::ostringstream message;

    message << "[Workflow Notification] ";

    switch (event->getType()) {
        case EventType::ITEM_STATE_CHANGED:
            message << "Item state changed";
            break;

        case EventType::ITEM_COMPLETED:
            message << "Item completed";
            break;

        case EventType::TASK_ACTION:
            message << "Task action performed";
            break;

        case EventType::EXTERNAL_EVENT:
            message << "External event received";
            break;

        case EventType::TIMEOUT:
            message << "Approval timeout";
            break;

        case EventType::WORKFLOW_FINISHED:
            message << "Workflow finished";
            break;
    }

    message << " | Source: " << event->getSourceID()
            << " | Details: " << event->getDetails()
            << " | Time: " << event->getTimestamp();

    notifications.push_back(message.str());
}

std::vector<std::string> Notifier::getNotifications() const {
    return notifications;
}

int Notifier::getNotificationCount() const {
    return static_cast<int>(notifications.size());
}

void Notifier::clearNotifications() {
    notifications.clear();
}

Notifier::~Notifier() {}
