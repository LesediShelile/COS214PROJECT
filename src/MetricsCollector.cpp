#include "../include/MetricsCollector.h"
#include <sstream>

/**
 * @brief Constructs a MetricsCollector.
 */
MetricsCollector::MetricsCollector() {
}

/**
 * @brief Updates metrics using a workflow event.
 * @param event The workflow event received.
 */
void MetricsCollector::update(WorkflowEvent* event) {
    if (event == 0) {
        return;
    }

    std::string state = event->getDetails();
    transitionCounts[state]++;

    stateEntryTime[state] = event->getTimestamp();
}

/**
 * @brief Returns the number of transitions recorded for a state.
 * @param state The state to check.
 * @return The transition count, or 0 if the state is not recorded.
 */
int MetricsCollector::getTransitionCount(std::string state) {
    if (transitionCounts.find(state) != transitionCounts.end()) {
        return transitionCounts[state];
    }

    return 0;
}

/**
 * @brief Returns the average time spent in a state.
 * @param state The state to check.
 * @return The average time, or 0.0f if no time has been recorded.
 */
float MetricsCollector::getAverageTimeInState(std::string state) {
    if (totalTimeInState.find(state) != totalTimeInState.end()) {
        return totalTimeInState[state];
    }

    return 0.0f;
}

/**
 * @brief Creates a summary of the collected metrics.
 * @return A string containing the metrics summary.
 */
std::string MetricsCollector::getSummary() {
    std::ostringstream summary;

    for (std::map<std::string, int>::iterator it =
             transitionCounts.begin();
         it != transitionCounts.end(); ++it) {
        summary << "State: " << it->first
                << ", Transitions: " << it->second << "\n";
    }

    for (std::map<std::string, float>::iterator it =
             totalTimeInState.begin();
         it != totalTimeInState.end(); ++it) {
        summary << "State: " << it->first
                << ", Total time: " << it->second << "\n";
    }

    return summary.str();
}

/**
 * @brief Destroys the MetricsCollector.
 */
MetricsCollector::~MetricsCollector() {
}
