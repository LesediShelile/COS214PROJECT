/**
 * @file APITwo.cpp
 * @brief Implements the second external API.
 */

#include "APITwo.h"

/**
 * @brief Constructs an APITwo object.
 */
APITwo::APITwo() : description(""), address("") {}

/**
 * @brief Adds information to the external service.
 */
void APITwo::add() {
    // Simulates adding information to the external service.
}

/**
 * @brief Sends an alert through the external service.
 */
void APITwo::alert() {
    // Simulates sending an alert.
}

/**
 * @brief Sets the information for an operation.
 * @param param Parameter describing the operation.
 * @param address Address associated with the operation.
 */
void APITwo::setInfo(const std::string& description,
                     const std::string& address) {
    this->description = description
    ;
    this->address = address;
}

/**
 * @brief Dispatches the requested operation.
 */
void APITwo::dispatch() {
    // Simulates dispatching the requested operation.
}

/**
 * @brief Destroys the APITwo object.
 */
APITwo::~APITwo() {}