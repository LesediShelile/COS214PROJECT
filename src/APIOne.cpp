/**
 * @file APIOne.cpp
 * @brief Implements the first external API.
 */

#include "APIOne.h"

/**
 * @brief Constructs an APIOne object.
 */
APIOne::APIOne() : serviceName("API One") {}

/**
 * @brief Adds an entry to the API.
 * @param key Key identifying the entry.
 * @param value Value associated with the key.
 */
void APIOne::addEntry(const std::string& key,
                      const std::string& value) {
    entries[key] = value;
}

/**
 * @brief Removes an entry from the API.
 * @param key Key identifying the entry.
 */
void APIOne::removeEntry(const std::string& key) {
    entries.erase(key);
}

/**
 * @brief Destroys the APIOne object.
 */
APIOne::~APIOne() {}