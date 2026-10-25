
#ifndef METRICSCOLLECTOR_H
#define METRICSCOLLECTOR_H

#include <map>
#include <string>
#include "WorkflowListener.h"

/**
 * @brief Collects metrics from workflow events.
 */
class MetricsCollector : public WorkflowListener {
private:
    /** @brief Number of transitions recorded for each state. */
    std::map<std::string, int> transitionCounts;

    /** @brief Entry timestamp recorded for each state. */
    std::map<std::string, std::string> stateEntryTime;

    /** @brief Total time recorded for each state. */
    std::map<std::string, float> totalTimeInState;

public:
    /** @brief Constructs a MetricsCollector. */
    MetricsCollector();

    /**
     * @brief Updates metrics using a workflow event.
     * @param event Pointer to the workflow event.
     */
    void update(WorkflowEvent* event) override;

    /**
     * @brief Gets the transition count for a state.
     * @param state The state to look up.
     * @return The number of recorded transitions.
     */
    int getTransitionCount(std::string state);

    /**
     * @brief Gets the average time spent in a state.
     * @param state The state to look up.
     * @return The average time recorded for the state.
     */
    float getAverageTimeInState(std::string state);

    /**
     * @brief Creates a summary of the collected metrics.
     * @return A summary string.
     */
    std::string getSummary();

    /** @brief Destroys the MetricsCollector. */
    ~MetricsCollector() override;
};

#endif // METRICSCOLLECTOR_H
