/**
 * @file APIOne.h
 * @brief Declares a generic external API.
 */

#ifndef APIONE_H
#define APIONE_H

#include <map>
#include <string>

/**
 * @class APIOne
 * @brief Provides operations for managing external entries.
 */
class APIOne {
private:
    /** Stores entries using a key-value structure. */
    std::map<std::string, std::string> entries;

    /** Name of the external service. */
    std::string serviceName;

public:
    /**
     * @brief Constructs an APIOne object.
     */
    APIOne();

    /**
     * @brief Adds an entry to the external service.
     * @param key Unique key identifying the entry.
     * @param value Data associated with the key.
     */
    void addEntry(const std::string& key, const std::string& value);

    /**
     * @brief Removes an entry from the external service.
     * @param key Key identifying the entry to remove.
     */
    void removeEntry(const std::string& key);

    /**
     * @brief Destroys the APIOne object.
     */
    ~APIOne();
};

#endif // APIONE_H