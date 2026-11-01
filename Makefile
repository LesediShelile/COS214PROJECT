
# make        Build the engine library and application (if main.cpp exists)
# make test    Build and run the doctest suite
# make run     Build and run the application
# make clean   Remove build output

CXX       ?= g++
CXXFLAGS  ?= -std=c++11 -Wall -Wextra -pedantic -g
CPPFLAGS  += -Iinclude -MMD -MP

BUILD     := build_make
SRC       := $(filter-out src/main.cpp,$(wildcard src/*.cpp))
OBJ       := $(SRC:src/%.cpp=$(BUILD)/obj/%.o)
LIB       := $(BUILD)/libtividy_core.a

MAIN      := src/main.cpp
HAS_MAIN  := $(wildcard $(MAIN))
MAIN_OBJ  := $(BUILD)/obj/main.o
APP_BIN   := $(BUILD)/tividy

TESTS     := $(wildcard tests/*.cpp)
TEST_OBJ  := $(TESTS:tests/%.cpp=$(BUILD)/obj/tests/%.o)
TEST_BIN  := $(BUILD)/tividy_tests

all: $(LIB) $(if $(HAS_MAIN),$(APP_BIN))

# Build the core library
$(LIB): $(OBJ)
	@mkdir -p $(dir $@)
	ar rcs $@ $^

# Compile source files
$(BUILD)/obj/%.o: src/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c $< -o $@

# Compile test files
$(BUILD)/obj/tests/%.o: tests/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -Itests/third_party -c $< -o $@

# Compile main.cpp separately
$(MAIN_OBJ): $(MAIN)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c $< -o $@

# Link the application
$(APP_BIN): $(MAIN_OBJ) $(LIB)
	$(CXX) $^ -o $@

# Link the tests
$(TEST_BIN): $(TEST_OBJ) $(LIB)
	$(CXX) $^ -o $@

test: $(TEST_BIN)
	./$(TEST_BIN)

run: $(APP_BIN)
	./$(APP_BIN)

clean:
	rm -rf $(BUILD)

.PHONY: all test clean run

-include $(OBJ:.o=.d) $(TEST_OBJ:.o=.d) $(MAIN_OBJ:.o=.d)
