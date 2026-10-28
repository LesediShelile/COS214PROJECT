/**
 * @file ExternalConnector.h
 * @brief Declares the common interface for external API adapters.
 */

#ifndef EXTERNALCONNECTOR_H
#define EXTERNALCONNECTOR_H

#include <string>

/**
 * @class ExternalConnector
 * @brief Defines the interface used to communicate with external services.
 *
 * Adapters implement this interface to translate generic requests
 * into operations supported by their external APIs.
 */
class ExternalConnector {
public:
    /**
     * @brief Constructs an ExternalConnector object.
     */
    ExternalConnector();

    /**
     * @brief Executes a request through the external service.
     * @param operation Operation to perform.
     * @param payload Data required for the operation.
     */
    virtual void request(const std::string* operation,
                         const std::string* payload) = 0;

    /**
     * @brief Returns the name of the external service.
     * @return Name of the service.
     */
    virtual std::string getName() = 0;

    /**
     * @brief Destroys the ExternalConnector object.
     */
    virtual ~ExternalConnector();
};

#endif // EXTERNALCONNECTOR_H