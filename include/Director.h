#ifndef DIRECTOR_H
#define DIRECTOR_H

#include <iostream>
#include <string>
#include <vector>
using namespace std;

//classes
#include "WorkflowBuilder.h"
#include "Stub.h"
#include "DefnitionBuilder.h"

//forward declarations
class Role;
class RoleBasedAssignment;
class WorkloadAssignment;
class ExternalConnector;
class EventPublisher;

class Director
{
    public:
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