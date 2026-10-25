#ifndef SEQUENTIALSTAGE_H
#define SEQUENTIALSTAGE_H

#include "Stage.h"

/**
 * @brief Composite whose children run one after another: a child only becomes
 *        available once the previous child is finished.
 */
class SequentialStage : public Stage {
public:
    SequentialStage(const std::string& itemID, const std::string& name);
    SequentialStage(const SequentialStage& other);
    virtual ~SequentialStage();

    virtual void activateChildren();
    virtual WorkItem* clone() const;

private:
    std::size_t currentChild_;   ///< index of the child currently being worked
    bool currentActivated_;      ///< has that child been made available yet?
};

#endif
