
#include "AdapterTwo.h"
#include "APITwo.h"

#include <stdexcept>

AdapterTwo::AdapterTwo(APITwo* adaptee)
    : adaptee(adaptee) {
    if (this->adaptee == nullptr) {
        throw std::invalid_argument(
            "AdapterTwo requires a valid APITwo object."
        );
    }
}

void AdapterTwo::request(
    const std::string* operation,
    const std::string* payload) {

    if (operation == nullptr || payload == nullptr) {
        throw std::invalid_argument(
            "Operation and payload cannot be null."
        );
    }

    adaptee->setInfo(*payload, "");

    if (*operation == "add") {
        adaptee->add();
    }
    else if (*operation == "alert") {
        adaptee->alert();
    }
    else if (*operation == "dispatch") {
        adaptee->dispatch();
    }
    else {
        throw std::invalid_argument(
            "Unsupported APITwo operation."
        );
    }
}

std::string AdapterTwo::getName() {
    return "API Two";
}

AdapterTwo::~AdapterTwo() = default;
