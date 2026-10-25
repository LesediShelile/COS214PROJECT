#ifndef WORKITEM_H
#define WORKITEM_H
#include <map>
#include <string>
#include "Participant.h"


class WorkItem{

    protected:
        std::string itemID;
        std::string name;
        std::map<std::string,std::string> data;
    
    protected:
        WorkItem(std::string itemID, std::string name);
        std::string getItemID();
        std::string getName();
        void setData(std::string key, std::string value);
        std::string getData(std::string key);
        bool hasData(std::string key);
        virtual void makeAvailable() =0;
        virtual void assign(Participant* participant)=0;
        virtual void start() =0;
        virtual void complete() =0;
        virtual void reject(std::string reason)=0;
        virtual void cancel()=0;
        virtual bool isFinished()=0;
        virtual bool isAvailable()=0;
        virtual WorkItem* clone()=0;
        virtual WorkItemIterator* createIterator()=0;
        virtual TaskMemento* createMemento()=0;
        virtual void restoreMemento(TaskMemento* memento)=0;
        std::string getStateName();
        virtual WorkItemIterator* createAvailableIterator()=0;


        virtual ~WorkItem() =0;





        
};

#endif //WORKITEM_H