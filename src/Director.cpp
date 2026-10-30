
#include "Director.h"

#include <stdexcept>

#include "WorkflowDefinition.h"

Director::Director(WorkflowBuilder& builder)
    : builder(builder) {}

WorkflowDefinition* Director::construct(
    const std::string& name,
    int version,
    const std::function<void(WorkflowBuilder&)>& recipe
) {
    if (name.empty()) {
        throw std::invalid_argument("Workflow name cannot be empty");
    }
    if (!recipe) {
        throw std::invalid_argument("Workflow construction recipe cannot be empty");
    }

    builder.reset(name, version);
    recipe(builder);
    return builder.getResult();
}

