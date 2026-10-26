#ifndef ADAPTERONE_H
#define ADAPTERONE_H
#include "ExternalConnector.h"
#include <string>

class APIOne;


class AdapterOne : public ExternalConnector {

    private:
        APIOne* adaptee;
    
    public:

        void request(std::string operation, std::string payload);
        std::string getName();
        ~AdapterOne();
    

};


#endif //ADAPTERONE_H