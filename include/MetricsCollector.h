
#ifndef METRICSCOLLECTOR_H
#define METRICSCOLLECTOR_H

#include <map>
#include <string>
#include "WorkflowListener.h"

class MetricsCollector : public WorkflowListener {
private:
    std::map<std::string, int> transitionCounts;
std::map<std::string, std::string> stateEntryTime;
std::map<std::string, std::string> currentState;
std::map<std::string, double> totalTimeInState;
std::map<std::string, int> completedIntervals;

public:
    MetricsCollector();

    void update(WorkflowEvent* event) override;

    int getTransitionCount(std::string state);

    float getAverageTimeInState(std::string state);

    std::string getSummary();

    ~MetricsCollector() override;
};

#endif
