/**
 * @file AdapterTwo.cpp
 * @brief Implements the adapter for APITwo.
 */

#include "AdapterTwo.h"
#include "APITwo.h"

/**
 * @brief Constructs an AdapterTwo and its API object.
 */
AdapterTwo::AdapterTwo() : adaptee(new APITwo()) {}

/**
 * @brief Translates a generic request into an APITwo operation.
 * @param operation Operation to perform.
 * @param payload Data for the operation.
 */
void AdapterTwo::request(const std::string* operation,
                         const std::string* payload) {
    if (adaptee == nullptr || operation == nullptr || payload == nullptr) {
        return;
    }

    adaptee->setInfo(*operation, *payload);

    if (*operation == "add") {
        adaptee->add();
    } else if (*operation == "alert") {
        adaptee->alert();
    } else if (*operation == "dispatch") {
        adaptee->dispatch();
    }
}

/**
 * @brief Returns the name of the adapted API.
 * @return Name of the API.
 */
std::string AdapterTwo::getName() {
    return "API Two";
}

/**
 * @brief Destroys the AdapterTwo and its API object.
 */
AdapterTwo::~AdapterTwo() {
    delete adaptee;
}