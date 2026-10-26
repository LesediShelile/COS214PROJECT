/**
 * @file AdapterOne.cpp
 * @brief Implements the adapter for APIOne.
 */

#include "AdapterOne.h"
#include "APIOne.h"

/**
 * @brief Constructs an AdapterOne.
 * @param adaptee API object to adapt.
 */
AdapterOne::AdapterOne(APIOne* adaptee) : adaptee(adaptee) {}

/**
 * @brief Translates a generic request into an APIOne operation.
 * @param operation Operation to perform.
 * @param payload Data for the operation.
 */
void AdapterOne::request(const std::string* operation,
                         const std::string* payload) {
    if (adaptee == nullptr || operation == nullptr || payload == nullptr) {
        return;
    }

    if (*operation == "add") {
        adaptee->addEntry(*payload, *payload);
    } else if (*operation == "remove") {
        adaptee->removeEntry(*payload);
    }
}

/**
 * @brief Returns the name of the adapted API.
 * @return Name of the API.
 */
std::string AdapterOne::getName() {
    return "API One";
}

/**
 * @brief Destroys the AdapterOne.
 */
AdapterOne::~AdapterOne() {}