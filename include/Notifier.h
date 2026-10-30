
#ifndef NOTIFIER_H
#define NOTIFIER_H

#include "WorkflowListener.h"
#include <string>
#include <vector>

class Notifier : public WorkflowListener {
private:
    std::vector<std::string> notifications;

public:
    Notifier();

    void update(WorkflowEvent* event) override;

    std::vector<std::string> getNotifications() const;

    int getNotificationCount() const;

    void clearNotifications();

    ~Notifier() override;
};

#endif
