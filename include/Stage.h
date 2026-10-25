#ifndef STAGE_H
#define STAGE_H
#include <vector>
#include <string>
#include "WorkItem.h"
#include "Participant.h"
#include "WorkItemIterator.h"

class Stage{

    protected:
        std::vector<WorkItem*> children;
    
    protected:
        Stage(std::string itemID, std::string name);
    
    public:
        void assign(Participant* participant);
        void reject(std::string reason);
        WorkItem* clone();
        WorkItemIterator* createIterator();
        TaskMemento* createMemento();
        void restoreMemento(TaskMemento* memento);
        virtual void start()=0;
         virtualvoid cancel()=0;
        virtual void complete()=0;
        virtual void makeAvailable()=0;
        virtual bool isFinished()=0;
        virtual bool isAvailable()=0;
        void add(WorkIteam* item);
        std::vector<WorkIteam*> getChildren();
        void activateChildren();
        std::string getStateName();
        WorkIteamIterator* createAvailableIterator();
        virtual ~Stage()=0;

};


#endif