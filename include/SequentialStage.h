#ifndef SEQUENTIALSTAGE_H
#define SEQUENTIALSTAGE_H
#include "Stage.h"
#include "WorkItem.h"

class SequentialStage : public Stage{

    private:
        WorkItem* currentChild;
    public:
        SequentialStage(std::string itemID, std::string name);
        SequentialStage(const SequentialStage& other);
        void start();
        void cancel();
        void makeAvailable();
        bool isFinished();
        bool isAvailable();
        void activateChildren();
        WorkItem* clone();
        ~SequentialStage();
        
    
};

#endif //SEQUENTIALSTAGE_H