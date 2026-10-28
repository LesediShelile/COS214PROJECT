#ifndef STAGE_H
#define STAGE_H

#include <string>
#include <vector>
#include "WorkItem.h"

/**
 * @brief Composite of the Composite pattern: a WorkItem that owns child
 *        WorkItems (Tasks or other Stages).
 *
 * Abstract: SequentialStage and ParallelStage decide in which order the
 * children become available (activateChildren()).
 *
 * Design decision: assign/start/complete/reject act on ONE unit of work, so a
 * Stage refuses them with std::logic_error. Act on its Tasks instead.
 * makeAvailable() starts the stage, cancel() cancels every unfinished child,
 * and isFinished()/isAvailable() aggregate over the children.
 */
class Stage : public WorkItem {
public:
    virtual ~Stage();

    /** @brief Add a child. The stage takes ownership. @throws std::invalid_argument on NULL. */
    void add(WorkItem* item);
    virtual std::vector<WorkItem*> getChildren() const;

    virtual void makeAvailable();
    virtual void assign(Participant* participant);
    virtual void start();
    virtual void complete();
    virtual void reject(const std::string& reason);
    virtual void cancel();

    /** @return true when every child is finished (an empty stage is finished). */
    virtual bool isFinished() const;
    /** @return true when at least one child is available. */
    virtual bool isAvailable() const;
    /** @return "Pending", "InProgress" or "Finished". */
    virtual std::string getStateName() const;

    /** @brief Stages hold no state of their own: returns NULL. */
    virtual TaskMemento* createMemento();
    virtual void restoreMemento(TaskMemento* memento);

    /** @brief Release the next children according to the stage's ordering rule.
     *  Call again after any child finishes. */
    virtual void activateChildren() = 0;

protected:
    Stage(const std::string& itemID, const std::string& name);
    /** Deep copy: clones every child. Used by clone() of the concrete stages. */
    Stage(const Stage& other);

    std::vector<WorkItem*> children_;
    bool activated_;

private:
    Stage& operator=(const Stage&);
};

#endif
