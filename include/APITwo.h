/**
 * @file APITwo.h
 * @brief Declares a second external API.
 */

#ifndef APITWO_H
#define APITWO_H

#include <string>

/**
 * @class APITwo
 * @brief Provides operations for sending information and triggering actions
 * through an external service.
 */
class APITwo {
private:
    /** Parameter describing the requested operation. */
    std::string description;

    /** Address associated with the operation. */
    std::string address;

public:
    /**
     * @brief Constructs an APITwo object.
     */
    APITwo();

    /**
     * @brief Adds information to the external service.
     */
    void add();

    /**
     * @brief Sends an alert through the external service.
     */
    void alert();

    /**
     * @brief Sets the information required for an operation.
     * @param description Parameter describing the operation.
     * @param address Address associated with the operation.
     */
    void setInfo(const std::string& param, const std::string& address);

    /**
     * @brief Dispatches the requested operation.
     */
    void dispatch();

    /**
     * @brief Destroys the APITwo object.
     */
    ~APITwo();
};

#endif // APITWO_H