#include "../include/AvailableItemsIterator.h"
#include "../include/DefinitionBuilder.h"
#include "../include/DepthFirstIterator.h"
#include "../include/ParallelStage.h"
#include "../include/Role.h"
#include "../include/RoleBasedAssignment.h"
#include "../include/SequentialStage.h"
#include "../include/Stage.h"
#include "../include/Task.h"
#include "../include/WorkflowDefinition.h"
#include "../include/WorkItem.h"

#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace
{
    int checks = 0;
    int failures = 0;

    void check(bool condition, const std::string& description)
    {
        ++checks;
        if (!condition)
        {
            ++failures;
            std::cerr << "FAIL: " << description << '\n';
        }
    }

    template <typename Action>
    void expectLogicError(const std::string& description, Action action)
    {
        ++checks;
        try
        {
            action();
            ++failures;
            std::cerr << "FAIL: " << description << " (no exception thrown)\n";
        }
        catch (const std::logic_error&)
        {
        }
        catch (const std::exception& error)
        {
            ++failures;
            std::cerr << "FAIL: " << description << " (wrong exception: "
                      << error.what() << ")\n";
        }
    }

    void testBuilderValidation()
    {
        Role role("nurse", 3);
        RoleBasedAssignment strategy;

        {
            DefinitionBuilder builder;
            expectLogicError("getResult before reset", [&builder]() {
                builder.getResult();
            });
            expectLogicError("begin stage before reset", [&builder]() {
                builder.beginSequentialStage("too early");
            });
            expectLogicError("add task before reset", [&builder, &role, &strategy]() {
                builder.addTask("too early", &role, &strategy);
            });
        }

        {
            DefinitionBuilder builder;
            builder.reset("empty", 1);
            expectLogicError("getResult for empty definition", [&builder]() {
                builder.getResult();
            });
            expectLogicError("add task without an open stage", [&builder, &role, &strategy]() {
                builder.addTask("orphan", &role, &strategy);
            });
            expectLogicError("commit without a pending task", [&builder]() {
                builder.commitPendingTask();
            });
            expectLogicError("endStage with no open stage", [&builder]() {
                builder.endStage();
            });
        }

        {
            DefinitionBuilder builder;
            builder.reset("pending", 1);
            builder.beginSequentialStage("root");
            builder.addTask("one", &role, &strategy);
            expectLogicError("add a second task before committing the first",
                [&builder, &role, &strategy]() {
                    builder.addTask("two", &role, &strategy);
                });
            expectLogicError("end a stage with a pending task", [&builder]() {
                builder.endStage();
            });
            expectLogicError("getResult while a stage is open", [&builder]() {
                builder.getResult();
            });

            builder.commitPendingTask();
            builder.endStage();
            std::unique_ptr<WorkflowDefinition> result(builder.getResult());
            check(result->getRoot() != nullptr,
                  "builder remains usable after rejected operations");
            expectLogicError("getResult transfers ownership only once", [&builder]() {
                builder.getResult();
            });
        }

        {
            DefinitionBuilder builder;
            builder.reset("duplicate-root", 1);
            builder.beginSequentialStage("root");
            builder.endStage();
            expectLogicError("cannot add a second root stage", [&builder]() {
                builder.beginParallelStage("second root");
            });
        }

        {
            DefinitionBuilder builder;
            builder.reset("discarded", 1);
            builder.beginSequentialStage("unfinished");
            builder.addTask("discard this pending task", &role, &strategy);
            builder.reset("replacement", 4);
            builder.beginParallelStage("new root");
            builder.addTask("replacement task", &role, &strategy);
            builder.commitPendingTask();
            builder.endStage();

            std::unique_ptr<WorkflowDefinition> result(builder.getResult());
            check(result->getName() == "replacement",
                  "reset replaces an unfinished definition");
            check(result->getVersion() == 4,
                  "reset applies the replacement version");
            check(result->getRoot()->getName() == "new root",
                  "reset clears unfinished stage state");
        }
    }

    void testBuilderTreeAndDepthFirstIterator()
    {
        Role role("nurse", 3);
        RoleBasedAssignment strategy;
        DefinitionBuilder builder;
        builder.reset("nested-workflow", 7);

        builder.beginSequentialStage("root");
        builder.addTask("first", &role, &strategy);
        builder.commitPendingTask();

        builder.beginParallelStage("parallel");
        builder.addTask("second", &role, &strategy);
        builder.commitPendingTask();
        builder.addTask("third", &role, &strategy);
        builder.commitPendingTask();
        builder.endStage();

        builder.addTask("last", &role, &strategy);
        builder.commitPendingTask();
        builder.endStage();

        std::unique_ptr<WorkflowDefinition> definition(builder.getResult());
        check(definition->getName() == "nested-workflow",
              "definition name is preserved");
        check(definition->getVersion() == 7,
              "definition version is preserved");

        WorkItem* root = definition->getRoot();
        check(root != nullptr, "built definition has a root");
        if (root == nullptr)
        {
            return;
        }

        check(dynamic_cast<SequentialStage*>(root) != nullptr,
              "requested root stage type is built");
        check(root->getName() == "root", "root stage name is preserved");

        const std::vector<WorkItem*> rootChildren = root->getChildren();
        check(rootChildren.size() == 3, "root contains its direct children");
        if (rootChildren.size() == 3)
        {
            check(rootChildren[0]->getName() == "first",
                  "first root child remains in insertion order");
            check(rootChildren[1]->getName() == "parallel",
                  "nested stage remains in insertion order");
            check(rootChildren[2]->getName() == "last",
                  "last root child remains in insertion order");
            check(dynamic_cast<ParallelStage*>(rootChildren[1]) != nullptr,
                  "nested stage has the requested parallel type");
            check(dynamic_cast<Task*>(rootChildren[0]) != nullptr,
                  "builder creates task leaves");
        }

        std::unique_ptr<WorkItemIterator> iterator(root->createIterator());
        const std::string expectedNames[] = {
            "root", "first", "parallel", "second", "third", "last"
        };
        const std::string expectedIDs[] = {"0", "1", "2", "3", "4", "5"};
        std::size_t visited = 0;

        for (iterator->first(); !iterator->isDone(); iterator->next())
        {
            WorkItem* item = iterator->current();
            check(item != nullptr, "depth-first iterator current item is non-null");
            if (item == nullptr)
            {
                continue;
            }

            if (visited < sizeof(expectedNames) / sizeof(expectedNames[0]))
            {
                check(item->getName() == expectedNames[visited],
                      "depth-first traversal order at position " +
                          std::to_string(visited));
                check(item->getItemID() == expectedIDs[visited],
                      "builder assigns sequential item IDs at position " +
                          std::to_string(visited));
            }
            ++visited;
        }

        check(visited == sizeof(expectedNames) / sizeof(expectedNames[0]),
              "depth-first iterator visits every node exactly once");
        check(iterator->current() == nullptr,
              "depth-first iterator has no current item after completion");

        iterator->first();
        check(!iterator->isDone() && iterator->current() == root,
              "first() restarts depth-first traversal at the root");
        iterator->next();
        check(iterator->current() == rootChildren[0],
              "next() advances from root to first child");

        DepthFirstIterator emptyIterator(nullptr);
        check(emptyIterator.isDone(), "null-root depth-first iterator is done");
        check(emptyIterator.current() == nullptr,
              "null-root depth-first iterator has no current item");
        emptyIterator.first();
        check(emptyIterator.isDone(),
              "first() keeps a null-root depth-first iterator done");
    }

    void testAvailableItemsIterator()
    {
        Role role("nurse", 3);
        RoleBasedAssignment strategy;
        DefinitionBuilder builder;
        builder.reset("sequential-availability", 1);
        builder.beginSequentialStage("sequence");
        builder.addTask("first", &role, &strategy);
        builder.commitPendingTask();
        builder.addTask("second", &role, &strategy);
        builder.commitPendingTask();
        builder.endStage();

        std::unique_ptr<WorkflowDefinition> definition(builder.getResult());
        WorkItem* root = definition->getRoot();
        const std::vector<WorkItem*> tasks = root->getChildren();
        check(tasks.size() == 2,
              "sequential builder creates exactly two direct tasks");
        if (tasks.size() != 2)
        {
            return;
        }

        root->makeAvailable();
        std::unique_ptr<WorkItemIterator> iterator(root->createAvailableIterator());
        check(!iterator->isDone(), "available iterator finds an available task");
        check(iterator->current() == tasks[0],
              "sequential stage exposes only its first task");

        iterator->first();
        check(iterator->current() == tasks[0],
              "available iterator first() restarts filtered traversal");

        tasks[0]->cancel();
        root->activateChildren();
        iterator->first();
        check(!iterator->isDone(),
              "available iterator finds the next task after progression");
        check(iterator->current() == tasks[1],
              "sequential stage exposes its next task after the first finishes");
        iterator->next();
        check(iterator->isDone(),
              "available iterator does not return unavailable or stage nodes");

        AvailableItemsIterator emptyAvailableIterator(nullptr);
        check(emptyAvailableIterator.isDone(),
              "null-root available iterator is done");
        check(emptyAvailableIterator.current() == nullptr,
              "null-root available iterator has no current item");
    }
}

int main()
{
    testBuilderValidation();
    testBuilderTreeAndDepthFirstIterator();
    testAvailableItemsIterator();

    std::cout << "Builder and iterator checks: " << checks
              << ", failures: " << failures << '\n';
    return failures == 0 ? 0 : 1;
}
