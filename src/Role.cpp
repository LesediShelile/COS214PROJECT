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

// Adds a permission if it has not already been added.
void Role::addPermission(const std::string& permission) {

    if (!hasPermission(permission)) {
        permissions.push_back(permission);
    }

}

// Checks whether this role contains a permission.
bool Role::hasPermission(const std::string& permission) const {

    for (std::size_t i = 0; i < permissions.size(); i++) {

        if (permissions[i] == permission) {
            return true;
        }

    }

    return false;
}