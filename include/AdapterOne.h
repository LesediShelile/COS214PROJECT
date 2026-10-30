
#ifndef ADAPTERONE_H
#define ADAPTERONE_H

#include "ExternalConnector.h"

class APIOne;

class AdapterOne : public ExternalConnector {
private:
    APIOne* adaptee; // Non-owning pointer

public:
    explicit AdapterOne(APIOne* adaptee);

    void request(const std::string* operation,
                 const std::string* payload) override;

    std::string getName() override;

    ~AdapterOne() override;
};

#endif
