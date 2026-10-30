#ifndef ROLE_H
#define ROLE_H

#include <string>
#include <vector>
/**
 * @brief A job role with an authority level, e.g. Nurse (1) or Doctor (2).
 *
 * Used to decide who can be assigned a task and who can approve requests.
 */
class Role {
public:

    /**
     * @brief Creates a role.
     * @param name The name of the role, e.g. "Nurse".
     * @param authorityLevel How much approval authority the role has. Higher means more.
     */
    Role(std::string name, int authorityLevel);

    /** @brief Destructor. */
    ~Role();

    /**
     * @brief Gets the name of the role.
     * @return The role's name.
     */
    std::string getName() const;

    /**
     * @brief Gets the role's authority level.
     * @return The authority level.
     */
    int getAuthorityLevel() const;

    /**
     * @brief Checks whether two roles are the same.
     * @param other The role to compare with.
     * @return True if both roles have the same name, false if other is nullptr.
     */
    bool equals(Role* other) const;

     /**
     * @brief Adds a permission to this role.
     */
    void addPermission(const std::string& permission);

    /**
     * @brief Checks whether this role has a permission.
     */
    bool hasPermission(const std::string& permission) const;

private:

    /** @brief The name of the role. */
    std::string name;
    
    /** @brief How much approval authority the role has. */
    int authorityLevel;

     /** @brief Operations this role is allowed to perform. */
    std::vector<std::string> permissions;
};

#endif