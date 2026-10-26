#ifndef ROLE_H
#define ROLE_H

#include <string>

class Role {
public:

    Role(std::string name, int authorityLevel);

    ~Role();

    std::string getName() const;

    int getAuthorityLevel() const;

    bool equals(Role* other) const;

private:

    std::string name;
    
    int authorityLevel;
};

#endif