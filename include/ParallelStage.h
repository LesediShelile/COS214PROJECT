#ifndef PARALLELSTAGE_H
#define PARALLELSTAGE_H
#include <string>
#include "WorkItem.h"

class ParallelStage{

public:
    ParallelStage(std::string itemID, std::string name);
    ParallelStage(const ParallelStage& other);
    void start();
    void cancel();
    void complete();
    void MakeAvailable();
    bool isFinished();
    bool isAvailable();
    WorkItem* clone();
    void activateChildren();
    ~ParallelStage();


};

#endif //PARALLELSTAGE_H