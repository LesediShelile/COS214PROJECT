/**
 * @file WorkflowDefinition.h
 * @brief Declaration of the WorkflowDefinition class representing process blueprints.
 * @author Kiara Ajodhaparsadh
 * @date 2026-10-27
 */

#ifndef WORKFLOWDEFINITION_H
#define WORKFLOWDEFINITION_H

#include <string>

class WorkItem;
class ApprovalHandler;
class WorkflowInstance;

/**
 * @class WorkflowDefinition
 * @brief Template blueprint describing the structure and behavior of a workflow process.
 * 
 * Acts as the Product of the Builder pattern and the client of the Prototype pattern.
 * Owns the root of a WorkItem tree and the head of an ApprovalHandler chain.
 * Once frozen, its internal structure becomes immutable, ensuring that any
 * WorkflowInstance created from it receives an exact, isolated clone of its tree.
 * A definition never executes directly: its tasks are never assigned or completed.
 * 
 * @note Instances cannot be generated until the definition is frozen.
 * @see WorkflowInstance
 * @see WorkItem
 * @see ApprovalHandler
 */
class WorkflowDefinition {
public:
    /**
     * @brief Constructs a new WorkflowDefinition.
     * @param[in] name The unique identifier/name for this workflow definition.
     * @param[in] version The revision/version number of this definition.
     */
    WorkflowDefinition(const std::string& name, int version);

    /**
     * @brief Destructor that deallocates the root WorkItem and the approval chain.
     */
    ~WorkflowDefinition();

    /**
     * @brief Deleted copy constructor to maintain single-point ownership of the tree.
     */
    WorkflowDefinition(const WorkflowDefinition&) = delete;

    /**
     * @brief Deleted copy assignment operator to prevent shared ownership.
     */
    WorkflowDefinition& operator=(const WorkflowDefinition&) = delete;

    /**
     * @brief Gets the name of the workflow definition.
     * @return std::string The workflow definition name.
     */
    std::string getName() const;

    /**
     * @brief Gets the version number of the workflow definition.
     * @return int The version number.
     */
    int getVersion() const;

    /**
     * @brief Gets the root WorkItem of the definition's composite structure.
     * @return WorkItem* Pointer to the root work item.
     */
    WorkItem* getRoot() const;

    /**
     * @brief Gets the head of the approval handler chain.
     * @return ApprovalHandler* Pointer to the first handler in the chain.
     */
    ApprovalHandler* getApprovalChain() const;

    /**
     * @brief Sets the root WorkItem for the workflow tree.
     * @param[in] root Pointer to the WorkItem root node.
     * @throws std::logic_error If the definition is already frozen.
     * @throws std::invalid_argument If the root parameter is null.
     */
    void setRoot(WorkItem* root);

    /**
     * @brief Sets the approval chain of responsibility head.
     * @param[in] handler Pointer to the head ApprovalHandler.
     * @throws std::logic_error If the definition is already frozen.
     */
    void setApprovalChain(ApprovalHandler* handler);

    /**
     * @brief Freezes the definition, locking it against further modifications.
     * @throws std::logic_error If no root work item has been set.
     */
    void freeze();

    /**
     * @brief Checks if the workflow definition has been frozen.
     * @return true If the definition is frozen and immutable, false otherwise.
     */
    bool isFrozen() const;

    /**
     * @brief Instantiates a new WorkflowInstance using the Prototype pattern.
     * @param[in] instanceID The unique ID to assign to the new instance.
     * @return WorkflowInstance* Pointer to the newly allocated instance (caller assumes ownership).
     * @throws std::logic_error If the definition is not yet frozen.
     */
    WorkflowInstance* createInstance(const std::string& instanceID);

private:
    std::string name;                ///< Unique name of the workflow definition.
    int version;                     ///< Version number of the workflow.
    WorkItem* root;                  ///< Root node of the WorkItem composite tree.
    ApprovalHandler* approvalChain;  ///< Head of the approval handler chain of responsibility.
    bool frozen;                     ///< Flag indicating whether structural modifications are locked.
};

#endif