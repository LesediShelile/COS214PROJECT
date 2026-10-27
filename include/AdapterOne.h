/**
 * @file AdapterOne.h
 * @brief Declares the AdapterOne class.
 */

#ifndef ADAPTERONE_H
#define ADAPTERONE_H

#include "ExternalConnector.h"
#include <string>

/**
 * @class APIOne
 * @brief Forward declaration of the external API class.
 */
class APIOne;

/**
 * @class AdapterOne
 * @brief Adapts APIOne to the ExternalConnector interface.
 *
 * Allows clients to interact with APIOne through the
 * ExternalConnector interface.
 */
class AdapterOne : public ExternalConnector {
private:
    /** Pointer to the external API being adapted. */
    APIOne* adaptee;

public:
    /**
     * @brief Constructs an AdapterOne object.
     * @param adaptee Pointer to the API object being adapted.
     */
    AdapterOne(APIOne* adaptee);

    /**
     * @brief Executes an operation through the external API.
     * @param operation Operation to perform.
     * @param payload Data required for the operation.
     */
    void request(std::string* operation, std::string* payload) override;

    /**
     * @brief Returns the name of the external API.
     * @return Name of the API.
     */
    std::string getName() override;

    /**
     * @brief Destroys the AdapterOne object.
     */
    ~AdapterOne();
};

#endif // ADAPTERONE_H