#ifndef VALIDATEDTASK_H
#define VALIDATEDTASK_H

#include <string>
#include <vector>
#include "TaskDecorator.h"

/**
 * @brief Concrete Decorator: refuses to complete a work item until every
 *        required data field has been filled in.
 *
 * Example: a doctor cannot complete "Assessment and treatment" until
 * "diagnosis" and "disposition" are set. Every other operation is forwarded
 * unchanged. The required fields are plain data, so the engine and the
 * scenario never need a ValidatedTask subclass per rule.
 */
class ValidatedTask : public TaskDecorator {
public:
    /** @param task item to wrap (ownership passes to the decorator).
     *  @param fields data keys that must be present and non-empty. */
    ValidatedTask(WorkItem* task, const std::vector<std::string>& fields);
    ValidatedTask(const ValidatedTask& other);
    virtual ~ValidatedTask();

    /** @brief Add another required data key. */
    void addRequiredField(const std::string& field);
    /** @return true when every required field is present and non-empty. */
    bool validate() const;
    /** @return the required fields that are still missing or empty. */
    std::vector<std::string> getMissingFields() const;

    /** @brief Completes the wrapped item only if validate() holds.
     *  @throws std::runtime_error naming the missing fields otherwise;
     *          the wrapped item is left untouched. */
    virtual void complete();

    virtual WorkItem* clone() const;

private:
    std::vector<std::string> requiredFields_;
};

#endif
