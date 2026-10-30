
#ifndef ADAPTERTWO_H
#define ADAPTERTWO_H

#include "ExternalConnector.h"

class APITwo;

class AdapterTwo : public ExternalConnector {
private:
    APITwo* adaptee; // Non-owning pointer

public:
    explicit AdapterTwo(APITwo* adaptee);

    void request(const std::string* operation,
                 const std::string* payload) override;

    std::string getName() override;

    ~AdapterTwo() override;
};

#endif
