#ifndef APPROVALHANDLER_H
#define APPROVALHANDLER_H

class ApprovalRequest;

class ApprovalHandler {

    private:

        ApprovalHandler* next = nullptr;
    
    public:

        virtual ~ApprovalHandler();

        virtual void setNext (ApprovalHandler* handler);

        virtual void handle (ApprovalRequest* request) = 0;

    protected:
        
        void forward (ApprovalRequest* request);
};
#endif