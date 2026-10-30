
#include "AdapterOne.h"
#include "APIOne.h"

#include <stdexcept>

AdapterOne::AdapterOne(APIOne* adaptee)
    : adaptee(adaptee) {
    if (this->adaptee == nullptr) {
        throw std::invalid_argument(
            "AdapterOne requires a valid APIOne object."
        );
    }
}

void AdapterOne::request(
    const std::string* operation,
    const std::string* payload) {

    if (operation == nullptr || payload == nullptr) {
        throw std::invalid_argument(
            "Operation and payload cannot be null."
        );
    }

    if (*operation == "add") {
        const std::size_t separator = payload->find('=');

        if (separator == std::string::npos) {
            throw std::invalid_argument(
                "Add payload must use key=value format."
            );
        }

        adaptee->addEntry(
            payload->substr(0, separator),
            payload->substr(separator + 1)
        );
    }
    else if (*operation == "remove") {
        adaptee->removeEntry(*payload);
    }
    else {
        throw std::invalid_argument(
            "Unsupported APIOne operation."
        );
    }
}

std::string AdapterOne::getName() {
    return "API One";
}

AdapterOne::~AdapterOne() = default;
