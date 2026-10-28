#ifndef APPROVALHANDLER_H
#define APPROVALHANDLER_H

class ApprovalRequest;

/**
 * @brief Abstract handler in the approval chain (Chain of Responsibility).
 *
 * Each handler either deals with an ApprovalRequest itself or
 * forwards it to the next handler in the chain.
 */
class ApprovalHandler {

    private:

        /** @brief The next handler in the chain (not owned). */
        ApprovalHandler* next = nullptr;
    
    public:

        /** @brief Virtual destructor so concrete handlers are cleaned up correctly. */
        virtual ~ApprovalHandler();

        /**
         * @brief Links this handler to the next one in the chain.
         * @param handler The handler that receives requests this one cannot deal with.
         */
        virtual void setNext (ApprovalHandler* handler);

        /**
         * @brief Handles an approval request or passes it on.
         * @param request The request to handle.
         */
        virtual void handle (ApprovalRequest* request) = 0;

    protected:
        
        /**
         * @brief Passes the request to the next handler in the chain.
         *
         * If there is no next handler, the request has reached the end of the chain.
         * @param request The request to pass on.
         */
        void forward (ApprovalRequest* request);
};
#endif