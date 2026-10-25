#ifndef PARALLELSTAGE_H
#define PARALLELSTAGE_H

#include "Stage.h"

/**
 * @brief Composite whose children are all available at the same time. The
 *        stage is finished when every child is.
 */
class ParallelStage : public Stage {
public:
    ParallelStage(const std::string& itemID, const std::string& name);
    ParallelStage(const ParallelStage& other);
    virtual ~ParallelStage();

    virtual void activateChildren();
    virtual WorkItem* clone() const;
};

#endif
