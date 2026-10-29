#include "ApprovalHandler.h"
#include "ApprovalRequest.h"
#include <iostream>

ApprovalHandler::~ApprovalHandler() {
  /**
  * @brief each handler owns its successor, so deleting the head deletes the chain
  */
       delete next;
}

void ApprovalHandler::setNext(ApprovalHandler* handler) {

    next = handler;

}

void ApprovalHandler::forward(ApprovalRequest* request) {

    if (next != nullptr) {

        next->handle(request);

    } else {

        std::cout << "End of approval chain: nobody could approve this request" << std::endl;
        
    }

}



/*#ifndef APPROVALHANDLER_H
#define APPROVALHANDLER_H

class ApprovalRequest;

class ApprovalHandler {

    private:

        ApprovalHandler* next = nullptr;
    
    public:

        virtual ~ApprovalHandler();

        virtual void setNext (ApprovalHandler* handler);

        virtual void handle (ApprovalRequest* request);

        void forward (ApprovalRequest* request);
};
#endif*/