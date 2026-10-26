#ifndef EMERGENCY_ADMISSION_DIRECTOR_H
#define EMERGENY_ADMISSION_DIRECTOR_H

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

class EmergencyAdmissionDirector
{
    public:
        EmergencyAdmissionDirector();
        void construct(string name, ExternalConnector* ambulance, ExternalConnector* records, EventPublisher* publisher, bool includeCleaner);
        vector<Role*> getRoles();
        ~EmergencyAdmissionDirector();
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
#endif //EMERGENCY_ADMISSION_DIRECRTOR_H