#   make          build the engine library
#   make test     build and run the doctest suite
#   make clean    remove build output

CXX      ?= g++
CXXFLAGS ?= -std=c++11 -Wall -Wextra -pedantic -g
CPPFLAGS += -Iinclude -MMD -MP

BUILD    := build_make
SRC      := $(filter-out src/main.cpp,$(wildcard src/*.cpp))
OBJ      := $(SRC:src/%.cpp=$(BUILD)/obj/%.o)
LIB      := $(BUILD)/libtividy_core.a

TESTS    := $(wildcard tests/*.cpp)
TEST_OBJ := $(TESTS:tests/%.cpp=$(BUILD)/obj/tests/%.o)
TEST_BIN := $(BUILD)/tividy_tests

HAS_MAIN := $(wildcard src/main.cpp)
APP_BIN  := $(BUILD)/tividy

all: $(LIB) $(if $(HAS_MAIN),$(APP_BIN))

$(LIB): $(OBJ)
	@mkdir -p $(dir $@)
	ar rcs $@ $^

$(BUILD)/obj/%.o: src/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c $< -o $@

$(BUILD)/obj/tests/%.o: tests/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -Itests/third_party -c $< -o $@

$(APP_BIN): $(BUILD)/obj/main.o $(LIB)
	$(CXX) $^ -o $@

$(TEST_BIN): $(TEST_OBJ) $(LIB)
	$(CXX) $^ -o $@

test: $(TEST_BIN)
	./$(TEST_BIN)

clean:
	rm -rf $(BUILD)

.PHONY: all test clean

-include $(OBJ:.o=.d) $(TEST_OBJ:.o=.d) $(BUILD)/obj/main.d
