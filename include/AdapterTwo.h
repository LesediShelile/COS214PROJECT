/**
 * @file AdapterTwo.h
 * @brief Declares the AdapterTwo class.
 */

#ifndef ADAPTERTWO_H
#define ADAPTERTWO_H

#include "ExternalConnector.h"
#include <string>

/**
 * @class APITwo
 * @brief Forward declaration of the external API class.
 */
class APITwo;

/**
 * @class AdapterTwo
 * @brief Adapts APITwo to the ExternalConnector interface.
 *
 * Allows clients to interact with APITwo through the
 * ExternalConnector interface.
 */
class AdapterTwo : public ExternalConnector {
private:
    /** Pointer to the external API being adapted. */
    APITwo* adaptee;

public:
    /**
     * @brief Constructs an AdapterTwo object.
     */
    AdapterTwo();

    /**
     * @brief Executes an operation through the external API.
     * @param operation Operation to perform.
     * @param payload Data required for the operation.
     */
    void request(const std::string* operation, std::string* payload) override;

    /**
     * @brief Returns the name of the external API.
     * @return Name of the API.
     */
    std::string getName() override;

    /**
     * @brief Destroys the AdapterTwo object.
     */
    ~AdapterTwo();
};

#endif // ADAPTERTWO_H