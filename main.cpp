/**
 * @file main.cpp
 * @brief Interactive, Menu-Driven Terminal User Interface (TUI) for TIVIDY.
 * 
 * Replicates the full scenario demonstrations without hardcoded configurations.
 * Allows users to dynamically configure personnel quantities, select or create custom tasks,
 * organize sequential and parallel execution groups, apply GoF decorators, execute workflows,
 * and persist reusable custom scenarios across sessions.
 */

#include <iostream>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <algorithm>
#include <fstream>
#include <ctime>
#include <cctype>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>

#ifdef _WIN32
#include <windows.h>
#endif

// TIVIDY Core Engine Includes
#include "Role.h"
#include "Participant.h"
#include "AssignmentStrategy.h"
#include "RoleBasedAssignment.h"
#include "WorkloadAssignment.h"
#include "RuleBasedAssignment.h"
#include "ExternalConnector.h"
#include "APIOne.h"
#include "APITwo.h"
#include "AdapterOne.h"
#include "AdapterTwo.h"
#include "EventPublisher.h"
#include "AuditLogger.h"
#include "Notifier.h"
#include "MetricsCollector.h"
#include "ApprovalLevel.h"
#include "ApprovalRequest.h"
#include "ApprovalStatus.h"
#include "CommandInvoker.h"
#include "WorkflowDefinition.h"
#include "WorkflowInstance.h"
#include "WorkItem.h"
#include "Task.h"
#include "ExternalTask.h"
#include "TaskDecorator.h"
#include "PriorityTask.h"
#include "ValidatedTask.h"
#include "AuditedTask.h"
#include "Stage.h"
#include "SequentialStage.h"
#include "ParallelStage.h"
#include "WorkItemIterator.h"
#include "AvailableItemsIterator.h"
#include "AssignCommand.h"
#include "StartCommand.h"
#include "CompleteCommand.h"
#include "RejectCommand.h"
#include "WorkflowEvent.h"
#include "EventType.h"

namespace {

// ===========================================================================
// ANSI Terminal Styling and Helpers
// ===========================================================================
const char* RESET   = "\033[0m";
const char* BOLD    = "\033[1m";
const char* DIM     = "\033[2m";
const char* CYAN    = "\033[36m";
const char* BLUE    = "\033[34m";
const char* GREEN   = "\033[32m";
const char* YELLOW  = "\033[33m";
const char* RED     = "\033[31m";
const char* MAGENTA = "\033[35m";
const char* WHITE   = "\033[97m";

void clearScreen() {
#ifdef _WIN32
    std::system("cls");
#else
    std::cout << "\033[2J\033[H";
#endif
}

void printRule(int width = 72, const char* colour = DIM) {
    std::cout << colour;
    for (int i = 0; i < width; ++i) std::cout << "─";
    std::cout << RESET << "\n";
}

void printBanner() {
    std::cout << CYAN << BOLD
              << "  ████████╗██╗██╗   ██╗██╗██████╗ ██╗   ██╗\n"
              << "  ╚══██╔══╝██║██║   ██║██║██╔══██╗╚██╗ ██╔╝\n"
              << "     ██║   ██║██║   ██║██║██║  ██║ ╚████╔╝ \n"
              << "     ██║   ██║╚██╗ ██╔╝██║██║  ██║  ╚██╔╝  \n"
              << "     ██║   ██║ ╚████╔╝ ██║██████╔╝   ██║   \n"
              << "     ╚═╝   ╚═╝  ╚═══╝  ╚═╝╚═════╝    ╚═╝   \n"
              << RESET << DIM
              << "  Interactive Menu-Driven Workflow Engine Terminal\n"
              << RESET;
}

void printHeader(const std::string& title, const std::string& subtitle = "") {
    clearScreen();
    std::cout << "\n";
    printBanner();
    printRule(72, CYAN);
    std::cout << "  " << WHITE << BOLD << "▶  " << title << RESET << "\n";
    if (!subtitle.empty()) {
        std::cout << "  " << DIM << subtitle << RESET << "\n";
    }
    printRule(72, DIM);
    std::cout << "\n";
}

void pause(const std::string& prompt = "Press ENTER to continue...") {
    std::cout << "\n  " << DIM << "↩ " << prompt << RESET;
    std::string line;
    std::getline(std::cin, line);
}

std::string pad(const std::string& str, std::size_t width) {
    if (str.length() >= width) return str.substr(0, width);
    return str + std::string(width - str.length(), ' ');
}

int promptChoice(const std::string& prompt, int minVal, int maxVal) {
    while (true) {
        std::cout << prompt;
        std::string input;
        if (!std::getline(std::cin, input)) return minVal;
        size_t first = input.find_first_not_of(" \t\r\n");
        if (first == std::string::npos) {
            std::cout << "  " << RED << "Please enter a choice (" << minVal << " - " << maxVal << ")." << RESET << "\n";
            continue;
        }
        size_t last = input.find_last_not_of(" \t\r\n");
        std::string trimmed = input.substr(first, last - first + 1);
        if (trimmed == "b" || trimmed == "B" || trimmed == "q" || trimmed == "Q") return 0;
        std::stringstream ss(trimmed);
        int val;
        char extra;
        if ((ss >> val) && !(ss >> extra) && val >= minVal && val <= maxVal) return val;
        std::cout << "  " << RED << "Invalid selection! Please enter a number between " << minVal << " and " << maxVal << "." << RESET << "\n";
    }
}

int promptInt(const std::string& prompt, int defaultVal, int minVal = 0, int maxVal = 9999) {
    while (true) {
        std::cout << prompt;
        std::string input;
        if (!std::getline(std::cin, input)) return defaultVal;
        size_t first = input.find_first_not_of(" \t\r\n");
        if (first == std::string::npos) return defaultVal;
        size_t last = input.find_last_not_of(" \t\r\n");
        std::string trimmed = input.substr(first, last - first + 1);
        std::stringstream ss(trimmed);
        int val;
        char extra;
        if ((ss >> val) && !(ss >> extra) && val >= minVal && val <= maxVal) return val;
        std::cout << "  " << RED << "Please enter an integer between " << minVal << " and " << maxVal << " (default " << defaultVal << ")." << RESET << "\n";
    }
}

std::string promptString(const std::string& prompt, const std::string& defaultVal = "", bool allowEmpty = false) {
    while (true) {
        std::cout << prompt;
        std::string input;
        if (!std::getline(std::cin, input)) return defaultVal;
        size_t first = input.find_first_not_of(" \t\r\n");
        if (first == std::string::npos) {
            if (!defaultVal.empty()) return defaultVal;
            if (allowEmpty) return "";
            std::cout << "  " << RED << "Input cannot be blank. Please try again." << RESET << "\n";
            continue;
        }
        size_t last = input.find_last_not_of(" \t\r\n");
        return input.substr(first, last - first + 1);
    }
}

bool promptBool(const std::string& prompt, bool defaultVal = true) {
    while (true) {
        std::cout << prompt << (defaultVal ? " [Y/n]: " : " [y/N]: ");
        std::string input;
        if (!std::getline(std::cin, input)) return defaultVal;
        size_t first = input.find_first_not_of(" \t\r\n");
        if (first == std::string::npos) return defaultVal;
        char c = static_cast<char>(std::tolower(input[first]));
        if (c == 'y') return true;
        if (c == 'n') return false;
        std::cout << "  " << RED << "Please enter 'y' for yes or 'n' for no." << RESET << "\n";
    }
}

std::string stateBadge(const std::string& stateName) {
    if (stateName == "Completed") return std::string(GREEN) + "[Completed]" + RESET;
    if (stateName == "Started")   return std::string(CYAN) + "[Started]  " + RESET;
    if (stateName == "Assigned")  return std::string(YELLOW) + "[Assigned] " + RESET;
    if (stateName == "Available") return std::string(WHITE) + BOLD + "[Available]" + RESET;
    if (stateName == "Created")   return std::string(DIM) + "[Created]  " + RESET;
    if (stateName == "Rejected")  return std::string(RED) + "[Rejected] " + RESET;
    if (stateName == "Cancelled") return std::string(RED) + DIM + "[Cancelled]" + RESET;
    return "[" + stateName + "]";
}

// ===========================================================================
// Scenario Configuration Models
// ===========================================================================
struct RoleConfig {
    std::string name;
    int authorityLevel;
    RoleConfig() : authorityLevel(1) {}
    RoleConfig(const std::string& n, int lvl) : name(n), authorityLevel(lvl) {}
};

struct ParticipantConfig {
    std::string id;
    std::string name;
    std::string roleName;
    ParticipantConfig() {}
    ParticipantConfig(const std::string& i, const std::string& n, const std::string& r)
        : id(i), name(n), roleName(r) {}
};

struct DecoratorConfig {
    bool hasPriority;
    int priorityLevel;
    bool hasValidation;
    std::vector<std::string> requiredFields;
    bool hasAuditing;

    DecoratorConfig()
        : hasPriority(false), priorityLevel(2), hasValidation(false), hasAuditing(false) {}

    std::string summary() const {
        std::string s;
        if (hasPriority) s += "[Priority " + std::to_string(priorityLevel) + "]";
        if (hasValidation) {
            if (!s.empty()) s += " ";
            s += "[Validation: ";
            for (std::size_t i = 0; i < requiredFields.size(); ++i) {
                if (i > 0) s += ",";
                s += requiredFields[i];
            }
            s += "]";
        }
        if (hasAuditing) {
            if (!s.empty()) s += " ";
            s += "[Audited]";
        }
        return s.empty() ? "None" : s;
    }
};

struct TaskConfig {
    std::string id;
    std::string name;
    std::string roleName;
    std::string strategyType;
    bool isExternal;
    std::string externalConnectorType;
    std::string externalRequest;
    std::string stageGroupId;
    DecoratorConfig decorators;

    TaskConfig() : strategyType("RoleBased"), isExternal(false) {}
    TaskConfig(const std::string& taskId, const std::string& taskName, const std::string& role,
               const std::string& strat = "RoleBased")
        : id(taskId), name(taskName), roleName(role), strategyType(strat), isExternal(false) {}
};

struct StageGroupConfig {
    std::string id;
    std::string name;
    bool isParallel;
    std::vector<std::string> taskIds;
    StageGroupConfig() : isParallel(false) {}
    StageGroupConfig(const std::string& gid, const std::string& gname, bool parallel = false)
        : id(gid), name(gname), isParallel(parallel) {}
};

class ScenarioModel {
public:
    std::string scenarioType;
    std::string name;
    std::string description;
    std::vector<RoleConfig> roles;
    std::vector<ParticipantConfig> participants;
    std::vector<TaskConfig> tasks;
    std::vector<StageGroupConfig> stageGroups;

    ScenarioModel() : scenarioType("Custom"), name("Custom Workflow"), description("") {}
    ScenarioModel(const std::string& type, const std::string& n, const std::string& d = "")
        : scenarioType(type), name(n), description(d) {}

    void addRole(const std::string& roleName, int authorityLevel) {
        for (auto& r : roles) {
            if (r.name == roleName) {
                r.authorityLevel = authorityLevel;
                return;
            }
        }
        roles.push_back(RoleConfig(roleName, authorityLevel));
    }

    void addParticipant(const std::string& id, const std::string& pName, const std::string& rName) {
        participants.push_back(ParticipantConfig(id, pName, rName));
    }

    std::vector<ParticipantConfig> getParticipantsByRole(const std::string& rName) const {
        std::vector<ParticipantConfig> out;
        for (const auto& p : participants) {
            if (p.roleName == rName) out.push_back(p);
        }
        return out;
    }

    TaskConfig* findTask(const std::string& tid) {
        for (auto& t : tasks) if (t.id == tid) return &t;
        return nullptr;
    }

    const TaskConfig* findTask(const std::string& tid) const {
        for (const auto& t : tasks) if (t.id == tid) return &t;
        return nullptr;
    }

    void addStageGroup(const std::string& gid, const std::string& gname, bool isParallel) {
        stageGroups.push_back(StageGroupConfig(gid, gname, isParallel));
    }

    StageGroupConfig* findStageGroup(const std::string& gid) {
        for (auto& g : stageGroups) if (g.id == gid) return &g;
        return nullptr;
    }

    void assignTaskToGroup(const std::string& tid, const std::string& gid) {
        TaskConfig* t = findTask(tid);
        if (t) t->stageGroupId = gid;
        for (auto& g : stageGroups) {
            auto& tids = g.taskIds;
            for (auto it = tids.begin(); it != tids.end(); ) {
                if (*it == tid) it = tids.erase(it);
                else ++it;
            }
        }
        StageGroupConfig* grp = findStageGroup(gid);
        if (grp) grp->taskIds.push_back(tid);
    }

    void printSummary() const {
        std::cout << "  " << BOLD << WHITE << "Scenario: " << name << " (" << scenarioType << ")" << RESET << "\n";
        if (!description.empty()) std::cout << "  " << DIM << description << RESET << "\n";
        printRule(72, DIM);

        std::cout << "  " << CYAN << BOLD << "[1] PERSONNEL (" << participants.size() << " created dynamically)" << RESET << "\n";
        for (const auto& r : roles) {
            auto parts = getParticipantsByRole(r.name);
            std::cout << "    • " << BOLD << r.name << RESET << " (Auth Level " << r.authorityLevel << "): "
                      << parts.size() << " instance(s)\n";
            for (const auto& p : parts) {
                std::cout << "      └─ " << p.name << " [" << p.id << "]\n";
            }
        }
        std::cout << "\n";

        std::cout << "  " << CYAN << BOLD << "[2] EXECUTION GROUPS & TASKS (" << tasks.size() << " tasks)" << RESET << "\n";
        for (std::size_t g = 0; g < stageGroups.size(); ++g) {
            const auto& grp = stageGroups[g];
            std::string mode = grp.isParallel ? (std::string(MAGENTA) + BOLD + "PARALLEL" + RESET)
                                              : (std::string(BLUE) + BOLD + "SEQUENTIAL" + RESET);
            std::cout << "    📂 Group " << (g + 1) << ": " << BOLD << grp.name << RESET << " (" << mode << ")\n";
            if (grp.taskIds.empty()) std::cout << "       " << DIM << "(No tasks assigned)" << RESET << "\n";
            for (const auto& tid : grp.taskIds) {
                const TaskConfig* tk = findTask(tid);
                if (tk) {
                    std::cout << "       " << (grp.isParallel ? "⇉ " : "⇣ ")
                              << pad(tk->name, 40)
                              << DIM << "Role: " << pad(tk->roleName, 18)
                              << "Strategy: " << tk->strategyType << RESET << "\n";
                    if (tk->decorators.hasPriority || tk->decorators.hasValidation || tk->decorators.hasAuditing) {
                        std::cout << "          " << YELLOW << "Decorators: " << tk->decorators.summary() << RESET << "\n";
                    }
                    if (tk->isExternal) {
                        std::cout << "          " << GREEN << "External: " << tk->externalConnectorType
                                  << " (" << tk->externalRequest << ")" << RESET << "\n";
                    }
                }
            }
        }
        std::cout << "\n";
    }

    static std::vector<RoleConfig> getHospitalDefaultRoles() {
        return {
            RoleConfig("Paramedic", 1),
            RoleConfig("Nurse", 2),
            RoleConfig("Doctor", 3),
            RoleConfig("Secretary", 1),
            RoleConfig("Cleaner", 1),
            RoleConfig("Department Head", 4),
            RoleConfig("Hospital Administrator", 5)
        };
    }

    static std::vector<RoleConfig> getLegalDefaultRoles() {
        return {
            RoleConfig("Paralegal", 1),
            RoleConfig("Associate", 2),
            RoleConfig("Senior Attorney", 3),
            RoleConfig("Partner", 4),
            RoleConfig("Managing Partner", 5),
            RoleConfig("Finance Clerk", 1)
        };
    }

    static std::vector<TaskConfig> getHospitalPredefinedTasks() {
        std::vector<TaskConfig> list;
        TaskConfig t1("h_pickup", "Paramedic picks up patient (dispatch notified)", "Paramedic", "RoleBased");
        t1.isExternal = true;
        t1.externalConnectorType = "APITwo_Dispatch";
        t1.externalRequest = "Ambulance dispatch confirmation";
        t1.stageGroupId = "grp_transport";
        list.push_back(t1);

        TaskConfig t2("h_transport", "Transport patient to hospital", "Paramedic", "RoleBased");
        t2.stageGroupId = "grp_transport";
        list.push_back(t2);

        TaskConfig t3("h_bay_prep", "Nurse prepares treatment bay", "Nurse", "Workload");
        t3.stageGroupId = "grp_transport";
        list.push_back(t3);

        TaskConfig t4("h_register", "Secretary registers patient", "Secretary", "RoleBased");
        t4.decorators.hasValidation = true;
        t4.decorators.requiredFields = {"patientName", "complaint"};
        t4.stageGroupId = "grp_arrival";
        list.push_back(t4);

        TaskConfig t5("h_records", "Retrieve patient history from hospital records", "Secretary", "RoleBased");
        t5.isExternal = true;
        t5.externalConnectorType = "APIOne_Records";
        t5.externalRequest = "patient-history-query";
        t5.stageGroupId = "grp_arrival";
        list.push_back(t5);

        TaskConfig t6("h_handover", "Paramedic hands patient over to nurse", "Paramedic", "RoleBased");
        t6.stageGroupId = "grp_arrival";
        list.push_back(t6);

        TaskConfig t7("h_intake", "Nurse intake and triage preparation", "Nurse", "Workload");
        t7.decorators.hasAuditing = true;
        t7.decorators.hasPriority = true;
        t7.decorators.priorityLevel = 1;
        t7.stageGroupId = "grp_arrival";
        list.push_back(t7);

        TaskConfig t8("h_doctor_accept", "Doctor accepts the case", "Doctor", "Workload");
        t8.stageGroupId = "grp_treatment";
        list.push_back(t8);

        TaskConfig t9("h_assessment", "Doctor assesses and treats patient", "Doctor", "RuleBased");
        t9.decorators.hasAuditing = true;
        t9.stageGroupId = "grp_treatment";
        list.push_back(t9);

        TaskConfig t10("h_room_alloc", "Doctor allocates room (ward / ICU / surgery)", "Doctor", "RoleBased");
        t10.stageGroupId = "grp_treatment";
        list.push_back(t10);

        TaskConfig t11("h_cleaning", "Cleaning staff clean the bay", "Cleaner", "RoleBased");
        t11.stageGroupId = "grp_closeout";
        list.push_back(t11);

        TaskConfig t12("h_archive", "Secretary archives patient history", "Secretary", "RoleBased");
        t12.stageGroupId = "grp_closeout";
        list.push_back(t12);

        return list;
    }

    static std::vector<TaskConfig> getLegalPredefinedTasks() {
        std::vector<TaskConfig> list;
        TaskConfig t1("l_client_intake", "Client intake interview", "Paralegal", "RoleBased");
        t1.decorators.hasValidation = true;
        t1.decorators.requiredFields = {"clientName", "matter"};
        t1.stageGroupId = "grp_legal_intake";
        list.push_back(t1);

        TaskConfig t2("l_conflict", "Conflict of interest check", "Associate", "Workload");
        t2.stageGroupId = "grp_legal_intake";
        list.push_back(t2);

        TaskConfig t3("l_engagement", "Send engagement letter", "Associate", "Workload");
        t3.stageGroupId = "grp_legal_onboarding";
        list.push_back(t3);

        TaskConfig t4("l_trust", "Receive trust deposit", "Finance Clerk", "RoleBased");
        t4.stageGroupId = "grp_legal_onboarding";
        list.push_back(t4);

        TaskConfig t5("l_evidence", "Gather documents and evidence", "Paralegal", "RoleBased");
        t5.stageGroupId = "grp_investigation";
        list.push_back(t5);

        TaskConfig t6("l_research", "Legal research and precedents", "Associate", "RuleBased");
        t6.stageGroupId = "grp_investigation";
        list.push_back(t6);

        TaskConfig t7("l_witness", "Interview witnesses", "Paralegal", "RoleBased");
        t7.stageGroupId = "grp_investigation";
        list.push_back(t7);

        TaskConfig t8("l_expert_brief", "Brief expert witness", "Associate", "Workload");
        t8.stageGroupId = "grp_investigation";
        list.push_back(t8);

        TaskConfig t9("l_expert_report", "Receive expert report", "Associate", "Workload");
        t9.stageGroupId = "grp_investigation";
        list.push_back(t9);

        TaskConfig t10("l_draft", "Draft court pleadings", "Associate", "RuleBased");
        t10.decorators.hasAuditing = true;
        t10.stageGroupId = "grp_pleadings";
        list.push_back(t10);

        TaskConfig t11("l_review", "Senior review and sign-off", "Senior Attorney", "RoleBased");
        t11.decorators.hasAuditing = true;
        t11.decorators.hasPriority = true;
        t11.decorators.priorityLevel = 2;
        t11.stageGroupId = "grp_pleadings";
        list.push_back(t11);

        TaskConfig t12("l_efile", "E-file pleadings with court (portal notified)", "Paralegal", "RoleBased");
        t12.isExternal = true;
        t12.externalConnectorType = "APITwo_Dispatch";
        t12.externalRequest = "Court e-filing submission";
        t12.stageGroupId = "grp_pleadings";
        list.push_back(t12);

        TaskConfig t13("l_hearing", "Attend court hearing", "Senior Attorney", "RoleBased");
        t13.stageGroupId = "grp_pleadings";
        list.push_back(t13);

        TaskConfig t14("l_judgment", "Record court judgment", "Senior Attorney", "RoleBased");
        t14.stageGroupId = "grp_judgment";
        list.push_back(t14);

        TaskConfig t15("l_settle", "Agree costs / settlement", "Partner", "RoleBased");
        t15.stageGroupId = "grp_judgment";
        list.push_back(t15);

        TaskConfig t16("l_invoice", "Invoice the client", "Finance Clerk", "RoleBased");
        t16.stageGroupId = "grp_billing";
        list.push_back(t16);

        TaskConfig t17("l_archive", "Archive the case file", "Paralegal", "RoleBased");
        t17.stageGroupId = "grp_billing";
        list.push_back(t17);

        return list;
    }
};

// ===========================================================================
// Scenario Persistence (Disk Storage & JSON Serialization)
// ===========================================================================
struct SavedScenarioInfo {
    std::string filename;
    std::string name;
    std::string type;
    int taskCount;
    int personnelCount;
};

std::string sanitizeFilename(const std::string& name) {
    std::string clean;
    for (char c : name) {
        if (std::isalnum(c) || c == '_' || c == '-') clean += c;
        else if (c == ' ') clean += '_';
    }
    return clean.empty() ? "scenario" : clean;
}

std::string resolveSavedDir(const std::string& dir = "saved_scenarios") {
    struct stat st;
    if (stat(dir.c_str(), &st) == 0 && S_ISDIR(st.st_mode)) return dir;
    std::string fallback = "cpp_project/" + dir;
    if (stat(fallback.c_str(), &st) == 0 && S_ISDIR(st.st_mode)) return fallback;
    mkdir(dir.c_str(), 0755);
    return dir;
}

std::string escapeJson(const std::string& s) {
    std::string res;
    for (char c : s) {
        if (c == '"') res += "\\\"";
        else if (c == '\\') res += "\\\\";
        else if (c == '\n') res += "\\n";
        else if (c == '\r') res += "\\r";
        else if (c == '\t') res += "\\t";
        else res += c;
    }
    return res;
}

std::string serializeScenario(const ScenarioModel& model) {
    std::ostringstream ss;
    ss << "{\n";
    ss << "  \"scenarioType\": \"" << escapeJson(model.scenarioType) << "\",\n";
    ss << "  \"name\": \"" << escapeJson(model.name) << "\",\n";
    ss << "  \"description\": \"" << escapeJson(model.description) << "\",\n";

    ss << "  \"roles\": [\n";
    for (std::size_t i = 0; i < model.roles.size(); ++i) {
        ss << "    {\n";
        ss << "      \"name\": \"" << escapeJson(model.roles[i].name) << "\",\n";
        ss << "      \"authorityLevel\": " << model.roles[i].authorityLevel << "\n";
        ss << "    }" << (i + 1 < model.roles.size() ? "," : "") << "\n";
    }
    ss << "  ],\n";

    ss << "  \"participants\": [\n";
    for (std::size_t i = 0; i < model.participants.size(); ++i) {
        ss << "    {\n";
        ss << "      \"id\": \"" << escapeJson(model.participants[i].id) << "\",\n";
        ss << "      \"name\": \"" << escapeJson(model.participants[i].name) << "\",\n";
        ss << "      \"roleName\": \"" << escapeJson(model.participants[i].roleName) << "\"\n";
        ss << "    }" << (i + 1 < model.participants.size() ? "," : "") << "\n";
    }
    ss << "  ],\n";

    ss << "  \"tasks\": [\n";
    for (std::size_t i = 0; i < model.tasks.size(); ++i) {
        const auto& t = model.tasks[i];
        ss << "    {\n";
        ss << "      \"id\": \"" << escapeJson(t.id) << "\",\n";
        ss << "      \"name\": \"" << escapeJson(t.name) << "\",\n";
        ss << "      \"roleName\": \"" << escapeJson(t.roleName) << "\",\n";
        ss << "      \"strategyType\": \"" << escapeJson(t.strategyType) << "\",\n";
        ss << "      \"isExternal\": " << (t.isExternal ? "true" : "false") << ",\n";
        ss << "      \"externalConnectorType\": \"" << escapeJson(t.externalConnectorType) << "\",\n";
        ss << "      \"externalRequest\": \"" << escapeJson(t.externalRequest) << "\",\n";
        ss << "      \"stageGroupId\": \"" << escapeJson(t.stageGroupId) << "\",\n";
        ss << "      \"decorators\": {\n";
        ss << "        \"hasPriority\": " << (t.decorators.hasPriority ? "true" : "false") << ",\n";
        ss << "        \"priorityLevel\": " << t.decorators.priorityLevel << ",\n";
        ss << "        \"hasValidation\": " << (t.decorators.hasValidation ? "true" : "false") << ",\n";
        ss << "        \"requiredFields\": [";
        for (std::size_t f = 0; f < t.decorators.requiredFields.size(); ++f) {
            ss << "\"" << escapeJson(t.decorators.requiredFields[f]) << "\""
               << (f + 1 < t.decorators.requiredFields.size() ? ", " : "");
        }
        ss << "],\n";
        ss << "        \"hasAuditing\": " << (t.decorators.hasAuditing ? "true" : "false") << "\n";
        ss << "      }\n";
        ss << "    }" << (i + 1 < model.tasks.size() ? "," : "") << "\n";
    }
    ss << "  ],\n";

    ss << "  \"stageGroups\": [\n";
    for (std::size_t i = 0; i < model.stageGroups.size(); ++i) {
        const auto& g = model.stageGroups[i];
        ss << "    {\n";
        ss << "      \"id\": \"" << escapeJson(g.id) << "\",\n";
        ss << "      \"name\": \"" << escapeJson(g.name) << "\",\n";
        ss << "      \"isParallel\": " << (g.isParallel ? "true" : "false") << ",\n";
        ss << "      \"taskIds\": [";
        for (std::size_t k = 0; k < g.taskIds.size(); ++k) {
            ss << "\"" << escapeJson(g.taskIds[k]) << "\"" << (k + 1 < g.taskIds.size() ? ", " : "");
        }
        ss << "]\n";
        ss << "    }" << (i + 1 < model.stageGroups.size() ? "," : "") << "\n";
    }
    ss << "  ]\n";
    ss << "}\n";
    return ss.str();
}

std::string extractString(const std::string& src, const std::string& key) {
    std::string needle = "\"" + key + "\": \"";
    size_t pos = src.find(needle);
    if (pos == std::string::npos) return "";
    pos += needle.size();
    size_t endPos = src.find("\"", pos);
    return (endPos == std::string::npos) ? "" : src.substr(pos, endPos - pos);
}

int extractInt(const std::string& src, const std::string& key, int defaultVal = 0) {
    std::string needle = "\"" + key + "\": ";
    size_t pos = src.find(needle);
    if (pos == std::string::npos) return defaultVal;
    pos += needle.size();
    try { return std::stoi(src.substr(pos)); } catch (...) { return defaultVal; }
}

bool extractBool(const std::string& src, const std::string& key, bool defaultVal = false) {
    std::string needle = "\"" + key + "\": ";
    size_t pos = src.find(needle);
    if (pos == std::string::npos) return defaultVal;
    pos += needle.size();
    if (src.compare(pos, 4, "true") == 0) return true;
    if (src.compare(pos, 5, "false") == 0) return false;
    return defaultVal;
}

bool deserializeScenario(const std::string& jsonContent, ScenarioModel& model) {
    try {
        model.scenarioType = extractString(jsonContent, "scenarioType");
        if (model.scenarioType.empty()) model.scenarioType = "Custom";
        model.name = extractString(jsonContent, "name");
        model.description = extractString(jsonContent, "description");
        model.roles.clear();
        model.participants.clear();
        model.tasks.clear();
        model.stageGroups.clear();

        size_t rolesPos = jsonContent.find("\"roles\": [");
        if (rolesPos != std::string::npos) {
            size_t endRoles = jsonContent.find("]", rolesPos);
            size_t cur = rolesPos;
            while ((cur = jsonContent.find("{", cur)) != std::string::npos && cur < endRoles) {
                size_t objEnd = jsonContent.find("}", cur);
                if (objEnd == std::string::npos) break;
                std::string block = jsonContent.substr(cur, objEnd - cur + 1);
                std::string rName = extractString(block, "name");
                int auth = extractInt(block, "authorityLevel", 1);
                if (!rName.empty()) model.addRole(rName, auth);
                cur = objEnd + 1;
            }
        }

        size_t partPos = jsonContent.find("\"participants\": [");
        if (partPos != std::string::npos) {
            size_t endParts = jsonContent.find("]", partPos);
            size_t cur = partPos;
            while ((cur = jsonContent.find("{", cur)) != std::string::npos && cur < endParts) {
                size_t objEnd = jsonContent.find("}", cur);
                if (objEnd == std::string::npos) break;
                std::string block = jsonContent.substr(cur, objEnd - cur + 1);
                std::string pId = extractString(block, "id");
                std::string pName = extractString(block, "name");
                std::string rName = extractString(block, "roleName");
                if (!pId.empty()) model.addParticipant(pId, pName, rName);
                cur = objEnd + 1;
            }
        }

        size_t tasksPos = jsonContent.find("\"tasks\": [");
        if (tasksPos != std::string::npos) {
            size_t endTasks = jsonContent.find("\"stageGroups\":");
            if (endTasks == std::string::npos) endTasks = jsonContent.size();
            size_t cur = tasksPos;
            while ((cur = jsonContent.find("{\n      \"id\":", cur)) != std::string::npos && cur < endTasks) {
                size_t objEnd = jsonContent.find("\n    }", cur);
                if (objEnd == std::string::npos) break;
                std::string block = jsonContent.substr(cur, objEnd - cur + 6);
                TaskConfig tc;
                tc.id = extractString(block, "id");
                tc.name = extractString(block, "name");
                tc.roleName = extractString(block, "roleName");
                tc.strategyType = extractString(block, "strategyType");
                tc.isExternal = extractBool(block, "isExternal");
                tc.externalConnectorType = extractString(block, "externalConnectorType");
                tc.externalRequest = extractString(block, "externalRequest");
                tc.stageGroupId = extractString(block, "stageGroupId");

                tc.decorators.hasPriority = extractBool(block, "hasPriority");
                tc.decorators.priorityLevel = extractInt(block, "priorityLevel", 2);
                tc.decorators.hasValidation = extractBool(block, "hasValidation");
                tc.decorators.hasAuditing = extractBool(block, "hasAuditing");

                size_t rfPos = block.find("\"requiredFields\": [");
                if (rfPos != std::string::npos) {
                    size_t rfEnd = block.find("]", rfPos);
                    size_t fCur = rfPos;
                    while ((fCur = block.find("\"", fCur + 1)) != std::string::npos && fCur < rfEnd) {
                        if (block.substr(fCur, 15) == "\"requiredFields") { fCur += 15; continue; }
                        size_t strEnd = block.find("\"", fCur + 1);
                        if (strEnd == std::string::npos || strEnd > rfEnd) break;
                        tc.decorators.requiredFields.push_back(block.substr(fCur + 1, strEnd - fCur - 1));
                        fCur = strEnd + 1;
                    }
                }
                if (!tc.id.empty()) model.tasks.push_back(tc);
                cur = objEnd + 1;
            }
        }

        size_t sgPos = jsonContent.find("\"stageGroups\": [");
        if (sgPos != std::string::npos) {
            size_t endSg = jsonContent.find("]", sgPos);
            size_t cur = sgPos;
            while ((cur = jsonContent.find("{", cur)) != std::string::npos && cur < endSg) {
                size_t objEnd = jsonContent.find("}", cur);
                if (objEnd == std::string::npos) break;
                std::string block = jsonContent.substr(cur, objEnd - cur + 1);
                std::string gid = extractString(block, "id");
                std::string gname = extractString(block, "name");
                bool isPar = extractBool(block, "isParallel");
                if (!gid.empty()) model.addStageGroup(gid, gname, isPar);

                size_t tidsPos = block.find("\"taskIds\": [");
                if (tidsPos != std::string::npos) {
                    size_t tidsEnd = block.find("]", tidsPos);
                    size_t tCur = tidsPos;
                    while ((tCur = block.find("\"", tCur + 1)) != std::string::npos && tCur < tidsEnd) {
                        if (block.substr(tCur, 8) == "\"taskIds") { tCur += 8; continue; }
                        size_t strEnd = block.find("\"", tCur + 1);
                        if (strEnd == std::string::npos || strEnd > tidsEnd) break;
                        std::string tId = block.substr(tCur + 1, strEnd - tCur - 1);
                        model.assignTaskToGroup(tId, gid);
                        tCur = strEnd + 1;
                    }
                }
                cur = objEnd + 1;
            }
        }
        return true;
    } catch (...) { return false; }
}

bool saveScenarioToDisk(const ScenarioModel& model) {
    std::string dir = resolveSavedDir();
    std::string fullPath = dir + "/" + sanitizeFilename(model.name) + ".json";
    std::ofstream out(fullPath);
    if (!out.is_open()) return false;
    out << serializeScenario(model);
    out.close();
    return true;
}

bool loadScenarioFromDisk(const std::string& path, ScenarioModel& model) {
    std::ifstream in(path);
    if (!in.is_open()) return false;
    std::stringstream buf;
    buf << in.rdbuf();
    in.close();
    return deserializeScenario(buf.str(), model);
}

std::vector<SavedScenarioInfo> listSavedScenarios() {
    std::vector<SavedScenarioInfo> list;
    std::string dir = resolveSavedDir();
    DIR* dp = opendir(dir.c_str());
    if (!dp) return list;
    struct dirent* ep;
    while ((ep = readdir(dp)) != nullptr) {
        std::string fname = ep->d_name;
        if (fname.size() > 5 && fname.substr(fname.size() - 5) == ".json") {
            std::string fullPath = dir + "/" + fname;
            ScenarioModel model;
            if (loadScenarioFromDisk(fullPath, model)) {
                SavedScenarioInfo info;
                info.filename = fullPath;
                info.name = model.name;
                info.type = model.scenarioType;
                info.taskCount = static_cast<int>(model.tasks.size());
                info.personnelCount = static_cast<int>(model.participants.size());
                list.push_back(info);
            }
        }
    }
    closedir(dp);
    return list;
}

// ===========================================================================
// Interactive Scenario Builder Steps (Dynamic Configuration)
// ===========================================================================
bool step1ConfigurePersonnel(ScenarioModel& model) {
    while (true) {
        printHeader(model.name, "Step 1 of 5: Configure Personnel");
        std::cout << "  " << BOLD << "Please specify the quantity of each personnel type to create dynamically:" << RESET << "\n\n";

        model.participants.clear();
        int participantNum = 1;

        for (const auto& role : model.roles) {
            std::string promptText = "  How many " + role.name + "s? (Authority " + std::to_string(role.authorityLevel) + ") [default: 1]: ";
            int defaultQty = 1;
            if (role.name == "Nurse" || role.name == "Doctor" || role.name == "Paralegal" || role.name == "Associate") {
                defaultQty = 2;
            }
            int qty = promptInt(promptText, defaultQty, 0, 50);

            for (int i = 1; i <= qty; ++i) {
                std::string pid = "P" + std::to_string(participantNum++);
                std::string pname = role.name + " #" + std::to_string(i);
                if (role.name == "Paramedic") pname = (i == 1) ? "Paramedic Thabo" : ("Paramedic Sbu " + std::to_string(i));
                else if (role.name == "Nurse") pname = (i == 1) ? "Nurse Dlamini" : (i == 2 ? "Nurse Khoza" : ("Nurse Nandi " + std::to_string(i)));
                else if (role.name == "Doctor") pname = (i == 1) ? "Dr. Khumalo" : (i == 2 ? "Dr. Naidoo" : ("Dr. Patel " + std::to_string(i)));
                else if (role.name == "Secretary") pname = (i == 1) ? "Secretary Lerato" : ("Secretary James " + std::to_string(i));
                else if (role.name == "Cleaner") pname = (i == 1) ? "Cleaner Pieter" : ("Cleaner Sipho " + std::to_string(i));
                else if (role.name == "Department Head") pname = "Dr. Mthembu (Dept Head)";
                else if (role.name == "Hospital Administrator") pname = "Dr. Botha (CEO/Admin)";
                else if (role.name == "Paralegal") pname = (i == 1) ? "Paralegal Sipho" : (i == 2 ? "Paralegal Anke" : ("Paralegal Ayesha " + std::to_string(i)));
                else if (role.name == "Associate") pname = (i == 1) ? "Associate Zanele" : (i == 2 ? "Associate Johan" : ("Associate Thabo " + std::to_string(i)));
                else if (role.name == "Senior Attorney") pname = (i == 1) ? "Senior Attorney Mokoena" : ("Senior Attorney Botha " + std::to_string(i));
                else if (role.name == "Partner") pname = (i == 1) ? "Partner Pillay" : ("Partner Adams " + std::to_string(i));
                else if (role.name == "Managing Partner") pname = "Managing Partner van Wyk";
                else if (role.name == "Finance Clerk") pname = (i == 1) ? "Finance Clerk Nomsa" : ("Finance Clerk Megan " + std::to_string(i));

                model.addParticipant(pid, pname, role.name);
            }
        }

        std::cout << "\n  " << GREEN << BOLD << "✔ Dynamic Personnel Creation Complete!" << RESET << " ("
                  << model.participants.size() << " instances instantiated)\n";
        printRule(72, DIM);
        for (const auto& p : model.participants) {
            std::cout << "    [" << p.id << "] " << pad(p.name, 32) << DIM << "Role: " << p.roleName << RESET << "\n";
        }
        printRule(72, DIM);

        std::cout << "\n  Options:\n";
        std::cout << "    1) Confirm personnel and proceed to Step 2 (Tasks)\n";
        std::cout << "    2) Reconfigure personnel quantities\n";
        std::cout << "    3) Add custom role & participant\n";
        std::cout << "    0) Cancel / Back to Main Menu\n";

        int opt = promptChoice("  Select choice [0-3]: ", 0, 3);
        if (opt == 1) return true;
        if (opt == 0) return false;
        if (opt == 3) {
            std::string nrName = promptString("    Enter new role name: ");
            int nAuth = promptInt("    Enter authority level [1-5]: ", 1, 1, 5);
            model.addRole(nrName, nAuth);
            std::string npName = promptString("    Enter participant name: ");
            std::string npId = "P" + std::to_string(model.participants.size() + 1);
            model.addParticipant(npId, npName, nrName);
            std::cout << "  " << GREEN << "Added role and participant!" << RESET << "\n";
            pause();
        }
    }
}

void promptSelectPredefinedTasks(ScenarioModel& model, const std::vector<TaskConfig>& pool) {
    if (pool.empty()) {
        std::cout << "  " << YELLOW << "No predefined tasks available for this scenario type." << RESET << "\n";
        pause();
        return;
    }
    std::vector<bool> selected(pool.size(), true);
    while (true) {
        printHeader(model.name, "Select Predefined Tasks");
        std::cout << "  " << BOLD << "Predefined Tasks (toggle or include all):" << RESET << "\n\n";
        for (std::size_t i = 0; i < pool.size(); ++i) {
            std::cout << "    " << (i + 1) << ") [" << (selected[i] ? GREEN + std::string("✔") + RESET : " ") << "] "
                      << pad(pool[i].name, 45) << DIM << "Role: " << pool[i].roleName << RESET << "\n";
        }
        std::cout << "\n    [A] Select All\n    [C] Clear All\n    [1-" << pool.size() << "] Toggle task\n"
                  << "    [D / ENTER] Done / Add selected tasks to scenario\n    [0] Cancel\n";

        std::string choice = promptString("\n  Enter command [D]: ", "d", true);
        if (choice == "0" || choice == "q" || choice == "Q") return;
        if (choice.empty() || choice == "d" || choice == "D") {
            int added = 0;
            for (std::size_t i = 0; i < pool.size(); ++i) {
                if (selected[i] && !model.findTask(pool[i].id)) {
                    model.tasks.push_back(pool[i]);
                    model.assignTaskToGroup(pool[i].id, pool[i].stageGroupId);
                    added++;
                }
            }
            std::cout << "  " << GREEN << "Added " << added << " task(s) to workflow." << RESET << "\n";
            pause();
            return;
        }
        if (choice == "a" || choice == "A") { std::fill(selected.begin(), selected.end(), true); continue; }
        if (choice == "c" || choice == "C") { std::fill(selected.begin(), selected.end(), false); continue; }
        try {
            int idx = std::stoi(choice);
            if (idx >= 1 && idx <= static_cast<int>(pool.size())) selected[idx - 1] = !selected[idx - 1];
        } catch (...) {}
    }
}

void promptCustomTask(ScenarioModel& model) {
    printHeader(model.name, "Create Custom Task");
    std::string tName = promptString("  Enter task name: ");
    if (tName.empty()) return;
    std::string tId = "task_" + std::to_string(model.tasks.size() + 1);

    std::cout << "\n  Select required role for this task:\n";
    for (std::size_t i = 0; i < model.roles.size(); ++i) {
        std::cout << "    " << (i + 1) << ") " << model.roles[i].name << " (Auth Level " << model.roles[i].authorityLevel << ")\n";
    }
    int rChoice = promptChoice("  Role choice [1-" + std::to_string(model.roles.size()) + "]: ", 1, static_cast<int>(model.roles.size()));
    std::string roleName = model.roles[rChoice - 1].name;

    std::cout << "\n  Select assignment strategy:\n";
    std::cout << "    1) Role-Based Assignment\n    2) Workload-Based Assignment\n    3) Rule-Based Assignment\n";
    int sChoice = promptChoice("  Strategy choice [1-3]: ", 1, 3);
    std::string strat = (sChoice == 2) ? "Workload" : (sChoice == 3 ? "RuleBased" : "RoleBased");

    TaskConfig tc(tId, tName, roleName, strat);
    bool isExt = promptBool("  Is this an external task communicating with an external API?", false);
    if (isExt) {
        tc.isExternal = true;
        std::cout << "    1) APITwo - Dispatch / External Service Notification\n";
        std::cout << "    2) APIOne - Database / Archive Query Connector\n";
        int eChoice = promptChoice("    Connector choice [1-2]: ", 1, 2);
        tc.externalConnectorType = (eChoice == 1) ? "APITwo_Dispatch" : "APIOne_Records";
        tc.externalRequest = promptString("    Enter request payload: ", "External task query payload");
    }

    if (!model.stageGroups.empty()) {
        std::cout << "\n  Select stage group to place this task in:\n";
        for (std::size_t i = 0; i < model.stageGroups.size(); ++i) {
            std::cout << "    " << (i + 1) << ") " << model.stageGroups[i].name
                      << (model.stageGroups[i].isParallel ? " [Parallel]" : " [Sequential]") << "\n";
        }
        int gChoice = promptChoice("    Group choice [1-" + std::to_string(model.stageGroups.size()) + "]: ",
                                   1, static_cast<int>(model.stageGroups.size()));
        tc.stageGroupId = model.stageGroups[gChoice - 1].id;
    }

    model.tasks.push_back(tc);
    if (!tc.stageGroupId.empty()) model.assignTaskToGroup(tc.id, tc.stageGroupId);
    std::cout << "\n  " << GREEN << "✔ Custom task created successfully!" << RESET << "\n";
    pause();
}

bool step2ConfigureTasks(ScenarioModel& model, const std::vector<TaskConfig>& predefinedPool) {
    while (true) {
        printHeader(model.name, "Step 2 of 5: Configure Tasks");
        std::cout << "  Current tasks configured: " << BOLD << model.tasks.size() << RESET << "\n";
        if (!model.tasks.empty()) {
            std::cout << "\n  " << DIM << "Configured Tasks:" << RESET << "\n";
            for (std::size_t i = 0; i < model.tasks.size(); ++i) {
                std::cout << "    " << (i + 1) << ") " << pad(model.tasks[i].name, 40)
                          << DIM << "Role: " << pad(model.tasks[i].roleName, 18)
                          << "Strategy: " << model.tasks[i].strategyType << RESET << "\n";
            }
        }
        std::cout << "\n  " << BOLD << "Choose task configuration method:" << RESET << "\n";
        std::cout << "    1) Use predefined tasks (" << predefinedPool.size() << " available)\n";
        std::cout << "    2) Create custom task\n";
        if (!model.tasks.empty()) {
            std::cout << "    3) Proceed to Step 3 (View and Organize Tasks)\n";
            std::cout << "    4) Clear all tasks\n";
        }
        std::cout << "    0) Back to Step 1 (Personnel)\n";

        int maxChoice = model.tasks.empty() ? 2 : 4;
        int opt = promptChoice("  Select choice [0-" + std::to_string(maxChoice) + "]: ", 0, maxChoice);
        if (opt == 0) return false;
        if (opt == 1) promptSelectPredefinedTasks(model, predefinedPool);
        else if (opt == 2) promptCustomTask(model);
        else if (opt == 3) return true;
        else if (opt == 4) {
            model.tasks.clear();
            std::cout << "  " << YELLOW << "Cleared all tasks." << RESET << "\n";
            pause();
        }
    }
}

bool step3ViewAndOrganizeTasks(ScenarioModel& model) {
    while (true) {
        printHeader(model.name, "Step 3 of 5: View and Organize Tasks");
        std::cout << "  " << BOLD << "Tasks and Execution Organization:" << RESET << "\n\n";

        for (std::size_t g = 0; g < model.stageGroups.size(); ++g) {
            const auto& grp = model.stageGroups[g];
            std::string mode = grp.isParallel ? (std::string(MAGENTA) + BOLD + "PARALLEL GROUP" + RESET)
                                              : (std::string(BLUE) + BOLD + "SEQUENTIAL GROUP" + RESET);
            std::cout << "  📂 Group " << (g + 1) << ": " << BOLD << grp.name << RESET << " (" << mode << ")\n";
            if (grp.taskIds.empty()) std::cout << "     " << DIM << "(No tasks assigned)" << RESET << "\n";
            for (const auto& tid : grp.taskIds) {
                const TaskConfig* tk = model.findTask(tid);
                if (tk) {
                    std::cout << "     " << (grp.isParallel ? "⇉ " : "⇣ ")
                              << pad(tk->name, 40) << DIM << "Role: " << pad(tk->roleName, 18)
                              << "Decorators: " << tk->decorators.summary() << RESET << "\n";
                }
            }
            std::cout << "\n";
        }

        std::cout << "  Organization Actions:\n";
        std::cout << "    1) Create new execution group\n";
        std::cout << "    2) Toggle group mode (Sequential ↔ Parallel)\n";
        std::cout << "    3) Move task to another group\n";
        std::cout << "    4) View task details\n";
        std::cout << "    5) Remove a task\n";
        std::cout << "    6) Proceed to Step 4 (Apply Decorators)\n";
        std::cout << "    0) Back to Step 2 (Tasks)\n";

        int opt = promptChoice("\n  Select choice [0-6]: ", 0, 6);
        if (opt == 0) return false;
        if (opt == 6) return true;

        if (opt == 1) {
            std::string gName = promptString("  Enter new group name: ");
            bool isPar = promptBool("  Should tasks in this group execute in PARALLEL?", false);
            std::string gid = "grp_" + std::to_string(model.stageGroups.size() + 1);
            model.addStageGroup(gid, gName, isPar);
            std::cout << "  " << GREEN << "Execution group added." << RESET << "\n";
            pause();
        } else if (opt == 2) {
            std::cout << "\n  Select group to toggle execution mode:\n";
            for (std::size_t i = 0; i < model.stageGroups.size(); ++i) {
                std::cout << "    " << (i + 1) << ") " << model.stageGroups[i].name
                          << " (Currently: " << (model.stageGroups[i].isParallel ? "PARALLEL" : "SEQUENTIAL") << ")\n";
            }
            int gIdx = promptChoice("  Group choice [1-" + std::to_string(model.stageGroups.size()) + "]: ",
                                    1, static_cast<int>(model.stageGroups.size()));
            model.stageGroups[gIdx - 1].isParallel = !model.stageGroups[gIdx - 1].isParallel;
            std::cout << "  " << GREEN << "Mode changed to "
                      << (model.stageGroups[gIdx - 1].isParallel ? "PARALLEL" : "SEQUENTIAL") << RESET << "\n";
            pause();
        } else if (opt == 3) {
            if (model.tasks.empty()) { std::cout << "  " << RED << "No tasks available." << RESET << "\n"; pause(); continue; }
            std::cout << "\n  Select task to move:\n";
            for (std::size_t i = 0; i < model.tasks.size(); ++i) {
                std::cout << "    " << (i + 1) << ") " << model.tasks[i].name << "\n";
            }
            int tIdx = promptChoice("  Task choice: ", 1, static_cast<int>(model.tasks.size()));
            std::cout << "\n  Move \"" << model.tasks[tIdx - 1].name << "\" to which group?\n";
            for (std::size_t i = 0; i < model.stageGroups.size(); ++i) {
                std::cout << "    " << (i + 1) << ") " << model.stageGroups[i].name << "\n";
            }
            int gChoice = promptChoice("  Group choice: ", 1, static_cast<int>(model.stageGroups.size()));
            model.assignTaskToGroup(model.tasks[tIdx - 1].id, model.stageGroups[gChoice - 1].id);
            std::cout << "  " << GREEN << "Task moved." << RESET << "\n";
            pause();
        } else if (opt == 4) {
            for (std::size_t i = 0; i < model.tasks.size(); ++i) std::cout << "    " << (i + 1) << ") " << model.tasks[i].name << "\n";
            int tIdx = promptChoice("  Task choice: ", 1, static_cast<int>(model.tasks.size()));
            const TaskConfig& tk = model.tasks[tIdx - 1];
            std::cout << "\n  " << BOLD << "Task Details:" << RESET << "\n"
                      << "    ID: " << tk.id << "\n    Name: " << tk.name << "\n    Role: " << tk.roleName
                      << "\n    Strategy: " << tk.strategyType << "\n    Decorators: " << tk.decorators.summary() << "\n";
            pause();
        } else if (opt == 5) {
            for (std::size_t i = 0; i < model.tasks.size(); ++i) std::cout << "    " << (i + 1) << ") " << model.tasks[i].name << "\n";
            int tIdx = promptChoice("  Task choice: ", 1, static_cast<int>(model.tasks.size()));
            std::string remName = model.tasks[tIdx - 1].name;
            std::string remId = model.tasks[tIdx - 1].id;
            for (auto it = model.tasks.begin(); it != model.tasks.end(); ++it) {
                if (it->id == remId) { model.tasks.erase(it); break; }
            }
            std::cout << "  " << YELLOW << "Removed \"" << remName << "\"." << RESET << "\n";
            pause();
        }
    }
}

void promptTaskDecoratorConfig(TaskConfig& task) {
    while (true) {
        printHeader("Configure Decorators", "Task: " + task.name);
        std::cout << "  " << BOLD << "Current Decorators applied:" << RESET << "\n";
        std::cout << "    " << YELLOW << task.decorators.summary() << RESET << "\n\n";

        std::cout << "  Select decorator action:\n";
        std::cout << "    1) " << (task.decorators.hasPriority ? "Modify Priority Decorator" : "Add Priority Decorator (PriorityTask)") << "\n";
        std::cout << "    2) " << (task.decorators.hasValidation ? "Modify Validation Decorator" : "Add Validation Decorator (ValidatedTask)") << "\n";
        std::cout << "    3) " << (task.decorators.hasAuditing ? "Remove Auditing Decorator" : "Add Auditing Decorator (AuditedTask)") << "\n";
        std::cout << "    4) Clear all decorators on this task\n";
        std::cout << "    0) Done / Back to task list\n";

        int c = promptChoice("\n  Select choice [0-4]: ", 0, 4);
        if (c == 0) return;
        if (c == 1) {
            std::cout << "\n  Priority Levels: 1 = Critical, 2 = High, 3 = Medium, 4 = Low\n";
            int pr = promptInt("  Enter priority level [1-4]: ", task.decorators.priorityLevel, 1, 4);
            task.decorators.hasPriority = true;
            task.decorators.priorityLevel = pr;
            std::cout << "  " << GREEN << "Priority decorator set to level " << pr << "." << RESET << "\n";
            pause();
        } else if (c == 2) {
            task.decorators.hasValidation = true;
            std::cout << "\n  Enter required field keys (separated by comma, e.g. patientName,complaint):\n";
            std::string fieldsStr = promptString("  Required fields: ", "patientName,complaint");
            task.decorators.requiredFields.clear();
            std::stringstream ss(fieldsStr);
            std::string item;
            while (std::getline(ss, item, ',')) {
                size_t f = item.find_first_not_of(" \t\r\n");
                size_t l = item.find_last_not_of(" \t\r\n");
                if (f != std::string::npos) task.decorators.requiredFields.push_back(item.substr(f, l - f + 1));
            }
            std::cout << "  " << GREEN << "Validation decorator set with " << task.decorators.requiredFields.size() << " required field(s)." << RESET << "\n";
            pause();
        } else if (c == 3) {
            task.decorators.hasAuditing = !task.decorators.hasAuditing;
            std::cout << "  " << GREEN << "Auditing decorator " << (task.decorators.hasAuditing ? "enabled" : "disabled") << "." << RESET << "\n";
            pause();
        } else if (c == 4) {
            task.decorators.hasPriority = false;
            task.decorators.hasValidation = false;
            task.decorators.hasAuditing = false;
            task.decorators.requiredFields.clear();
            std::cout << "  " << YELLOW << "Cleared decorators for this task." << RESET << "\n";
            pause();
        }
    }
}

bool step4ApplyDecorators(ScenarioModel& model) {
    while (true) {
        printHeader(model.name, "Step 4 of 5: Apply GoF Decorators");
        std::cout << "  " << BOLD << "Task Decorators (PriorityTask, ValidatedTask, AuditedTask):" << RESET << "\n\n";
        for (std::size_t i = 0; i < model.tasks.size(); ++i) {
            std::cout << "    " << std::setw(2) << (i + 1) << ") "
                      << pad(model.tasks[i].name, 42)
                      << YELLOW << model.tasks[i].decorators.summary() << RESET << "\n";
        }

        std::cout << "\n  " << BOLD << "Decorator Options:" << RESET << "\n";
        std::cout << "    1) Customize decorators for a specific task\n";
        std::cout << "    2) Apply recommended default decorators for this scenario\n";
        std::cout << "    3) Clear all decorators from all tasks\n";
        std::cout << "    4) Proceed to Step 5 (Review and Execution)\n";
        std::cout << "    0) Back to Step 3 (Organize Tasks)\n";

        std::string choice = promptString("\n  Select option [1-4 or 0 to back]: ", "4", true);
        if (choice == "0" || choice == "b" || choice == "B") return false;
        if (choice == "4" || choice == "p" || choice == "P" || choice.empty()) return true;

        if (choice == "1") {
            int idx = promptChoice("  Select task number [1-" + std::to_string(model.tasks.size()) + "]: ",
                                   1, static_cast<int>(model.tasks.size()));
            promptTaskDecoratorConfig(model.tasks[idx - 1]);
        } else if (choice == "2") {
            if (model.scenarioType == "Hospital") {
                for (auto& t : model.tasks) {
                    if (t.id == "h_pickup") {
                        t.decorators.hasAuditing = true;
                    } else if (t.id == "h_register") {
                        t.decorators.hasValidation = true;
                        t.decorators.requiredFields = {"patientName", "complaint", "nationalId"};
                    } else if (t.id == "h_intake") {
                        t.decorators.hasPriority = true;
                        t.decorators.priorityLevel = 1;
                        t.decorators.hasAuditing = true;
                    } else if (t.id == "h_assessment") {
                        t.decorators.hasValidation = true;
                        t.decorators.requiredFields = {"diagnosis", "treatmentPlan"};
                        t.decorators.hasAuditing = true;
                    } else if (t.id == "h_room_alloc") {
                        t.decorators.hasPriority = true;
                        t.decorators.priorityLevel = 1;
                    }
                }
            } else if (model.scenarioType == "Legal") {
                for (auto& t : model.tasks) {
                    if (t.id == "l_client_intake") {
                        t.decorators.hasValidation = true;
                        t.decorators.requiredFields = {"clientName", "matterType", "retainerAmount"};
                    } else if (t.id == "l_draft" || t.id == "l_review") {
                        t.decorators.hasAuditing = true;
                        t.decorators.hasPriority = true;
                        t.decorators.priorityLevel = 2;
                    } else if (t.id == "l_settle") {
                        t.decorators.hasPriority = true;
                        t.decorators.priorityLevel = 1;
                        t.decorators.hasValidation = true;
                        t.decorators.requiredFields = {"settlementAmount"};
                        t.decorators.hasAuditing = true;
                    }
                }
            } else {
                for (std::size_t i = 0; i < model.tasks.size(); ++i) {
                    if (i == 0) {
                        model.tasks[i].decorators.hasValidation = true;
                        model.tasks[i].decorators.requiredFields = {"clientId", "approvalCode"};
                    } else if (i == 1) {
                        model.tasks[i].decorators.hasAuditing = true;
                    } else if (i == 2) {
                        model.tasks[i].decorators.hasPriority = true;
                        model.tasks[i].decorators.priorityLevel = 1;
                    }
                }
            }
            std::cout << "  " << GREEN << "Applied recommended decorator configuration." << RESET << "\n";
            pause();
        } else if (choice == "3") {
            for (auto& t : model.tasks) {
                t.decorators.hasPriority = false;
                t.decorators.hasValidation = false;
                t.decorators.hasAuditing = false;
                t.decorators.requiredFields.clear();
            }
            std::cout << "  " << YELLOW << "Cleared all decorators from all tasks." << RESET << "\n";
            pause();
        } else {
            try {
                int idx = std::stoi(choice);
                if (idx >= 1 && idx <= static_cast<int>(model.tasks.size())) {
                    promptTaskDecoratorConfig(model.tasks[idx - 1]);
                }
            } catch (...) {}
        }
    }
}

bool step5ReviewScenario(ScenarioModel& model, int& outExecutionMode) {
    while (true) {
        printHeader(model.name, "Step 5 of 5: Scenario Review & Confirmation");
        model.printSummary();

        std::cout << "  " << BOLD << "Ready to execute the scenario with the TIVIDY workflow engine?" << RESET << "\n\n";
        std::cout << "    1) 🕹  Run Interactive Step-by-Step Execution (Manual driving, Undo, Reject, Approvals)\n";
        std::cout << "    2) 🎬  Run Automated Demonstration (Full walkthrough with live telemetry)\n";
        std::cout << "    3) 💾  Save Scenario to Disk (saved_scenarios/)\n";
        std::cout << "    4) ✏   Modify Task Decorators (Step 4)\n";
        std::cout << "    5) ✏   Modify Organization & Parallel Groups (Step 3)\n";
        std::cout << "    6) ✏   Modify Tasks (Step 2)\n";
        std::cout << "    7) ✏   Modify Personnel (Step 1)\n";
        std::cout << "    0) ↩   Cancel and Return to Main Menu\n";

        int opt = promptChoice("\n  Select choice [0-7]: ", 0, 7);
        if (opt == 0) return false;
        if (opt == 1) { outExecutionMode = 2; return true; }
        if (opt == 2) { outExecutionMode = 1; return true; }
        if (opt == 3) {
            std::string sname = promptString("  Enter save name [" + model.name + "]: ", model.name);
            model.name = sname;
            saveScenarioToDisk(model);
            std::cout << "  " << GREEN << "Scenario saved to disk!" << RESET << "\n";
            pause();
        } else if (opt == 4) { if (!step4ApplyDecorators(model)) continue; }
        else if (opt == 5) { if (!step3ViewAndOrganizeTasks(model)) continue; }
        else if (opt == 6) {
            auto predefined = (model.scenarioType == "Hospital") ? ScenarioModel::getHospitalPredefinedTasks()
                            : (model.scenarioType == "Legal" ? ScenarioModel::getLegalPredefinedTasks() : std::vector<TaskConfig>());
            if (!step2ConfigureTasks(model, predefined)) continue;
        } else if (opt == 7) {
            if (!step1ConfigurePersonnel(model)) continue;
        }
    }
}

// ===========================================================================
// Workflow Execution Engine (Composite, Decorators, Observers, Commands)
// ===========================================================================
std::string currentTimestamp() {
    char buf[32];
    std::time_t t = std::time(nullptr);
    std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", std::localtime(&t));
    return buf;
}

// Returns all tasks in the workflow that are currently in an actionable state:
// Available (ready to be assigned), Assigned (ready to start), Started (ready to complete), or Rejected (ready to retry)
std::vector<WorkItem*> getActionableTasks(Stage* root, const std::map<std::string, WorkItem*>& taskMap) {
    std::vector<WorkItem*> out;
    if (!root) return out;
    root->activateChildren();

    for (const auto& pair : taskMap) {
        WorkItem* w = pair.second;
        if (!w) continue;
        std::string st = w->getStateName();
        if (st == "Available" || st == "Assigned" || st == "Started" || st == "Rejected") {
            out.push_back(w);
        }
    }
    return out;
}

void executeScenarioWorkflow(const ScenarioModel& model, int initialMode = 0) {
    int mode = initialMode;
    if (mode == 0) {
        printHeader("Execution Mode Selection", model.name);
        std::cout << "  " << BOLD << "Choose execution mode:" << RESET << "\n\n";
        std::cout << "    1) 🎬 Automated Demonstration Execution (auto-assigns & completes live)\n";
        std::cout << "    2) 🕹 Interactive Step-by-Step Execution (manual task driving, undo, reject)\n";
        std::cout << "    0) ↩ Back\n";

        mode = promptChoice("\n  Select execution mode [0-2]: ", 0, 2);
        if (mode == 0) return;
    }

    // Instantiate dynamic resources
    std::map<std::string, Role*> roles;
    std::vector<Participant*> participants;
    std::map<std::string, Participant*> participantMap;

    for (const auto& r : model.roles) {
        roles[r.name] = new Role(r.name, r.authorityLevel);
    }
    for (const auto& p : model.participants) {
        Role* rp = roles[p.roleName];
        if (!rp) { rp = new Role(p.roleName, 1); roles[p.roleName] = rp; }
        Participant* part = new Participant(p.id, p.name, rp);
        participants.push_back(part);
        participantMap[p.id] = part;
    }

    RoleBasedAssignment byRoleStrategy;
    WorkloadAssignment byWorkloadStrategy;
    RuleBasedAssignment ruleStrategy([](Task* t, Participant* p) {
        if (!t || !p) return false;
        Role* req = t->getRequiredRole();
        if (req && !p->hasRole(req)) return false;
        return p->getWorkload() < 3;
    });

    APIOne apiOne;
    APITwo apiTwo;
    AdapterOne adapterOne(&apiOne);
    AdapterTwo adapterTwo(&apiTwo);

    EventPublisher publisher;
    AuditLogger auditLogger;
    Notifier notifier;
    MetricsCollector metricsCollector;

    publisher.attach(&auditLogger);
    publisher.attach(&notifier);
    publisher.attach(&metricsCollector);

    // Approval Chain: Chain of Responsibility linking roles with authority >= 2 ascending
    std::vector<Role*> sortedRoles;
    for (auto& pair : roles) sortedRoles.push_back(pair.second);
    std::sort(sortedRoles.begin(), sortedRoles.end(), [](Role* a, Role* b) {
        return a->getAuthorityLevel() < b->getAuthorityLevel();
    });
    ApprovalLevel* headApproval = nullptr;
    ApprovalLevel* prevApproval = nullptr;
    for (auto* r : sortedRoles) {
        if (r->getAuthorityLevel() >= 2) {
            ApprovalLevel* lvl = new ApprovalLevel(r, 1800);
            if (!headApproval) headApproval = lvl;
            if (prevApproval) prevApproval->setNext(lvl);
            prevApproval = lvl;
        }
    }

    // Build Stage Tree (Composite Pattern)
    Stage* rootStage = new SequentialStage("root_workflow", model.name);
    std::map<std::string, WorkItem*> taskMap;

    for (const auto& grp : model.stageGroups) {
        Stage* stage = grp.isParallel ? static_cast<Stage*>(new ParallelStage(grp.id, grp.name))
                                      : static_cast<Stage*>(new SequentialStage(grp.id, grp.name));
        for (const auto& tid : grp.taskIds) {
            const TaskConfig* tc = model.findTask(tid);
            if (!tc) continue;
            Role* r = roles[tc->roleName];
            AssignmentStrategy* strat = &byRoleStrategy;
            if (tc->strategyType == "Workload") strat = &byWorkloadStrategy;
            else if (tc->strategyType == "RuleBased") strat = &ruleStrategy;

            Task* baseTask = nullptr;
            if (tc->isExternal) {
                ExternalConnector* conn = (tc->externalConnectorType == "APIOne_Records")
                    ? static_cast<ExternalConnector*>(&adapterOne)
                    : static_cast<ExternalConnector*>(&adapterTwo);
                baseTask = new ExternalTask(tc->id, tc->name, r, strat, conn, tc->externalRequest);
            } else {
                baseTask = new Task(tc->id, tc->name, r, strat);
            }

            WorkItem* decorated = baseTask;
            if (tc->decorators.hasValidation) {
                auto fields = tc->decorators.requiredFields;
                if (fields.empty()) fields.push_back("dataEntry");
                decorated = new ValidatedTask(decorated, fields);
            }
            if (tc->decorators.hasAuditing) {
                decorated = new AuditedTask(decorated, &publisher);
            }
            if (tc->decorators.hasPriority) {
                decorated = new PriorityTask(decorated, tc->decorators.priorityLevel);
            }
            stage->add(decorated);
            taskMap[tc->id] = decorated;
        }
        rootStage->add(stage);
    }
    rootStage->setPublisher(&publisher);

    WorkflowEvent startEvt(EventType::ITEM_STATE_CHANGED, rootStage->getItemID(), currentTimestamp(), "Started");
    publisher.publish(&startEvt);

    CommandInvoker invoker;

    // Helper to drive a single lifecycle transition for a task
    auto stepSingleTask = [&](WorkItem* task, Participant* chosenAssignee) {
        if (!task) return;
        const TaskConfig* tc = model.findTask(task->getItemID());
        std::string st = task->getStateName();

        if (st == "Created") {
            task->makeAvailable();
            st = task->getStateName();
        }

        if (st == "Available") {
            Participant* who = chosenAssignee;
            if (!who) {
                AssignmentStrategy* strat = &byRoleStrategy;
                if (tc && tc->strategyType == "Workload") strat = &byWorkloadStrategy;
                else if (tc && tc->strategyType == "RuleBased") strat = &ruleStrategy;

                Role* req = (tc && roles.count(tc->roleName)) ? roles[tc->roleName] : nullptr;
                Task dummyTask(task->getItemID(), task->getName(), req, strat);
                who = strat->selectParticipant(&dummyTask, participants);
                if (!who) {
                    for (auto* p : participants) {
                        if (!req || p->hasRole(req)) { who = p; break; }
                    }
                    if (!who && !participants.empty()) who = participants[0];
                }
            }
            invoker.executeCommand(new AssignCommand(task, who));
            std::cout << "  " << GREEN << "✔ [ASSIGNED]" << RESET << " \"" << task->getName() << "\" -> "
                      << (who ? who->getName() : "Unknown") << "\n";

            // Trigger approval chain if Priority 1
            if (tc && tc->decorators.hasPriority && tc->decorators.priorityLevel == 1 && headApproval) {
                std::cout << "     " << MAGENTA << "✦ Triggering Approval Chain (Priority 1)..." << RESET << "\n";
                ApprovalRequest req(task, 3, std::time(nullptr) + 1800);
                headApproval->handle(&req);
                std::cout << "     " << (req.getStatus() == ApprovalStatus::Approved ? GREEN + std::string("✔ Approved by Authority") : YELLOW + std::string("✦ Escalated in Chain")) << RESET << "\n";
            }
        } else if (st == "Assigned") {
            invoker.executeCommand(new StartCommand(task));
            std::cout << "  " << CYAN << "▶ [STARTED]" << RESET << " \"" << task->getName() << "\" by "
                      << (task->getAssignee() ? task->getAssignee()->getName() : "Assignee") << "\n";
        } else if (st == "Started") {
            if (tc && tc->decorators.hasValidation) {
                for (const auto& f : tc->decorators.requiredFields) {
                    if (!task->hasData(f) || task->getData(f).empty()) {
                        task->setData(f, "ValidRecord_" + f);
                        std::cout << "     " << DIM << "Validated field: " << f << RESET << "\n";
                    }
                }
            }
            invoker.executeCommand(new CompleteCommand(task));
            rootStage->activateChildren();
            std::cout << "  " << GREEN << "★ [COMPLETED]" << RESET << " \"" << task->getName() << "\"!\n";
        } else if (st == "Rejected") {
            task->makeAvailable();
            std::cout << "  " << YELLOW << "↺ [RE-ACTIVATED]" << RESET << " \"" << task->getName() << "\"\n";
        }
    };

    // Helper to drive a task through all transitions to completion
    auto driveTaskToCompletion = [&](WorkItem* task, Participant* assignee) {
        if (!task) return;
        while (!task->isFinished() && task->getStateName() != "Rejected") {
            stepSingleTask(task, assignee);
        }
    };

    rootStage->makeAvailable();
    rootStage->activateChildren();

    if (mode == 1) {
        // ===================================================================
        // Automated Demonstration Mode
        // ===================================================================
        printHeader(model.name, "Automated Workflow Demonstration Execution");
        std::cout << "  " << CYAN << BOLD << "Executing workflow with real-time observer telemetry..." << RESET << "\n\n";

        int guard = 0;
        while (!rootStage->isFinished() && ++guard < 120) {
            rootStage->activateChildren();
            std::vector<WorkItem*> ready = getActionableTasks(rootStage, taskMap);
            if (ready.empty()) break;

            for (auto* item : ready) {
                Participant* who = nullptr;
                const TaskConfig* tc = model.findTask(item->getItemID());
                if (tc && roles.count(tc->roleName)) {
                    Role* req = roles[tc->roleName];
                    for (auto* p : participants) {
                        if (p->hasRole(req)) { who = p; break; }
                    }
                }
                if (!who && !participants.empty()) who = participants[0];

                driveTaskToCompletion(item, who);
                rootStage->activateChildren();
            }
        }

        if (rootStage->isFinished()) {
            std::cout << "\n  " << GREEN << BOLD << "🎉 All workflow tasks have completed successfully!" << RESET << "\n";
            WorkflowEvent finishEvt(EventType::WORKFLOW_FINISHED, rootStage->getItemID(), currentTimestamp(), "All tasks completed successfully.");
            publisher.publish(&finishEvt);
        }
    } else {
        // ===================================================================
        // Interactive Step-by-Step Mode (Live Dashboard & Controls)
        // ===================================================================
        while (true) {
            rootStage->activateChildren();

            // Count completed
            int completedCount = 0;
            int totalTasks = static_cast<int>(model.tasks.size());
            for (const auto& pair : taskMap) {
                if (pair.second->getStateName() == "Completed") completedCount++;
            }

            printHeader(model.name, "Interactive Workflow Execution Dashboard");

            // Progress Bar
            int percent = (totalTasks > 0) ? (completedCount * 100 / totalTasks) : 100;
            std::cout << "  Progress: [";
            int barWidth = 32;
            int filled = (percent * barWidth) / 100;
            for (int b = 0; b < barWidth; ++b) {
                if (b < filled) std::cout << GREEN << "█" << RESET;
                else std::cout << DIM << "░" << RESET;
            }
            std::cout << "] " << BOLD << percent << "%" << RESET << " (" << completedCount << "/" << totalTasks << " tasks completed)\n\n";

            // Stage Groups & Tasks Board
            for (std::size_t g = 0; g < model.stageGroups.size(); ++g) {
                const auto& grp = model.stageGroups[g];
                std::string modeBadge = grp.isParallel ? (MAGENTA + std::string("[ParallelStage]") + RESET)
                                                       : (BLUE + std::string("[SequentialStage]") + RESET);
                std::cout << "  📂 " << BOLD << grp.name << RESET << " " << modeBadge << "\n";
                for (const auto& tid : grp.taskIds) {
                    auto it = taskMap.find(tid);
                    if (it == taskMap.end()) continue;
                    WorkItem* w = it->second;
                    std::cout << "     " << stateBadge(w->getStateName()) << " "
                              << pad(w->getName(), 36) << " ";
                    if (w->getAssignee()) {
                        std::cout << CYAN << "Owner: " << pad(w->getAssignee()->getName(), 18) << RESET;
                    } else {
                        std::cout << DIM << pad("Unassigned", 18) << RESET;
                    }
                    const TaskConfig* tc = model.findTask(tid);
                    if (tc && (tc->decorators.hasPriority || tc->decorators.hasValidation || tc->decorators.hasAuditing)) {
                        std::cout << " " << YELLOW << tc->decorators.summary() << RESET;
                    }
                    std::cout << "\n";
                }
                std::cout << "\n";
            }

            bool workflowFinished = rootStage->isFinished() || (completedCount == totalTasks);

            if (workflowFinished) {
                std::cout << "  " << GREEN << BOLD << "════════════════════════════════════════════════════════════════════════" << RESET << "\n";
                std::cout << "  " << GREEN << BOLD << "🎉 WORKFLOW COMPLETED! All " << totalTasks << " tasks have finished successfully." << RESET << "\n";
                std::cout << "  " << GREEN << BOLD << "════════════════════════════════════════════════════════════════════════" << RESET << "\n\n";

                WorkflowEvent finishEvt(EventType::WORKFLOW_FINISHED, rootStage->getItemID(), currentTimestamp(), "All tasks completed successfully.");
                publisher.publish(&finishEvt);

                std::cout << "  Post-Execution Actions:\n";
                std::cout << "    1) 📊 View Execution Results, History & Observer Telemetry (Audit, Notifier, Metrics)\n";
                std::cout << "    2) ↩ Undo Last Command (Command & Memento pattern)\n";
                std::cout << "    3) 💾 Save Completed Scenario to Disk\n";
                std::cout << "    0) ↩ Return to Main Menu\n";

                int endOpt = promptChoice("\n  Select action [0-3]: ", 0, 3);
                if (endOpt == 0) break;
                if (endOpt == 1) break; // proceeds to Telemetry display below
                if (endOpt == 2) {
                    if (invoker.canUndo() && invoker.undoLast()) {
                        std::cout << "  " << GREEN << "Undid last command." << RESET << "\n";
                        rootStage->activateChildren();
                    } else {
                        std::cout << "  " << YELLOW << "Nothing to undo." << RESET << "\n";
                    }
                    pause();
                    continue;
                }
                if (endOpt == 3) {
                    saveScenarioToDisk(model);
                    std::cout << "  " << GREEN << "Scenario saved to disk!" << RESET << "\n";
                    pause();
                    continue;
                }
            }

            // Actionable tasks
            std::vector<WorkItem*> actionable = getActionableTasks(rootStage, taskMap);

            std::cout << "  " << BOLD << "Currently Actionable Tasks (" << actionable.size() << "):" << RESET << "\n";
            if (actionable.empty()) {
                std::cout << "    " << DIM << "None (waiting for active stage transitions)" << RESET << "\n";
            } else {
                for (std::size_t i = 0; i < actionable.size(); ++i) {
                    WorkItem* w = actionable[i];
                    std::string nextAction;
                    if (w->getStateName() == "Available") nextAction = "Ready for Assignment";
                    else if (w->getStateName() == "Assigned") nextAction = "Ready to Start";
                    else if (w->getStateName() == "Started") nextAction = "Ready to Complete / Reject";
                    else if (w->getStateName() == "Rejected") nextAction = "Rejected (ready to re-activate)";

                    std::cout << "    " << (i + 1) << ") " << BOLD << w->getName() << RESET
                              << " " << stateBadge(w->getStateName())
                              << " " << DIM << "[" << nextAction << "]" << RESET << "\n";
                }
            }

            std::cout << "\n  Interactive Controls:\n";
            std::cout << "    1) Step & Advance next available task\n";
            std::cout << "    2) Pick specific task and execute action (Assign / Start / Complete / Reject)\n";
            std::cout << "    3) Run Approval Chain check (Chain of Responsibility)\n";
            std::cout << "    4) Undo last command (Command & Memento) [" << (invoker.canUndo() ? (GREEN + std::string("Available") + RESET) : (DIM + std::string("Empty") + RESET)) << "]\n";
            std::cout << "    5) Fast-forward all remaining tasks to completion\n";
            std::cout << "    0) Abort / Return to Scenario Menu\n";

            int c = promptChoice("\n  Select action [0-5]: ", 0, 5);
            if (c == 0) break;

            if (c == 1) {
                if (actionable.empty()) {
                    std::cout << "  " << YELLOW << "No actionable tasks right now." << RESET << "\n";
                    pause();
                    continue;
                }
                std::cout << "\n";
                stepSingleTask(actionable[0], nullptr);
                rootStage->activateChildren();
                pause("Press ENTER to continue...");
            } else if (c == 2) {
                std::vector<WorkItem*> allTasksList;
                std::cout << "\n  Select task to act on:\n";
                for (const auto& pair : taskMap) {
                    allTasksList.push_back(pair.second);
                    std::cout << "    " << allTasksList.size() << ") " << stateBadge(pair.second->getStateName())
                              << " " << pair.second->getName() << "\n";
                }
                int tChoice = promptChoice("  Task index [1-" + std::to_string(allTasksList.size()) + "]: ",
                                           1, static_cast<int>(allTasksList.size()));
                WorkItem* target = allTasksList[tChoice - 1];

                std::cout << "\n  Actions for \"" << target->getName() << "\" (Current: " << target->getStateName() << "):\n";
                std::cout << "    1) Assign task to participant\n";
                std::cout << "    2) Start task\n";
                std::cout << "    3) Complete task (enter validation payload)\n";
                std::cout << "    4) Reject task (record rejection reason)\n";
                std::cout << "    0) Cancel\n";

                int aChoice = promptChoice("  Action choice [0-4]: ", 0, 4);
                if (aChoice == 1) {
                    std::cout << "\n  Select participant:\n";
                    for (std::size_t i = 0; i < participants.size(); ++i) {
                        std::cout << "    " << (i + 1) << ") " << participants[i]->getName()
                                  << " [" << (participants[i]->getRole() ? participants[i]->getRole()->getName() : "Staff") << "]\n";
                    }
                    int pChoice = promptChoice("  Participant choice: ", 1, static_cast<int>(participants.size()));
                    try {
                        invoker.executeCommand(new AssignCommand(target, participants[pChoice - 1]));
                        std::cout << "  " << GREEN << "Assigned to " << participants[pChoice - 1]->getName() << RESET << "\n";
                    } catch (const std::exception& ex) {
                        std::cout << "  " << RED << "Error: " << ex.what() << RESET << "\n";
                    }
                    pause();
                } else if (aChoice == 2) {
                    try {
                        invoker.executeCommand(new StartCommand(target));
                        std::cout << "  " << GREEN << "Started task." << RESET << "\n";
                    } catch (const std::exception& ex) {
                        std::cout << "  " << RED << "Error: " << ex.what() << RESET << "\n";
                    }
                    pause();
                } else if (aChoice == 3) {
                    const TaskConfig* tc = model.findTask(target->getItemID());
                    if (tc && tc->decorators.hasValidation) {
                        std::cout << "  ValidatedTask required fields:\n";
                        for (const auto& f : tc->decorators.requiredFields) {
                            std::string val = promptString("    Enter value for '" + f + "': ", "Verified_" + f);
                            target->setData(f, val);
                        }
                    }
                    try {
                        invoker.executeCommand(new CompleteCommand(target));
                        rootStage->activateChildren();
                        std::cout << "  " << GREEN << "Task completed!" << RESET << "\n";
                    } catch (const std::exception& ex) {
                        std::cout << "  " << RED << "Error: " << ex.what() << RESET << "\n";
                    }
                    pause();
                } else if (aChoice == 4) {
                    std::string reason = promptString("  Rejection reason: ", "Discrepancy in records");
                    try {
                        target->reject(reason);
                        rootStage->activateChildren();
                        std::cout << "  " << YELLOW << "Task rejected with reason: " << reason << RESET << "\n";
                    } catch (const std::exception& ex) {
                        std::cout << "  " << RED << "Error: " << ex.what() << RESET << "\n";
                    }
                    pause();
                }
            } else if (c == 3) {
                if (headApproval) {
                    int reqLvl = promptInt("  Authority level required [1-5]: ", 3, 1, 5);
                    ApprovalRequest req(rootStage, reqLvl, std::time(nullptr) + 1800);
                    headApproval->handle(&req);
                    std::cout << "  Result: " << (req.getStatus() == ApprovalStatus::Approved ? GREEN + std::string("Approved by Authority") : RED + std::string("Pending / Escalated")) << RESET << "\n";
                    pause();
                } else {
                    std::cout << "  " << YELLOW << "No approval chain configured." << RESET << "\n";
                    pause();
                }
            } else if (c == 4) {
                if (invoker.canUndo() && invoker.undoLast()) {
                    std::cout << "  " << GREEN << "Undid last command using Command & Memento pattern." << RESET << "\n";
                    rootStage->activateChildren();
                } else {
                    std::cout << "  " << YELLOW << "Nothing to undo in history stack." << RESET << "\n";
                }
                pause();
            } else if (c == 5) {
                std::cout << "\n  " << CYAN << "Fast-forwarding remaining workflow tasks to completion..." << RESET << "\n";
                int guard = 0;
                while (!rootStage->isFinished() && ++guard < 150) {
                    rootStage->activateChildren();
                    std::vector<WorkItem*> act = getActionableTasks(rootStage, taskMap);
                    if (act.empty()) break;
                    for (auto* itm : act) {
                        driveTaskToCompletion(itm, nullptr);
                        rootStage->activateChildren();
                    }
                }
                std::cout << "  " << GREEN << "Completed fast-forward." << RESET << "\n";
                pause();
            }
        }
    }

    // =======================================================================
    // Final Results & Telemetry Output (Observers & Adapters Summary)
    // =======================================================================
    printHeader(model.name, "Execution Results, History & Observer Telemetry");

    std::cout << "  " << BOLD << CYAN << "[1] FINAL WORKFLOW STATUS" << RESET << "\n";
    for (const auto& grp : model.stageGroups) {
        std::cout << "    📂 " << BOLD << grp.name << RESET << " ("
                  << (grp.isParallel ? "ParallelStage" : "SequentialStage") << ")\n";
        for (const auto& tid : grp.taskIds) {
            auto it = taskMap.find(tid);
            if (it == taskMap.end()) continue;
            WorkItem* w = it->second;
            Participant* p = w->getAssignee();
            std::cout << "       • " << pad(w->getName(), 36) << " " << stateBadge(w->getStateName()) << " "
                      << DIM << (p ? "Owner: " + p->getName() : "Unassigned") << RESET << "\n";
        }
    }

    std::cout << "\n  " << BOLD << CYAN << "[2] AUDIT TRAIL (AuditLogger Observer: "
              << auditLogger.getRecords().size() << " records)" << RESET << "\n";
    auto records = auditLogger.getRecords();
    std::size_t startR = (records.size() > 8) ? records.size() - 8 : 0;
    for (std::size_t i = startR; i < records.size(); ++i) {
        std::cout << "      📋 " << DIM << records[i] << RESET << "\n";
    }

    std::cout << "\n  " << BOLD << CYAN << "[3] NOTIFIER BROADCASTS (Notifier Observer: "
              << notifier.getNotificationCount() << " notifications)" << RESET << "\n";
    auto notes = notifier.getNotifications();
    std::size_t startN = (notes.size() > 6) ? notes.size() - 6 : 0;
    for (std::size_t i = startN; i < notes.size(); ++i) {
        std::cout << "      🔔 " << notes[i] << "\n";
    }

    std::cout << "\n  " << BOLD << CYAN << "[4] METRICS SUMMARY (MetricsCollector Observer)" << RESET << "\n";
    std::cout << "      Completed Transitions: " << metricsCollector.getTransitionCount("Completed") << "\n";
    std::cout << "      Started Transitions:   " << metricsCollector.getTransitionCount("Started") << "\n";
    std::cout << "      Assigned Transitions:  " << metricsCollector.getTransitionCount("Assigned") << "\n";

    std::cout << "\n  " << BOLD << CYAN << "[5] EXTERNAL CONNECTORS (Adapter Pattern)" << RESET << "\n";
    for (const auto& t : model.tasks) {
        if (t.isExternal) {
            std::cout << "      • Connector: " << t.externalConnectorType << " | Payload: '"
                      << t.externalRequest << "' -> " << GREEN << "Successfully Delivered & Confirmed" << RESET << "\n";
        }
    }

    std::cout << "\n";
    printRule(72, CYAN);
    pause("Press ENTER to return to scenario menu...");

    if (model.scenarioType == "Custom" || promptBool("  Would you like to save this scenario to disk for future sessions?", false)) {
        saveScenarioToDisk(model);
        std::cout << "  " << GREEN << "Scenario saved to disk." << RESET << "\n";
        pause();
    }

    // Cleanup dynamically allocated resources
    delete rootStage;
    delete headApproval;
    for (auto* p : participants) delete p;
    for (auto& pair : roles) delete pair.second;
}

// ===========================================================================
// Menu Handlers for Hospital, Legal, Custom, and Saved Scenarios
// ===========================================================================
void handleHospitalScenario() {
    ScenarioModel model("Hospital", "Hospital Emergency Admission",
                        "Emergency patient admission workflow from paramedic pickup to cleaning and archiving.");
    auto defaultRoles = ScenarioModel::getHospitalDefaultRoles();
    for (const auto& r : defaultRoles) model.addRole(r.name, r.authorityLevel);

    model.addStageGroup("grp_transport", "Transport & Bay Preparation", true);
    model.addStageGroup("grp_arrival", "Arrival & Intake", false);
    model.addStageGroup("grp_treatment", "Assessment & Treatment", false);
    model.addStageGroup("grp_closeout", "Cleaning & Closeout", false);

    auto predefined = ScenarioModel::getHospitalPredefinedTasks();

    int execMode = 0;
    if (!step1ConfigurePersonnel(model)) return;
    if (!step2ConfigureTasks(model, predefined)) return;
    if (!step3ViewAndOrganizeTasks(model)) return;
    if (!step4ApplyDecorators(model)) return;
    if (!step5ReviewScenario(model, execMode)) return;

    executeScenarioWorkflow(model, execMode);
}

void handleLegalScenario() {
    ScenarioModel model("Legal", "Legal Case Management",
                        "Civil litigation matter lifecycle from client intake to court hearing and invoice.");
    auto defaultRoles = ScenarioModel::getLegalDefaultRoles();
    for (const auto& r : defaultRoles) model.addRole(r.name, r.authorityLevel);

    model.addStageGroup("grp_legal_intake", "Client Intake & Conflict Check", false);
    model.addStageGroup("grp_legal_onboarding", "Engagement & Trust Deposit", true);
    model.addStageGroup("grp_investigation", "Case Investigation & Discovery", true);
    model.addStageGroup("grp_pleadings", "Pleadings & Court Filing", false);
    model.addStageGroup("grp_judgment", "Judgment & Settlement", false);
    model.addStageGroup("grp_billing", "Billing & Case Archive", false);

    auto predefined = ScenarioModel::getLegalPredefinedTasks();

    int execMode = 0;
    if (!step1ConfigurePersonnel(model)) return;
    if (!step2ConfigureTasks(model, predefined)) return;
    if (!step3ViewAndOrganizeTasks(model)) return;
    if (!step4ApplyDecorators(model)) return;
    if (!step5ReviewScenario(model, execMode)) return;

    executeScenarioWorkflow(model, execMode);
}

void handleCreateCustomScenario() {
    printHeader("Create Your Own Scenario", "Initialize Custom Scenario");
    std::string name = promptString("  Enter scenario name [My Custom Workflow]: ", "My Custom Workflow");
    std::string desc = promptString("  Enter brief description: ", "Custom reusable workflow created via TUI");
    ScenarioModel model("Custom", name, desc);

    std::cout << "\n  Configure Roles for your custom workflow:\n";
    std::cout << "    1) Use standard professional roles (Manager, Specialist, Officer, Assistant, Admin)\n";
    std::cout << "    2) Define custom roles from scratch\n";
    int rChoice = promptChoice("  Select choice [1-2]: ", 1, 2);
    if (rChoice == 1) {
        model.addRole("Manager", 4);
        model.addRole("Specialist", 3);
        model.addRole("Officer", 2);
        model.addRole("Assistant", 1);
        model.addRole("Administrator", 5);
    } else {
        int rCount = promptInt("  Number of roles [1-10]: ", 3, 1, 10);
        for (int i = 1; i <= rCount; ++i) {
            std::string rName = promptString("    Role #" + std::to_string(i) + " name: ", "Role_" + std::to_string(i));
            int auth = promptInt("    Approval authority level (1-5): ", 1, 1, 5);
            model.addRole(rName, auth);
        }
    }

    model.addStageGroup("grp_phase1", "Phase 1 - Preparation", false);
    model.addStageGroup("grp_phase2", "Phase 2 - Execution", true);
    model.addStageGroup("grp_phase3", "Phase 3 - Wrap up", false);

    std::vector<TaskConfig> emptyPredefined;

    int execMode = 0;
    if (!step1ConfigurePersonnel(model)) return;
    if (!step2ConfigureTasks(model, emptyPredefined)) return;
    if (!step3ViewAndOrganizeTasks(model)) return;
    if (!step4ApplyDecorators(model)) return;
    if (!step5ReviewScenario(model, execMode)) return;

    if (promptBool("  Save this custom scenario for future sessions?", true)) {
        saveScenarioToDisk(model);
        std::cout << "  " << GREEN << "Scenario saved to disk!" << RESET << "\n";
        pause();
    }

    executeScenarioWorkflow(model, execMode);
}


void handleSavedScenariosMenu() {
    while (true) {
        printHeader("Saved Custom Scenarios", "Manage and Execute Persisted Workflows");
        auto saved = listSavedScenarios();
        if (saved.empty()) {
            std::cout << "  " << YELLOW << "No saved scenarios found on disk." << RESET << "\n";
            pause();
            return;
        }

        std::cout << "  " << BOLD << "Select a saved scenario:" << RESET << "\n\n";
        for (std::size_t i = 0; i < saved.size(); ++i) {
            std::cout << "    " << (i + 1) << ") " << pad(saved[i].name, 35)
                      << DIM << "[" << saved[i].type << " | " << saved[i].taskCount << " tasks | "
                      << saved[i].personnelCount << " staff]" << RESET << "\n";
        }
        std::cout << "\n    0) ↩ Back to Main Menu\n";

        int c = promptChoice("\n  Select scenario [0-" + std::to_string(saved.size()) + "]: ", 0, static_cast<int>(saved.size()));
        if (c == 0) return;

        std::string chosenFile = saved[c - 1].filename;
        ScenarioModel model;
        if (!loadScenarioFromDisk(chosenFile, model)) {
            std::cout << "  " << RED << "Error reading scenario file." << RESET << "\n";
            pause();
            continue;
        }

        while (true) {
            printHeader("Saved Scenario: " + model.name, "Manage Workflow");
            model.printSummary();

            std::cout << "  " << BOLD << "Available Operations:" << RESET << "\n";
            std::cout << "    1) ▶ Execute this scenario\n";
            std::cout << "    2) ✏ Edit this scenario (Personnel, Tasks, Groups, Decorators)\n";
            std::cout << "    3) 💾 Save changes\n";
            std::cout << "    4) 🗑 Delete this saved scenario from disk\n";
            std::cout << "    0) ↩ Back to Saved Scenarios List\n";

            int op = promptChoice("\n  Select operation [0-4]: ", 0, 4);
            if (op == 0) break;
            if (op == 1) {
                executeScenarioWorkflow(model);
            } else if (op == 2) {
                std::cout << "\n    1) Reconfigure Personnel  2) Modify Tasks  3) Modify Groups  4) Modify Decorators  0) Cancel\n";
                int eChoice = promptChoice("  Edit choice [0-4]: ", 0, 4);
                if (eChoice == 1) step1ConfigurePersonnel(model);
                else if (eChoice == 2) {
                    auto pre = (model.scenarioType == "Hospital") ? ScenarioModel::getHospitalPredefinedTasks()
                             : (model.scenarioType == "Legal" ? ScenarioModel::getLegalPredefinedTasks() : std::vector<TaskConfig>());
                    step2ConfigureTasks(model, pre);
                } else if (eChoice == 3) step3ViewAndOrganizeTasks(model);
                else if (eChoice == 4) step4ApplyDecorators(model);
            } else if (op == 3) {
                saveScenarioToDisk(model);
                std::cout << "  " << GREEN << "Changes saved to disk!" << RESET << "\n";
                pause();
            } else if (op == 4) {
                if (promptBool("  Are you sure you want to permanently delete this scenario?", false)) {
                    unlink(chosenFile.c_str());
                    std::cout << "  " << YELLOW << "Scenario deleted." << RESET << "\n";
                    pause();
                    break;
                }
            }
        }
    }
}

} // anonymous namespace

// ===========================================================================
// Application Main Entry Point
// ===========================================================================
int main() {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode = 0;
    if (hOut != INVALID_HANDLE_VALUE && GetConsoleMode(hOut, &mode)) {
        SetConsoleMode(hOut, mode | 0x0004);
    }
#endif

    resolveSavedDir();

    while (true) {
        printHeader("TIVIDY Main Menu", "Interactive Workflow Scenario System");

        auto saved = listSavedScenarios();

        std::cout << "  " << BOLD << "Please select an option:" << RESET << "\n\n";
        std::cout << "    " << CYAN << BOLD << "1)" << RESET << " 🏥 Hospital Scenario (Emergency Admission)\n";
        std::cout << "    " << CYAN << BOLD << "2)" << RESET << " ⚖️  Legal Scenario (Litigation Matter)\n";
        std::cout << "    " << CYAN << BOLD << "3)" << RESET << " ✨ Create Your Own Scenario\n";

        if (!saved.empty()) {
            std::cout << "    " << CYAN << BOLD << "4)" << RESET << " 📁 Saved Custom Scenarios (" << saved.size() << " saved)\n";
        }
        std::cout << "\n    " << RED << BOLD << "0)" << RESET << " 👋 Exit\n\n";

        int maxChoice = saved.empty() ? 3 : 4;
        int choice = promptChoice("  Enter your choice [0-" + std::to_string(maxChoice) + "]: ", 0, maxChoice);

        if (choice == 0) {
            std::cout << "\n  " << CYAN << "Thank you for using TIVIDY! Goodbye." << RESET << "\n\n";
            break;
        } else if (choice == 1) {
            handleHospitalScenario();
        } else if (choice == 2) {
            handleLegalScenario();
        } else if (choice == 3) {
            handleCreateCustomScenario();
        } else if (choice == 4) {
            handleSavedScenariosMenu();
        }
    }

    return 0;
}