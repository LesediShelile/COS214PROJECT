#include "../third_party/doctest.h"
#include "../include/TaskState.h"
#include "../include/Task.h"

class TestDefaultTaskState : public TaskState {
public:
    std::string getName() const override { return "TestState"; }
    TaskState* clone() const override { return new TestDefaultTaskState(*this); }
};

TEST_SUITE("TaskState default behaviour") {
    TEST_CASE("default state rejects assignment") {
        TestDefaultTaskState state;
        Task task("TS1", "Example task", nullptr, nullptr);
        CHECK_THROWS_AS(state.assign(&task, nullptr), std::logic_error);
    }

    TEST_CASE("default state rejects starting") {
        TestDefaultTaskState state;
        Task task("TS2", "Example task", nullptr, nullptr);
        CHECK_THROWS_AS(state.start(&task), std::logic_error);
    }

    TEST_CASE("default state rejects completion") {
        TestDefaultTaskState state;
        Task task("TS3", "Example task", nullptr, nullptr);
        CHECK_THROWS_AS(state.complete(&task), std::logic_error);
    }

    TEST_CASE("default state rejects rejection") {
        TestDefaultTaskState state;
        Task task("TS4", "Example task", nullptr, nullptr);
        CHECK_THROWS_AS(state.reject(&task, "Not approved"), std::logic_error);
    }

    TEST_CASE("default state rejects cancellation") {
        TestDefaultTaskState state;
        Task task("TS5", "Example task", nullptr, nullptr);
        CHECK_THROWS_AS(state.cancel(&task), std::logic_error);
    }

    TEST_CASE("default state rejects making a task available") {
        TestDefaultTaskState state;
        Task task("TS6", "Example task", nullptr, nullptr);
        CHECK_THROWS_AS(state.makeAvailable(&task), std::logic_error);
    }

    TEST_CASE("default state reports non-final and non-available") {
        TestDefaultTaskState state;
        CHECK_FALSE(state.isFinal());
        CHECK_FALSE(state.isAvailable());
        CHECK(state.getName() == "TestState");
        TaskState* copy = state.clone();
        REQUIRE(copy != nullptr);
        CHECK(copy->getName() == "TestState");
        delete copy;
    }
}
