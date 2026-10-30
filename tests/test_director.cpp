// Tests for the generic Builder-pattern Director.

#include "doctest.h"

#include <memory>
#include <stdexcept>
#include <string>

#include "DefinitionBuilder.h"
#include "Director.h"
#include "ParallelStage.h"
#include "Role.h"
#include "RoleBasedAssignment.h"
#include "SequentialStage.h"
#include "Stage.h"
#include "WorkflowDefinition.h"

TEST_CASE("Director constructs a workflow from a client supplied recipe") {
    DefinitionBuilder builder;
    Director director(builder);
    Role role("Role A", 1);
    RoleBasedAssignment assignment;

    std::unique_ptr<WorkflowDefinition> definition(director.construct(
        "Generic workflow",
        7,
        [&](WorkflowBuilder& workflow) {
            workflow.beginSequentialStage("First stage");
            workflow.addTask("First client-defined task", &role, &assignment);
            workflow.commitPendingTask();
            workflow.addTask("Second client-defined task", &role, &assignment);
            workflow.withPriority(2);
            workflow.commitPendingTask();
            workflow.endStage();
        }
    ));

    REQUIRE(definition != nullptr);
    CHECK(definition->getName() == "Generic workflow");
    CHECK(definition->getVersion() == 7);

    Stage* root = dynamic_cast<Stage*>(definition->getRoot());
    REQUIRE(root != nullptr);
    CHECK(root->getChildren().size() == 2);
}

TEST_CASE("Director supports different stage structures without scenario-specific code") {
    DefinitionBuilder builder;
    Director director(builder);
    Role role("Generic role", 1);
    RoleBasedAssignment assignment;

    std::unique_ptr<WorkflowDefinition> definition(director.construct(
        "Parallel workflow",
        1,
        [&](WorkflowBuilder& workflow) {
            workflow.beginParallelStage("Concurrent stage");
            workflow.addTask("Task A", &role, &assignment);
            workflow.commitPendingTask();
            workflow.addTask("Task B", &role, &assignment);
            workflow.commitPendingTask();
            workflow.endStage();
        }
    ));

    REQUIRE(definition != nullptr);
    ParallelStage* root = dynamic_cast<ParallelStage*>(definition->getRoot());
    REQUIRE(root != nullptr);
    CHECK(root->getChildren().size() == 2);
}

TEST_CASE("Director can construct successive definitions using the same builder") {
    DefinitionBuilder builder;
    Director director(builder);
    Role role("Generic role", 1);
    RoleBasedAssignment assignment;

    std::unique_ptr<WorkflowDefinition> first(director.construct(
        "First definition", 1,
        [&](WorkflowBuilder& workflow) {
            workflow.beginSequentialStage("Stage one");
            workflow.addTask("Task one", &role, &assignment);
            workflow.commitPendingTask();
            workflow.endStage();
        }
    ));

    std::unique_ptr<WorkflowDefinition> second(director.construct(
        "Second definition", 2,
        [&](WorkflowBuilder& workflow) {
            workflow.beginSequentialStage("Stage two");
            workflow.addTask("Task two", &role, &assignment);
            workflow.commitPendingTask();
            workflow.endStage();
        }
    ));

    REQUIRE(first != nullptr);
    REQUIRE(second != nullptr);
    CHECK(first->getName() == "First definition");
    CHECK(second->getName() == "Second definition");
    CHECK(first->getVersion() == 1);
    CHECK(second->getVersion() == 2);
}

TEST_CASE("Director rejects an empty workflow name") {
    DefinitionBuilder builder;
    Director director(builder);

    CHECK_THROWS_AS(
        director.construct("", 1, [](WorkflowBuilder&) {}),
        std::invalid_argument
    );
}

TEST_CASE("Director rejects an empty construction recipe") {
    DefinitionBuilder builder;
    Director director(builder);
    std::function<void(WorkflowBuilder&)> emptyRecipe;

    CHECK_THROWS_AS(
        director.construct("Workflow", 1, emptyRecipe),
        std::invalid_argument
    );
}
