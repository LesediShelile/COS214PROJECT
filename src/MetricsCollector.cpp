
#include "../include/MetricsCollector.h"
#include "../include/WorkflowEvent.h"
#include "../include/EventType.h"

#include <sstream>
#include <ctime>
#include <cstdio>

namespace {

bool parseTimestamp(const std::string& timestamp, std::time_t& result) {
    int year, month, day, hour, minute, second;

    if (std::sscanf(timestamp.c_str(), "%d-%d-%d %d:%d:%d",
        &year, &month, &day, &hour, &minute, &second) == 6) {

        std::tm info = {};
        info.tm_year = year - 1900;
        info.tm_mon = month - 1;
        info.tm_mday = day;
        info.tm_hour = hour;
        info.tm_min = minute;
        info.tm_sec = second;
        info.tm_isdst = -1;

        result = std::mktime(&info);
        return result != static_cast<std::time_t>(-1);
    }

    if (std::sscanf(timestamp.c_str(), "%d:%d:%d",
        &hour, &minute, &second) == 3) {
        result = hour * 3600 + minute * 60 + second;
        return true;
    }

    if (std::sscanf(timestamp.c_str(), "%d:%d",
        &hour, &minute) == 2) {
        result = hour * 3600 + minute * 60;
        return true;
    }

    return false;
}

}

MetricsCollector::MetricsCollector() {}

void MetricsCollector::update(WorkflowEvent* event) {
    if (event == nullptr ||
        event->getType() != EventType::ITEM_STATE_CHANGED) {
        return;
    }

    std::string state = event->getDetails();
    std::string source = event->getSourceID();
    std::string timestamp = event->getTimestamp();

    if (state.empty() || source.empty()) return;

    std::time_t currentTime;

    if (!parseTimestamp(timestamp, currentTime)) return;

    if (stateEntryTime.count(source)) {
        std::time_t previousTime;

        if (parseTimestamp(stateEntryTime[source], previousTime)) {
            double elapsed = std::difftime(currentTime, previousTime);

            if (elapsed < 0 &&
                previousTime < 86400 &&
                currentTime < 86400) {
                elapsed += 86400;
            }

            if (elapsed >= 0 && currentState.count(source)) {
                std::string previousState = currentState[source];

                totalTimeInState[previousState] += elapsed;
                completedIntervals[previousState]++;
            }
        }
    }

    stateEntryTime[source] = timestamp;
    currentState[source] = state;
    transitionCounts[state]++;
}

int MetricsCollector::getTransitionCount(std::string state) {
    return transitionCounts[state];
}

float MetricsCollector::getAverageTimeInState(std::string state) {
    if (completedIntervals[state] == 0) {
        return 0.0f;
    }

    return static_cast<float>(
        totalTimeInState[state] / completedIntervals[state]
    );
}

std::string MetricsCollector::getSummary() {
    std::ostringstream summary;

    for (std::map<std::string, int>::iterator it =
        transitionCounts.begin();
        it != transitionCounts.end(); ++it) {

        summary << "State: " << it->first
                << ", Transitions: " << it->second
                << ", Average time: "
                << getAverageTimeInState(it->first)
                << " seconds\n";
    }

    return summary.str();
}

MetricsCollector::~MetricsCollector() {}
