/**
 * @file ExternalTask.h
 * @brief Declares a work item that interacts with an external service.
 */

#ifndef EXTERNALTASK_H
#define EXTERNALTASK_H

#include <string>

class WorkItem;
class Role;
class AssignmentStrategy;
class ExternalConnector;

/**
 * @class ExternalTask
 * @brief Represents a task that sends requests to an external service.
 *
 * Uses an ExternalConnector to execute requests and store responses.
 */
class ExternalTask {
private:
    /** Connector used to communicate with the external service. */
    ExternalConnector* connector;

    /** Request to be sent to the external service. */
    std::string request;

    /** Response received from the external service. */
    std::string response;

public:
    /**
     * @brief Constructs an ExternalTask.
     * @param ItemID Unique identifier for the task.
     * @param name Name of the task.
     * @param role Role associated with the task.
     * @param strategy Strategy used to assign the task.
     * @param connector External service connector.
     * @param request Request to send to the external service.
     */
    ExternalTask(const std::string* ItemID,
                 const std::string* name,
                 Role* role,
                 AssignmentStrategy* strategy,
                 ExternalConnector* connector,
                 const std::string* request);

    /**
     * @brief Creates a copy of this task.
     * @return Pointer to the cloned WorkItem.
     */
    WorkItem* clone();

    /**
     * @brief Updates the request sent to the external service.
     * @param request New request.
     */
    void setRequest(const std::string* request);

    /**
     * @brief Executes the external request.
     * @return True if execution succeeds, otherwise false.
     */
    bool execute();

    /**
     * @brief Retrieves the response from the external service.
     * @return The stored response.
     */
    std::string getResponse();

    /**
     * @brief Destroys the ExternalTask.
     */
    ~ExternalTask();
};

#endif // EXTERNALTASK_H