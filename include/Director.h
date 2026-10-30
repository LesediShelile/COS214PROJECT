#ifndef DIRECTOR_H
#define DIRECTOR_H

#include <functional>
#include <string>

#include "WorkflowBuilder.h"

class WorkflowDefinition;

/**
 * @brief Builder-pattern Director that coordinates construction without
 *        prescribing any particular workflow scenario.
 *
 * The client supplies a construction recipe. The Director resets the supplied
 * builder, executes that recipe, and returns the resulting definition.
 * The builder is non-owning and must outlive the Director.
 */
class Director {
public:
    explicit Director(WorkflowBuilder& builder);

    /**
     * @brief Builds a workflow using a client-supplied recipe.
     * @param name Workflow definition name.
     * @param version Workflow definition version.
     * @param recipe Operations used to construct the workflow.
     * @return Newly created definition; ownership transfers to the caller.
     * @throws std::invalid_argument if the name is empty or recipe is empty.
     * @throws Any exception raised by the builder or recipe.
     */
    WorkflowDefinition* construct(
        const std::string& name,
        int version,
        const std::function<void(WorkflowBuilder&)>& recipe
    );

private:
    WorkflowBuilder& builder; // non-owning; caller controls its lifetime
};

#endif // DIRECTOR_H
