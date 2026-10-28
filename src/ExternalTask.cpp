/**
 * @file ExternalTask.cpp
 * @brief Implements the ExternalTask class.
 */

#include "ExternalTask.h"
#include "ExternalConnector.h"

/**
 * @brief Constructs an ExternalTask.
 * @param ItemID Unique identifier for the task.
 * @param name Name of the task.
 * @param role Role associated with the task.
 * @param strategy Strategy used to assign the task.
 * @param connector External service connector.
 * @param request Request to send to the external service.
 */
ExternalTask::ExternalTask(
    const std::string* ItemID,
    const std::string* name,
    Role* role,
    AssignmentStrategy* strategy,
    ExternalConnector* connector,
    const std::string* request)
    : Task(ItemID, name, role, strategy),
      connector(connector),
      request(request != nullptr ? *request : ""),
      response("") {
}

/**
 * @brief Updates the request.
 * @param request New request.
 */
void ExternalTask::setRequest(const std::string* request) {
    if (request != nullptr) {
        this->request = *request;
    }
}

/**
 * @brief Executes the request through the connector.
 * @return True if the request was submitted successfully.
 */
bool ExternalTask::execute() {
    if (connector == nullptr || request.empty()) {
        return false;
    }

    std::string operation = request;
    std::string payload = request;

    connector->request(&operation, &payload);

    response = "Request submitted to " + connector->getName();

    return true;
}

/**
 * @brief Returns the stored response.
 * @return Stored response.
 */
std::string ExternalTask::getResponse() {
    return response;
}

/**
 * @brief Destroys the ExternalTask.
 */
ExternalTask::~ExternalTask() {
}