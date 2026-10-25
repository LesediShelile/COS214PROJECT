#ifndef WORKFLOWDEFINITION_H
#define WORKFLOWDEFINITION_H

#include <string>

class WorkItem;
class ApprovalHandler;
class WorkflowInstance;

/*
 * A workflow definition: the template that describes how a process may behave.
 *
 * Product of the Builder and the client of Prototype. Owns the
 * root of a WorkItem tree and the head of the approval chain. Once frozen it no
 * longer changes, and every WorkflowInstance is created from a clone of its tree.
 * A definition never runs: its tasks are never started, assigned or completed.
 */
class WorkflowDefinition {
public:
    WorkflowDefinition(const std::string& name, int version);
    ~WorkflowDefinition();

    // Owns a tree and an approval chain, so copying is disabled.
    WorkflowDefinition(const WorkflowDefinition&) = delete;
    WorkflowDefinition& operator=(const WorkflowDefinition&) = delete;

    std::string getName() const;
    int getVersion() const;
    WorkItem* getRoot() const;
    ApprovalHandler* getApprovalChain() const;

   
    void setRoot(WorkItem* root);
    void setApprovalChain(ApprovalHandler* handler);

   
    void freeze();
    bool isFrozen() const;

    // Prototype: creates a new instance holding its own clone of the tree.
    // The caller owns the result. 
    WorkflowInstance* createInstance(const std::string& instanceID);

private:
    std::string name;
    int version;
    WorkItem* root;
    ApprovalHandler* approvalChain;
    bool frozen;
};

#endif