#include "ValidatedTask.h"
#include <stdexcept>

ValidatedTask::ValidatedTask(WorkItem* task, const std::vector<std::string>& fields)
    : TaskDecorator(task), requiredFields_(fields) {}

ValidatedTask::ValidatedTask(const ValidatedTask& other)
    : TaskDecorator(other), requiredFields_(other.requiredFields_) {}

ValidatedTask::~ValidatedTask() {}

void ValidatedTask::addRequiredField(const std::string& field) {
    requiredFields_.push_back(field);
}

std::vector<std::string> ValidatedTask::getMissingFields() const {
    std::vector<std::string> missing;
    for (std::size_t i = 0; i < requiredFields_.size(); ++i) {
        const std::string& key = requiredFields_[i];
        if (!hasData(key) || getData(key).empty()) missing.push_back(key);
    }
    return missing;
}

bool ValidatedTask::validate() const { return getMissingFields().empty(); }

void ValidatedTask::complete() {
    std::vector<std::string> missing = getMissingFields();
    if (!missing.empty()) {
        std::string list;
        for (std::size_t i = 0; i < missing.size(); ++i)
            list += (i ? ", " : "") + missing[i];
        throw std::runtime_error("Cannot complete '" + getName() +
                                 "': missing required data: " + list);
    }
    TaskDecorator::complete();
}

WorkItem* ValidatedTask::clone() const { return new ValidatedTask(*this); }
