#include "Director.h"
#include "Role.h"
#include "RoleBasedAssignment.h"
#include "WorkloadAssignment.h"
#include "ExternalConnector.h"
#include "EventPublisher.h"

Director::Director()
{
    this->doctorRole = new Role("doctor", 3);
    this->cleanerRole = new Role("cleaner", 1);
    this->machine = new DefinitionBuilder();
    this->nurseRole = new Role("nurse", 2);
    this->paramedicRole = new Role("paramedic", 3);
    this->secretaryRole = new Role("secretary", 1);
    this->workload = nullptr;
    this->roleBased = new RoleBasedAssignment();
}

void Director::construct(string name, ExternalConnector* ambulance, ExternalConnector* records, EventPublisher* publisher, bool includeCleaner)
{
    machine->reset(name, 1);

    machine->beginSequentialStage("Triage");
    machine->addTask("Initial Assessment", nurseRole, roleBased);
    machine->commitPendingTask();
    machine->endStage();

    if(includeCleaner)
    {
        machine->beginSequentialStage("Sanitation Stage");
        machine->addTask("Deep Clean room", cleanerRole, roleBased);
        machine->commitPendingTask();
        machine->endStage();
    }
}

vector<Role*> Director::getRoles()
{
    vector<Role *> roles = {doctorRole, nurseRole, paramedicRole, cleanerRole, secretaryRole};
    return roles;
}

Director::~Director()
{
    delete this->doctorRole;
    delete this->cleanerRole;
    delete this->machine;
    delete this->nurseRole;
    delete this->paramedicRole;
    delete this->secretaryRole;
    delete this->workload;
    delete this->roleBased;
}