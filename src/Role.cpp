#include "Role.h"

Role::Role(std::string name, int authorityLevel) : name(name), authorityLevel(authorityLevel) {}

Role::~Role() {}

std::string Role::getName() const {

    return name;

}

int Role::getAuthorityLevel() const {

    return authorityLevel;

}

// Two roles are the same if they have the same name
bool Role::equals(Role* other) const {

    if (other == nullptr) {

        return false;
        
    }
    return name == other->name;
}