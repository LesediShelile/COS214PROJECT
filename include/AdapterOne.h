#ifndef ADAPTERONE_H
#define ADAPTERONE_H
#include "ExternalConnector.h"
#include <string>

class APIOne;

class AdapterOne : public ExternalConnector {
private:
    APIOne* adaptee;

public:
    AdapterOne(APIOne* adaptee);

    void request(std::string* operation, std::string* payload) override;
    std::string getName() override;

    ~AdapterOne();
};

#endif // ADAPTERONE_H
