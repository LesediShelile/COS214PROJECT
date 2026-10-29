#ifndef DIRECTOR_H
#define DIRECTOR_H

#include <iostream>
#include <string>
#include <vector>
using namespace std;
#include "WorkflowBuilder.h"
#include "DefinitionBuilder.h"

//forward declarations
class Role;
class RoleBasedAssignment;
class WorkloadAssignment;
class ExternalConnector;
class EventPublisher;

/**
 * @brief Builder Pattern: Director 
 * Directs the construction of a predefined workflow
 * 
 * Uses a workflow builder to create the workflow's stages
 * and tasks
 * Provides the roles when assigning those tasks
 */

class Director
{
    public:

        /**
         * @brief initialises roles, strategies and builder
         */
        Director();
        void construct(string name, ExternalConnector* ambulance, ExternalConnector* records, EventPublisher* publisher, bool includeCleaner);
        vector<Role*> getRoles();
        ~Director();
    private:
        WorkflowBuilder* machine;
        Role* paramedicRole;
        Role* nurseRole;
        Role* doctorRole;
        Role* secretaryRole;
        Role* cleanerRole;
        RoleBasedAssignment* roleBased;
        WorkloadAssignment* workload;
};
#endif //DIRECRTOR_H