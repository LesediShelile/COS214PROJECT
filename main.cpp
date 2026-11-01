// TIVIDY - Friendly interactive C++11 workflow showcase
// Features: guided tasks, error lab, workflow designer, undo/redo + checkpoints, design-pattern notes.
// Build from the project root with: g++ -std=c++11 -Iinclude main.cpp src/*.cpp -o tividy
#include <iostream>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <algorithm>
#include <stdexcept>
#include <cstdlib>

#include "include/DefinitionBuilder.h"
#include "include/Director.h"
#include "include/WorkflowDefinition.h"
#include "include/WorkflowInstance.h"
#include "include/WorkItem.h"
#include "include/WorkItemIterator.h"
#include "include/Participant.h"
#include "include/Role.h"
#include "include/RoleBasedAssignment.h"
#include "include/Task.h"
#include "include/TaskDecorator.h"

#ifdef _WIN32
#include <windows.h>
#endif

namespace {

// ---------------------------------------------------------------- look & feel
const char* RESET="\033[0m"; const char* BOLD="\033[1m"; const char* DIM="\033[2m";
const char* CYAN="\033[36m"; const char* GREEN="\033[32m"; const char* YELLOW="\033[33m";
const char* RED="\033[31m";  const char* MAGENTA="\033[35m"; const char* WHITE="\033[97m";

bool g_notes=true;   // show design-pattern notes?

void clearScreen(){
#ifdef _WIN32
    std::system("cls");
#else
    std::cout << "\033[2J\033[H";
#endif
}
void rule(int n=62){ std::cout<<DIM; for(int i=0;i<n;++i) std::cout<<"─"; std::cout<<RESET<<"\n"; }
std::string pad(const std::string& s, std::size_t n){
    return s.size()>=n ? s.substr(0,n) : s+std::string(n-s.size(),' ');
}
std::string num(std::size_t n){ std::ostringstream o; o<<n; return o.str(); }

void tivi(const std::string& msg, const char* colour=MAGENTA){
    std::cout<<"\n  "<<colour<<"(\\_/)"<<RESET<<"\n  "<<colour<<"(^.^)"<<RESET<<"  "<<msg<<"\n  "<<colour<<"c(\")(\")"<<RESET<<"\n";
}
void step(int n,int total,const std::string& text){
    std::cout<<"\n  "<<CYAN<<BOLD<<"Step "<<n<<" of "<<total<<RESET<<"  "<<BOLD<<text<<RESET<<"\n\n";
}
void tip(const std::string& t){ std::cout<<"\n  "<<DIM<<"💡 Tip: "<<t<<RESET<<"\n"; }

// A design-pattern note: shows which pattern is working behind the scenes
void pattern(const std::string& name,const std::string& text){
    if(!g_notes) return;
    std::cout<<"\n  "<<MAGENTA<<"🧠 "<<BOLD<<name<<" pattern"<<RESET<<DIM<<" · "<<text<<RESET<<"\n";
}

std::string niceState(const std::string& s){
    if(s=="Completed") return "Done";
    if(s=="Started")   return "In progress";
    if(s=="Assigned")  return "Assigned";
    if(s=="Rejected")  return "Sent back";
    if(s=="Cancelled") return "Cancelled";
    return "Waiting";
}
std::string stateIcon(const std::string& s){
    if(s=="Completed") return "✅";
    if(s=="Started")   return "🔵";
    if(s=="Assigned")  return "🟡";
    if(s=="Rejected")  return "🔴";
    if(s=="Cancelled") return "⚫";
    return "⚪";
}
const char* stateColour(const std::string& s){
    if(s=="Completed") return GREEN;
    if(s=="Rejected"||s=="Cancelled") return RED;
    if(s=="Started") return CYAN;
    if(s=="Assigned") return YELLOW;
    return DIM;
}

// ------------------------------------------------------------------ scenarios
struct Scenario {
    std::string name, subtitle, emoji, blurb;
    WorkflowDefinition* definition;
    std::vector<Participant*> people;
    std::vector<Role*> roles;
    Scenario():definition(0){}
    ~Scenario(){ delete definition; for(std::size_t i=0;i<people.size();++i) delete people[i]; for(std::size_t i=0;i<roles.size();++i) delete roles[i]; }
    Role* role(const std::string& n, int level){ Role* r=new Role(n,level); roles.push_back(r); return r; }
    void person(const std::string& id,const std::string& n,Role* r){ people.push_back(new Participant(id,n,r)); }
};

Scenario* makeHospital(AssignmentStrategy* strategy){
    Scenario* s=new Scenario; s->name="Hospital emergency admission"; s->subtitle="Emergency intake  |  Patient: Thabo Mokoena  |  Case: ER-2048";
    s->emoji="🏥"; s->blurb="An emergency patient arrives. Follow them from ambulance to treatment.";
    Role *para=s->role("Paramedic",1), *clerk=s->role("Admissions Clerk",1), *nurse=s->role("Nurse",2), *doctor=s->role("Doctor",3), *bed=s->role("Bed Manager",2), *clean=s->role("Porter",1);
    s->person("H01","Lerato Dlamini",para); s->person("H02","James Nkosi",clerk); s->person("H03","Nandi Khumalo",nurse); s->person("H04","Dr. A. Patel",doctor); s->person("H05","Sipho Maseko",bed); s->person("H06","Mia Jacobs",clean);
    DefinitionBuilder b; Director d(b);
    s->definition=d.construct(s->name,1,[&](WorkflowBuilder& w){
        w.beginSequentialStage("Emergency admission");
        w.addTask("Dispatch ambulance",para,strategy); w.withPriority(3); w.commitPendingTask();
        w.addTask("Register patient",clerk,strategy); w.withValidation(std::vector<std::string>(1,"patient-id")); w.commitPendingTask();
        w.beginParallelStage("Prepare and transport");
        w.addTask("Prepare emergency bay",nurse,strategy); w.withPriority(2); w.commitPendingTask();
        w.addTask("Transport patient to hospital",para,strategy); w.commitPendingTask();
        w.endStage();
        w.addTask("Clinical handover",nurse,strategy); w.commitPendingTask();
        w.addTask("Doctor assessment",doctor,strategy); w.withPriority(3); w.commitPendingTask();
        w.addTask("Approve room allocation",bed,strategy); w.commitPendingTask();
        w.addTask("Treatment and stabilisation",doctor,strategy); w.commitPendingTask();
        w.addTask("Clean and reset emergency bay",clean,strategy); w.commitPendingTask();
        w.endStage();
    }); s->definition->freeze(); return s;
}
Scenario* makeLegal(AssignmentStrategy* strategy){
    Scenario* s=new Scenario; s->name="Legal case management"; s->subtitle="Civil litigation  |  Client: M. Ndlovu  |  Matter: LGL-7712";
    s->emoji="⚖️"; s->blurb="A new client needs help. Follow the matter from intake to invoice.";
    Role *sec=s->role("Legal Secretary",1), *law=s->role("Attorney",2), *para=s->role("Paralegal",1), *partner=s->role("Senior Partner",3), *counsel=s->role("Advocate",2), *finance=s->role("Accounts Clerk",1);
    s->person("L01","Zanele Mokoena",sec); s->person("L02","Thabo van der Merwe",law); s->person("L03","Ayesha Khan",para); s->person("L04","Partner R. Botha",partner); s->person("L05","Adv. P. Dube",counsel); s->person("L06","Megan Smith",finance);
    DefinitionBuilder b; Director d(b);
    s->definition=d.construct(s->name,1,[&](WorkflowBuilder& w){
        w.beginSequentialStage("Matter lifecycle");
        w.addTask("Client intake and mandate",sec,strategy); w.withValidation(std::vector<std::string>(1,"signed-mandate")); w.commitPendingTask();
        w.addTask("Conflict of interest check",law,strategy); w.commitPendingTask();
        w.beginParallelStage("Discovery");
        w.addTask("Collect and index evidence",para,strategy); w.withPriority(2); w.commitPendingTask();
        w.addTask("Legal research and precedent review",law,strategy); w.commitPendingTask();
        w.endStage();
        w.addTask("Draft pleadings",law,strategy); w.withPriority(3); w.commitPendingTask();
        w.addTask("Partner review and approval",partner,strategy); w.commitPendingTask();
        w.addTask("File court documents",sec,strategy); w.commitPendingTask();
        w.addTask("Prepare hearing bundle",para,strategy); w.commitPendingTask();
        w.addTask("Attend hearing",counsel,strategy); w.commitPendingTask();
        w.addTask("Record judgment and advise client",law,strategy); w.commitPendingTask();
        w.addTask("Invoice and archive matter",finance,strategy); w.commitPendingTask();
        w.endStage();
    }); s->definition->freeze(); return s;
}

// --------------------------------------------- running workflow + command log
// Command pattern: every action is stored as a small object, so we can undo/redo.
// Memento pattern: a checkpoint is a saved snapshot of that command log.
struct Command {
    int task;            // index among the workflow's leaf tasks
    int action;          // 1 assign, 2 start, 3 complete, 4 reject
    int person;          // index into scenario->people (or -1)
    std::string reason;
    bool activate;       // was activateNext() called afterwards?
    int group;           // commands from one user step share a group (undone together)
    std::string label;
    Command():task(0),action(0),person(-1),activate(false),group(0){}
};
struct Checkpoint { std::string name; std::vector<Command> log; };

struct Running {
    Scenario* scenario; WorkflowInstance* instance;
    std::vector<std::string> history;
    std::string idBase; int generation; int nextGroup;
    std::vector<Command> log, redo;
    std::vector<Checkpoint> checkpoints;
    Running():scenario(0),instance(0),generation(0),nextGroup(0){}
};

// --------------------------------------------------------------- engine helpers
std::vector<WorkItem*> allItems(WorkItem* root){
    std::vector<WorkItem*> v;
    std::unique_ptr<WorkItemIterator> it(root->createIterator());
    for(it->first();!it->isDone();it->next()) if(it->current()) v.push_back(it->current());
    return v;
}
WorkItem* unwrapTask(WorkItem* item){
    TaskDecorator* d=dynamic_cast<TaskDecorator*>(item);
    while(d){ item=d->getWrapped(); d=dynamic_cast<TaskDecorator*>(item); }
    return item;
}
Role* requiredRole(WorkItem* item){
    Task* t=dynamic_cast<Task*>(unwrapTask(item));
    return t ? t->getRequiredRole() : 0;
}
std::vector<WorkItem*> leafItems(WorkflowInstance* inst){
    std::vector<WorkItem*> result, all=allItems(inst->getRoot());
    for(std::size_t i=0;i<all.size();++i) if(all[i]->getChildren().empty()) result.push_back(all[i]);
    return result;
}
std::vector<WorkItem*> leafItems(Running& r){ return leafItems(r.instance); }
bool contains(const std::vector<WorkItem*>& v, WorkItem* x){ return std::find(v.begin(),v.end(),x)!=v.end(); }
int countDone(Running& r,int& total){
    std::vector<WorkItem*> items=leafItems(r); int done=0; total=(int)items.size();
    for(std::size_t i=0;i<items.size();++i) if(items[i]->getStateName()=="Completed") ++done;
    return done;
}
const char* actionName(int a){ static const char* n[]={"","assign","start","complete","reject"}; return n[a]; }
void applyAction(WorkItem* w,int action,Participant* p,const std::string& reason){
    if(action==1) w->assign(p);
    else if(action==2) w->start();
    else if(action==3) w->complete();
    else w->reject(reason);
}
int taskIndex(WorkflowInstance* inst, WorkItem* w){
    std::vector<WorkItem*> v=leafItems(inst);
    for(std::size_t i=0;i<v.size();++i) if(v[i]==w) return (int)i;
    return -1;
}
int personIndex(Running& r, Participant* p){
    for(std::size_t i=0;i<r.scenario->people.size();++i) if(r.scenario->people[i]==p) return (int)i;
    return -1;
}
// Save a successful action as a command (clears the redo stack)
void record(Running& r, WorkItem* w,int action,Participant* p,const std::string& reason,bool activate,int group){
    int t=taskIndex(r.instance,w); if(t<0) return;
    Command c; c.task=t; c.action=action; c.person=p?personIndex(r,p):-1; c.reason=reason; c.activate=activate; c.group=group;
    c.label=std::string(actionName(action))+" '"+w->getName()+"'"+(p?std::string(" -> ")+p->getName():std::string());
    r.log.push_back(c); r.redo.clear();
}
// Rebuild the instance from scratch by replaying the first n commands
bool rebuild(Running& r,std::size_t n){
    try{
        std::string id=r.idBase+"-r"+num((std::size_t)(++r.generation));
        WorkflowInstance* inst=r.scenario->definition->createInstance(id);
        inst->start();
        for(std::size_t i=0;i<n && i<r.log.size();++i){
            const Command& c=r.log[i];
            std::vector<WorkItem*> items=leafItems(inst);
            if(c.task<0 || c.task>=(int)items.size()) throw std::runtime_error("replay mismatch");
            Participant* p=c.person>=0 ? r.scenario->people[c.person] : 0;
            applyAction(items[c.task],c.action,p,c.reason);
            if(c.activate) inst->activateNext();
        }
        r.instance=inst; return true;
    }catch(const std::exception& e){
        std::cout<<"\n  "<<RED<<"😿 Could not rebuild the workflow: "<<e.what()<<RESET<<"\n";
        return false;
    }
}

// ------------------------------------------------------------------- UI pieces
void progressBar(int done,int total,int width=30){
    int filled=total?done*width/total:0;
    std::cout<<"  "<<GREEN; for(int i=0;i<filled;++i) std::cout<<"█";
    std::cout<<DIM; for(int i=filled;i<width;++i) std::cout<<"░";
    std::cout<<RESET<<"  "<<done<<"/"<<total<<" tasks done";
    if(total) std::cout<<" ("<<done*100/total<<"%)";
    std::cout<<"\n";
}
void header(const Running* r){
    clearScreen();
    std::cout<<"\n  "<<CYAN<<BOLD<<"🌸 TIVIDY"<<RESET<<DIM<<"  ·  your friendly workflow helper"<<RESET<<"\n";
    rule();
    if(r){
        WorkflowStatus st=r->instance->getStatus();
        std::string status = st==WorkflowStatus::RUNNING ? "In progress" : st==WorkflowStatus::COMPLETED ? "Finished 🎉" : "Not started";
        std::cout<<"  "<<r->scenario->emoji<<"  "<<WHITE<<BOLD<<r->scenario->name<<RESET<<"\n"
                 <<"  "<<DIM<<r->scenario->subtitle<<RESET<<"\n"
                 <<"  Status: "<<GREEN<<status<<RESET<<DIM<<"   (id "<<r->instance->getInstanceID()<<")"<<RESET<<"\n";
        rule();
    }
}
void pause(const std::string& msg="Press ENTER to go back"){
    std::cout<<"\n  "<<DIM<<"↩  "<<msg<<"..."<<RESET; std::string x; std::getline(std::cin,x);
}
int choose(const std::string& prompt,int lo,int hi){
    for(;;){
        std::cout<<prompt; std::string x;
        if(!std::getline(std::cin,x)) return lo;
        std::stringstream ss(x); int n; char c;
        if((ss>>n) && !(ss>>c) && n>=lo && n<=hi) return n;
        std::cout<<"  "<<RED<<"Oops! Please type a number from "<<lo<<" to "<<hi<<" and press ENTER."<<RESET<<"\n";
    }
}
std::string ask(const std::string& prompt){
    std::cout<<prompt; std::string x; std::getline(std::cin,x);
    std::size_t a=x.find_first_not_of(" \t\r\n"); if(a==std::string::npos) return "";
    std::size_t b=x.find_last_not_of(" \t\r\n"); return x.substr(a,b-a+1);
}

void printTree(WorkItem* item,int depth,const std::vector<WorkItem*>& ready){
    std::string indent(depth*3+2,' ');
    std::vector<WorkItem*> kids=item->getChildren();
    if(!kids.empty()){
        std::cout<<indent<<"📂 "<<BOLD<<item->getName()<<RESET<<"\n";
        for(std::size_t i=0;i<kids.size();++i) printTree(kids[i],depth+1,ready);
    }else{
        std::string st=item->getStateName();
        std::cout<<indent<<stateIcon(st)<<" "<<pad(item->getName(),36)<<stateColour(st)<<pad(niceState(st),12)<<RESET;
        if(contains(ready,item)) std::cout<<YELLOW<<"👈 ready!"<<RESET;
        std::cout<<"\n";
    }
}

// Run one task through its whole life, explaining each step and saving commands
bool runTask(Running& r, WorkItem* w, Participant* p){
    std::vector<WorkItem*> before=r.instance->getAvailableItems();
    int g=++r.nextGroup;
    try{
        std::cout<<"\n  "<<YELLOW<<"1️⃣  Assigning"<<RESET<<"  → "<<p->getName()<<" takes the task...\n";
        w->assign(p);  record(r,w,1,p,"",false,g); r.history.push_back("Assigned '"+w->getName()+"' to "+p->getName());
        std::cout<<"  "<<CYAN<<"2️⃣  Starting"<<RESET<<"   → "<<p->getName()<<" is working on it...\n";
        w->start();    record(r,w,2,p,"",false,g); r.history.push_back("Started '"+w->getName()+"'");
        std::cout<<"  "<<GREEN<<"3️⃣  Finishing"<<RESET<<"  → all done!\n";
        w->complete(); r.instance->activateNext(); record(r,w,3,p,"",true,g);
        r.history.push_back("Completed '"+w->getName()+"' ("+p->getName()+")");
    }catch(const std::exception& e){
        std::cout<<"\n  "<<RED<<BOLD<<"🛡️  Blocked! The engine refused that action."<<RESET<<"\n"
                 <<"  "<<RED<<"Reason: "<<e.what()<<RESET<<"\n"
                 <<"  "<<DIM<<"This is error handling: the workflow stays safe and nothing is corrupted."<<RESET<<"\n";
        r.history.push_back("BLOCKED on '"+w->getName()+"' ("+p->getName()+"): "+e.what());
        pattern("State","a task's current state decides which moves are legal, so illegal ones are refused.");
        return false;
    }
    std::vector<WorkItem*> after=r.instance->getAvailableItems();
    std::vector<std::string> fresh;
    for(std::size_t i=0;i<after.size();++i) if(!contains(before,after[i])) fresh.push_back(after[i]->getName());
    if(r.instance->isFinished()){
        std::cout<<"\n  "<<GREEN<<BOLD<<"🎉 That was the last task. The whole workflow is finished!"<<RESET<<"\n";
    }else if(!fresh.empty()){
        std::cout<<"\n  "<<GREEN<<"🔓 The engine unlocked:"<<RESET<<"\n";
        for(std::size_t i=0;i<fresh.size();++i) std::cout<<"     ✨ "<<fresh[i]<<"\n";
    }else{
        std::cout<<"\n  "<<DIM<<"Other tasks in this stage must finish before the next one unlocks."<<RESET<<"\n";
    }
    pattern("State","the task moved Waiting → Assigned → In progress → Done, one legal move at a time.");
    pattern("Composite","stages contain tasks, and the engine walks that tree to decide what unlocks next.");
    pattern("Command","your step was saved as commands. Open the Time machine to undo it.");
    return true;
}

// ---------------------------------------------------------------------- screens
void showHelp(){
    header(0);
    std::cout<<"  "<<BOLD<<"📖 How does TIVIDY work?"<<RESET<<"\n\n"
             <<"  A "<<BOLD<<"workflow"<<RESET<<" is a recipe for getting a job done: a list of\n"
             <<"  tasks that must happen in the right order.\n\n"
             <<"  🧩 "<<BOLD<<"Tasks"<<RESET<<"      Small jobs, e.g. \"Register patient\".\n"
             <<"  👥 "<<BOLD<<"Roles"<<RESET<<"      Every task needs a role (Nurse, Attorney...).\n"
             <<"  🧑 "<<BOLD<<"People"<<RESET<<"     Only people with the right role can do a task.\n"
             <<"  📂 "<<BOLD<<"Stages"<<RESET<<"     Groups of tasks. Some run one after another,\n"
             <<"               some can be worked on at the same time.\n"
             <<"  🔓 "<<BOLD<<"Unlocking"<<RESET<<"  When a task is done, the engine unlocks what is next.\n\n"
             <<"  "<<BOLD<<"A task's life:"<<RESET<<"  ⚪ Waiting → 🟡 Assigned → 🔵 In progress → ✅ Done\n"
             <<"                  (or 🔴 Sent back if it needs correcting)\n\n"
             <<"  "<<BOLD<<"Quick start:"<<RESET<<"\n"
             <<"   1. Pick a story, or create your own workflow ✨\n"
             <<"   2. Choose \"Do the next task\" and follow the 3 easy steps.\n"
             <<"      You may pick ANY task or person. Invalid picks are refused safely.\n"
             <<"   3. Made a mistake? Use the Time machine to undo, redo or save checkpoints.\n"
             <<"   4. Curious how it's built? Pattern notes 🧠 explain each step,\n"
             <<"      and the Pattern guide lists them all.\n";
    pause();
}

void showPatternGuide(){
    header(0);
    std::cout<<"  "<<BOLD<<"🧠 Design patterns in TIVIDY"<<RESET<<"\n\n";
    struct Row{ const char* icon; const char* name; const char* what; const char* where; };
    Row rows[]={
        {"🔁","Iterator","WorkItemIterator visits every item without knowing the tree's shape.","Big picture, task lists"},
        {"🌳","Composite","Stages and tasks are both work items; stages contain children.","Big picture, unlocking"},
        {"🎁","Decorator","TaskDecorator wraps a task to add priority/validation.","Designer, task step 3"},
        {"🎯","Strategy","RoleBasedAssignment is a swappable rule for who may take a task.","Choosing a person"},
        {"🚦","State","A task only allows certain moves in each state.","Error lab, blocked actions"},
        {"🏗️","Builder + Director","WorkflowBuilder assembles; the Director runs the recipe.","Create my own workflow"},
        {"🏭","Factory","createInstance() makes a fresh run from a frozen definition.","Starting a story"},
        {"📝","Command","Each action is saved as an object, so it can be undone.","Time machine"},
        {"📸","Memento","A checkpoint snapshots the command log to restore later.","Time machine"}
    };
    for(std::size_t i=0;i<sizeof(rows)/sizeof(rows[0]);++i)
        std::cout<<"  "<<rows[i].icon<<" "<<BOLD<<pad(rows[i].name,18)<<RESET<<rows[i].what<<"\n"
                 <<"     "<<DIM<<"See it in: "<<rows[i].where<<RESET<<"\n\n";
    std::cout<<"  "<<DIM<<"In this demo, Command and Memento live in the menu layer: undo replays a\n  saved command log on a fresh instance."<<RESET<<"\n";
    pause();
}

void showMap(Running& r){
    header(&r);
    int total; int done=countDone(r,total);
    std::cout<<"  "<<BOLD<<"🗺️  The big picture"<<RESET<<"\n\n";
    progressBar(done,total);
    std::cout<<"\n";
    printTree(r.instance->getRoot(),0,r.instance->getAvailableItems());
    std::cout<<"\n  "<<DIM<<"Legend:  ⚪ waiting  🟡 assigned  🔵 in progress  ✅ done  🔴 sent back"<<RESET<<"\n";
    pattern("Iterator","WorkItemIterator walked every item to count your progress.");
    pattern("Composite","the indented tree is stages (📂) containing tasks, built from the same WorkItem type.");
    pause();
}

void doNextTask(Running& r){
    header(&r);
    if(r.instance->isFinished()){ tivi("Everything is done! Pick another story from the menu.",GREEN); pause(); return; }
    std::vector<WorkItem*> items=leafItems(r);
    std::vector<WorkItem*> ready=r.instance->getAvailableItems();

    step(1,3,"Pick any task");
    for(std::size_t i=0;i<items.size();++i){
        std::string st=items[i]->getStateName(); Role* need=requiredRole(items[i]);
        std::cout<<"  "<<(i+1<10?" ":"")<<i+1<<")  "<<stateIcon(st)<<" "<<pad(items[i]->getName(),36)
                 <<(contains(ready,items[i])? std::string(GREEN)+"ready"+RESET : std::string(DIM)+niceState(st)+RESET);
        if(need) std::cout<<DIM<<"   ("<<need->getName()<<")"<<RESET;
        std::cout<<"\n";
    }
    std::cout<<"   0)  ↩  Back\n";
    tip("Try a task that isn't ready, or the wrong person. The engine should refuse, and that's error handling!");
    pattern("Iterator","this list was produced by walking the workflow with WorkItemIterator.");
    int pick=choose("\n  Which task? 👉 ",0,(int)items.size()); if(!pick) return;
    WorkItem* w=items[pick-1]; Role* needed=requiredRole(w);
    bool isReady=contains(ready,w);

    header(&r);
    step(2,3,"Who should do it?");
    std::cout<<"  Task: "<<WHITE<<BOLD<<w->getName()<<RESET<<"\n";
    if(needed) std::cout<<"  "<<DIM<<"Needs the role: "<<needed->getName()<<RESET<<"\n";
    std::cout<<"\n";
    std::vector<Participant*>& all=r.scenario->people;
    for(std::size_t i=0;i<all.size();++i){
        bool ok=!needed||all[i]->hasRole(needed);
        std::cout<<"  "<<i+1<<")  🧑 "<<pad(all[i]->getName(),24)<<pad(all[i]->getRole()->getName(),18)
                 <<(ok? std::string(GREEN)+"✔ right role"+RESET : std::string(RED)+"✘ wrong role"+RESET)<<"\n";
    }
    std::cout<<"  0)  ↩  Cancel\n";
    pattern("Strategy","RoleBasedAssignment is the rule deciding who qualifies. It could be swapped for another rule.");
    int who=choose("\n  Which person? 👉 ",0,(int)all.size()); if(!who) return;
    Participant* p=all[who-1];
    bool roleOk=!needed||p->hasRole(needed);

    header(&r);
    step(3,3,"Let's try it!");
    std::cout<<"  🧩 Task:    "<<BOLD<<w->getName()<<RESET<<"\n  🧑 Person:  "<<BOLD<<p->getName()<<RESET<<DIM<<"  ("<<p->getRole()->getName()<<")"<<RESET<<"\n";
    if(!isReady) std::cout<<"\n  "<<YELLOW<<"⚠️  This task is not ready yet. The engine may refuse."<<RESET<<"\n";
    if(!roleOk)  std::cout<<"  "<<YELLOW<<"⚠️  This person doesn't have the required role. The engine may refuse."<<RESET<<"\n";
    if(dynamic_cast<TaskDecorator*>(w))
        pattern("Decorator","this task is wrapped (e.g. priority or validation) without changing the Task class itself.");
    std::cout<<"\n  1) ✅ Go!\n  0) ↩  Cancel\n";
    if(!choose("\n  Ready? 👉 ",0,1)) return;
    if(runTask(r,w,p)){
        if(!isReady||!roleOk) std::cout<<"\n  "<<YELLOW<<"🤔 Surprise: the engine allowed it even though it looked invalid."<<RESET<<"\n";
        int t; int d=countDone(r,t); std::cout<<"\n"; progressBar(d,t);
    }
    pause();
}

void errorLab(Running& r){
    for(;;){
        header(&r);
        std::cout<<"  "<<BOLD<<"🧪 Error lab: try to break it!"<<RESET<<"\n  "<<DIM<<"Pick any task, person and single action, even impossible ones.\n  Watch how the engine protects the workflow."<<RESET<<"\n\n";
        std::vector<WorkItem*> items=leafItems(r);
        for(std::size_t i=0;i<items.size();++i){
            std::string st=items[i]->getStateName();
            std::cout<<"  "<<(i+1<10?" ":"")<<i+1<<")  "<<stateIcon(st)<<" "<<pad(items[i]->getName(),36)<<stateColour(st)<<niceState(st)<<RESET<<"\n";
        }
        std::cout<<"   0)  ↩  Back\n";
        int pick=choose("\n  Which task? 👉 ",0,(int)items.size()); if(!pick) return;
        WorkItem* w=items[pick-1];

        std::cout<<"\n  "<<BOLD<<"What should happen to \""<<w->getName()<<"\"?"<<RESET<<"\n\n"
                 <<"  1)  🟡 Assign it to someone\n  2)  🔵 Start it\n  3)  ✅ Complete it\n  4)  🔴 Send it back (reject)\n  0)  ↩  Back\n";
        int act=choose("\n  Which action? 👉 ",0,4); if(!act) continue;

        Participant* p=0;
        if(act==1){
            Role* needed=requiredRole(w);
            std::cout<<"\n  "<<BOLD<<"Who?"<<RESET<<"\n\n";
            std::vector<Participant*>& all=r.scenario->people;
            for(std::size_t i=0;i<all.size();++i){
                bool ok=!needed||all[i]->hasRole(needed);
                std::cout<<"  "<<i+1<<")  🧑 "<<pad(all[i]->getName(),24)<<pad(all[i]->getRole()->getName(),18)
                         <<(ok? std::string(GREEN)+"✔"+RESET : std::string(RED)+"✘ wrong role"+RESET)<<"\n";
            }
            std::cout<<"  0)  ↩  Back\n";
            int who=choose("\n  Which person? 👉 ",0,(int)all.size()); if(!who) continue;
            p=all[who-1];
        }
        std::string reason;
        if(act==4){ reason=ask("\n  Why? (ENTER = \"Needs correction\") "); if(reason.empty()) reason="Needs correction"; }

        std::string before=w->getStateName();
        std::cout<<"\n  ▶ Trying to "<<BOLD<<actionName(act)<<RESET<<" '"<<w->getName()<<"' (currently "<<niceState(before)<<")...\n";
        try{
            applyAction(w,act,p,reason);
            r.instance->activateNext();
            record(r,w,act,p,reason,true,++r.nextGroup);
            std::cout<<"\n  "<<GREEN<<BOLD<<"✅ The engine allowed it."<<RESET<<"  "<<niceState(before)<<" → "<<niceState(w->getStateName())<<"\n";
            r.history.push_back(std::string("Lab: ")+actionName(act)+" '"+w->getName()+"' allowed");
        }catch(const std::exception& e){
            std::cout<<"\n  "<<RED<<BOLD<<"🛡️  Blocked! The engine refused."<<RESET<<"\n  "<<RED<<"Reason: "<<e.what()<<RESET<<"\n"
                     <<"  "<<DIM<<"State is unchanged: still "<<niceState(w->getStateName())<<"."<<RESET<<"\n";
            r.history.push_back(std::string("Lab: ")+actionName(act)+" '"+w->getName()+"' BLOCKED: "+e.what());
            pattern("State","each state allows only certain moves. Anything else is refused and the task stays as it was.");
        }
        pause("Press ENTER to try something else");
    }
}

void autoRun(Running& r){
    header(&r);
    tivi("Sit back! I'll run every task for you. Press ENTER after each one.");
    pause("Press ENTER to begin");
    int guard=0;
    for(;;){
        std::vector<WorkItem*> avail=r.instance->getAvailableItems();
        if(avail.empty()||++guard>200) break;
        WorkItem* w=avail[0]; Role* need=requiredRole(w);
        Participant* who=0;
        for(std::size_t i=0;i<r.scenario->people.size() && !who;++i)
            if(!need || r.scenario->people[i]->hasRole(need)) who=r.scenario->people[i];
        if(!who) break;
        header(&r);
        std::cout<<"  🧩 "<<BOLD<<w->getName()<<RESET<<DIM<<"   → "<<who->getName()<<" ("<<who->getRole()->getName()<<")"<<RESET<<"\n";
        if(!runTask(r,w,who)) break;
        int t; int d=countDone(r,t); std::cout<<"\n"; progressBar(d,t);
        if(r.instance->isFinished()) break;
        pause("Press ENTER for the next task");
    }
    if(r.instance->isFinished()) tivi("All done! 🎉 Check \"What happened so far\" for the full story.",GREEN);
    pause();
}

void showHistory(Running& r){
    header(&r);
    std::cout<<"  "<<BOLD<<"📜 What happened so far"<<RESET<<"\n\n";
    if(r.history.empty()) std::cout<<"  Nothing yet.\n";
    for(std::size_t i=0;i<r.history.size();++i) std::cout<<"  "<<DIM<<(i+1<10?" ":"")<<i+1<<"."<<RESET<<" "<<r.history[i]<<"\n";
    std::cout<<"\n  Outcome so far: "<<r.instance->getOutcome()<<"\n";
    pause();
}

void rejectTask(Running& r){
    std::vector<WorkItem*> avail=r.instance->getAvailableItems(); header(&r);
    if(avail.empty()){ tivi("There's nothing ready to send back right now.",YELLOW); pause(); return; }
    std::cout<<"  "<<BOLD<<"🔴 Send a task back for correction"<<RESET<<"\n  "<<DIM<<"Use this when something isn't right and needs another look."<<RESET<<"\n\n";
    for(std::size_t i=0;i<avail.size();++i) std::cout<<"  "<<i+1<<")  "<<avail[i]->getName()<<"\n";
    std::cout<<"  0)  ↩  Back\n";
    int n=choose("\n  Which task? 👉 ",0,(int)avail.size()); if(!n) return;
    std::string reason=ask("  Why? (ENTER = \"Needs correction\") "); if(reason.empty()) reason="Needs correction";
    try{
        avail[n-1]->reject(reason);
        r.instance->activateNext();
        record(r,avail[n-1],4,0,reason,true,++r.nextGroup);
        r.history.push_back("Returned '"+avail[n-1]->getName()+"': "+reason);
        tivi("Done! The task was sent back with your note.",GREEN);
        pattern("Command","this was saved as a command, so you can undo it from the Time machine.");
    }catch(const std::exception& e){ std::cout<<"\n  "<<RED<<"😿 "<<e.what()<<RESET<<"\n"; }
    pause();
}

// ---------------------------------------------------------------- time machine
void undoStep(Running& r){
    if(r.log.empty()){ std::cout<<"\n  "<<YELLOW<<"Nothing to undo yet."<<RESET<<"\n"; return; }
    std::vector<Command> logBak=r.log, redoBak=r.redo;
    int g=r.log.back().group; std::vector<Command> moved;
    while(!r.log.empty() && r.log.back().group==g){ moved.push_back(r.log.back()); r.redo.push_back(r.log.back()); r.log.pop_back(); }
    if(rebuild(r,r.log.size())){
        std::string what=moved.back().label; if(moved.size()>1) what+=" (+"+num(moved.size()-1)+" more)";
        r.history.push_back("UNDO: "+what);
        std::cout<<"\n  "<<GREEN<<"⏪ Undone: "<<RESET<<what<<"\n";
    }else{ r.log=logBak; r.redo=redoBak; }
}
void redoStep(Running& r){
    if(r.redo.empty()){ std::cout<<"\n  "<<YELLOW<<"Nothing to redo."<<RESET<<"\n"; return; }
    std::vector<Command> logBak=r.log, redoBak=r.redo;
    int g=r.redo.back().group; std::vector<Command> moved;
    while(!r.redo.empty() && r.redo.back().group==g){ moved.push_back(r.redo.back()); r.log.push_back(r.redo.back()); r.redo.pop_back(); }
    if(rebuild(r,r.log.size())){
        std::string what=moved.front().label; if(moved.size()>1) what+=" (+"+num(moved.size()-1)+" more)";
        r.history.push_back("REDO: "+what);
        std::cout<<"\n  "<<GREEN<<"⏩ Redone: "<<RESET<<what<<"\n";
    }else{ r.log=logBak; r.redo=redoBak; }
}
void timeMachine(Running& r){
    for(;;){
        header(&r);
        std::cout<<"  "<<BOLD<<"🕰️  Time machine"<<RESET<<"\n  "<<DIM<<"Undo mistakes, redo them, or save a checkpoint to come back to."<<RESET<<"\n\n";
        std::cout<<"  Saved steps: "<<BOLD<<r.log.size()<<RESET<<"    Can redo: "<<BOLD<<r.redo.size()<<RESET<<"    Checkpoints: "<<BOLD<<r.checkpoints.size()<<RESET<<"\n\n";
        std::size_t from=r.log.size()>6 ? r.log.size()-6 : 0;
        if(r.log.empty()) std::cout<<"  "<<DIM<<"(no steps yet. Do a task first!)"<<RESET<<"\n";
        else std::cout<<"  "<<BOLD<<"Latest steps:"<<RESET<<"\n";
        for(std::size_t i=from;i<r.log.size();++i) std::cout<<"   "<<DIM<<num(i+1)<<"."<<RESET<<" "<<r.log[i].label<<"\n";
        if(!r.checkpoints.empty()){
            std::cout<<"\n  "<<BOLD<<"Checkpoints:"<<RESET<<"\n";
            for(std::size_t i=0;i<r.checkpoints.size();++i) std::cout<<"   📸 "<<r.checkpoints[i].name<<DIM<<"  ("<<r.checkpoints[i].log.size()<<" steps)"<<RESET<<"\n";
        }
        pattern("Command","each saved step is a command object. Undo rebuilds the workflow and replays all but the last step.");
        pattern("Memento","a checkpoint is a snapshot of the command log. Restoring it brings that exact moment back.");
        std::cout<<"\n  1)  ⏪ Undo last step\n  2)  ⏩ Redo\n  3)  📸 Save a checkpoint\n  4)  🔙 Restore a checkpoint\n  0)  ↩  Back\n";
        int c=choose("\n  Your choice 👉 ",0,4);
        if(c==0) return;
        if(c==1){ undoStep(r); pause("Press ENTER to continue"); }
        else if(c==2){ redoStep(r); pause("Press ENTER to continue"); }
        else if(c==3){
            std::string nm=ask("\n  Name this checkpoint (ENTER = automatic): ");
            if(nm.empty()) nm="Checkpoint "+num(r.checkpoints.size()+1);
            Checkpoint cp; cp.name=nm; cp.log=r.log; r.checkpoints.push_back(cp);
            r.history.push_back("Saved checkpoint '"+nm+"'");
            std::cout<<"\n  "<<GREEN<<"📸 Saved \""<<nm<<"\"."<<RESET<<"\n"; pause("Press ENTER to continue");
        }else if(c==4){
            if(r.checkpoints.empty()){ std::cout<<"\n  "<<YELLOW<<"No checkpoints yet. Save one first!"<<RESET<<"\n"; pause("Press ENTER to continue"); continue; }
            std::cout<<"\n";
            for(std::size_t i=0;i<r.checkpoints.size();++i) std::cout<<"  "<<i+1<<")  📸 "<<r.checkpoints[i].name<<DIM<<"  ("<<r.checkpoints[i].log.size()<<" steps)"<<RESET<<"\n";
            std::cout<<"  0)  ↩  Back\n";
            int k=choose("\n  Restore which one? 👉 ",0,(int)r.checkpoints.size()); if(!k) continue;
            std::vector<Command> logBak=r.log, redoBak=r.redo;
            r.log=r.checkpoints[k-1].log; r.redo.clear();
            if(rebuild(r,r.log.size())){
                r.history.push_back("Restored checkpoint '"+r.checkpoints[k-1].name+"'");
                std::cout<<"\n  "<<GREEN<<"🔙 Back at \""<<r.checkpoints[k-1].name<<"\"."<<RESET<<"\n";
            }else{ r.log=logBak; r.redo=redoBak; }
            pause("Press ENTER to continue");
        }
    }
}

// ------------------------------------------------------------ workflow designer
struct DesignStep {
    int kind;                 // 0 task, 1 begin parallel group, 2 end group
    std::string name; int role; int priority; std::string field;
    DesignStep():kind(0),role(0),priority(0){}
};
struct Design {
    std::string name;
    std::vector<std::string> roleNames; std::vector<int> roleLevels;
    std::vector<std::string> personNames; std::vector<int> personRoles;
    std::vector<DesignStep> steps;
};
int openGroups(const Design& d){
    int o=0; for(std::size_t i=0;i<d.steps.size();++i){ if(d.steps[i].kind==1) ++o; else if(d.steps[i].kind==2) --o; } return o;
}
int taskCount(const Design& d){ int n=0; for(std::size_t i=0;i<d.steps.size();++i) if(d.steps[i].kind==0) ++n; return n; }
void addRole(Design& d,const std::string& n,int lvl){ d.roleNames.push_back(n); d.roleLevels.push_back(lvl); }
void addPerson(Design& d,const std::string& n,int role){ d.personNames.push_back(n); d.personRoles.push_back(role); }
void addTaskStep(Design& d,const std::string& n,int role,int pr,const std::string& field){
    DesignStep s; s.kind=0; s.name=n; s.role=role; s.priority=pr; s.field=field; d.steps.push_back(s);
}
void addMarker(Design& d,int kind,const std::string& n){ DesignStep s; s.kind=kind; s.name=n; d.steps.push_back(s); }

void loadStarter(Design& d){
    d=Design(); d.name="Birthday party";
    addRole(d,"Host",2); addRole(d,"Helper",1); addRole(d,"Baker",1);
    addPerson(d,"Sam",0); addPerson(d,"Priya",1); addPerson(d,"Leo",2); addPerson(d,"Zoe",1);
    addTaskStep(d,"Choose date and guest list",0,3,"");
    addMarker(d,1,"Get things ready");
    addTaskStep(d,"Bake the cake",2,2,"");
    addTaskStep(d,"Blow up balloons",1,0,"");
    addMarker(d,2,"");
    addTaskStep(d,"Send invitations",0,0,"guest-list");
    addTaskStep(d,"Throw the party!",0,0,"");
}
void renderDesign(const Design& d){
    std::cout<<"  "<<BOLD<<"📋 "<<d.name<<RESET<<"\n\n";
    std::cout<<"  👥 Roles:  ";
    if(d.roleNames.empty()) std::cout<<DIM<<"none yet"<<RESET;
    for(std::size_t i=0;i<d.roleNames.size();++i) std::cout<<d.roleNames[i]<<DIM<<" (level "<<d.roleLevels[i]<<")"<<RESET<<(i+1<d.roleNames.size()?", ":"");
    std::cout<<"\n  🧑 People: ";
    if(d.personNames.empty()) std::cout<<DIM<<"none yet"<<RESET;
    for(std::size_t i=0;i<d.personNames.size();++i) std::cout<<d.personNames[i]<<DIM<<" ["<<d.roleNames[d.personRoles[i]]<<"]"<<RESET<<(i+1<d.personNames.size()?", ":"");
    std::cout<<"\n\n  🧩 Steps:\n";
    if(d.steps.empty()) std::cout<<"     "<<DIM<<"(no steps yet)"<<RESET<<"\n";
    int depth=0, n=0;
    for(std::size_t i=0;i<d.steps.size();++i){
        const DesignStep& s=d.steps[i];
        if(s.kind==2){ if(depth>0) --depth; continue; }
        std::string ind(5+depth*3,' ');
        if(s.kind==1){ std::cout<<ind<<"📂 "<<BOLD<<s.name<<RESET<<DIM<<" (these can happen at the same time)"<<RESET<<"\n"; ++depth; continue; }
        std::cout<<ind<<++n<<". "<<s.name<<DIM<<"  ["<<d.roleNames[s.role]<<"]"<<RESET;
        if(s.priority>0) std::cout<<YELLOW<<"  ⭐ priority "<<s.priority<<RESET;
        if(!s.field.empty()) std::cout<<CYAN<<"  🔍 needs \""<<s.field<<"\""<<RESET;
        std::cout<<"\n";
    }
    if(openGroups(d)>0) std::cout<<"\n  "<<YELLOW<<"(a parallel group is open: add tasks, then choose \"End the group\")"<<RESET<<"\n";
}
std::vector<std::string> designProblems(const Design& d){
    std::vector<std::string> p;
    if(d.roleNames.empty()) p.push_back("Add at least one role.");
    if(taskCount(d)==0) p.push_back("Add at least one task.");
    for(std::size_t r=0;r<d.roleNames.size();++r){
        bool used=false, staffed=false;
        for(std::size_t i=0;i<d.steps.size();++i) if(d.steps[i].kind==0 && d.steps[i].role==(int)r) used=true;
        for(std::size_t i=0;i<d.personRoles.size();++i) if(d.personRoles[i]==(int)r) staffed=true;
        if(used && !staffed) p.push_back("Nobody has the role \""+d.roleNames[r]+"\", but a task needs it. Add a person.");
    }
    return p;
}
Scenario* buildCustom(const Design& src,AssignmentStrategy* strategy,std::string& err){
    Design f=src;
    while(!f.steps.empty() && f.steps.back().kind==1) f.steps.pop_back();   // drop empty open group
    while(openGroups(f)>0) addMarker(f,2,"");
    std::unique_ptr<Scenario> s(new Scenario);
    s->name=f.name; s->emoji="✨"; s->subtitle="Custom workflow  |  "+num(taskCount(f))+" tasks";
    s->blurb="Your own workflow with "+num(taskCount(f))+" tasks.";
    std::vector<Role*> rs;
    for(std::size_t i=0;i<f.roleNames.size();++i) rs.push_back(s->role(f.roleNames[i],f.roleLevels[i]));
    for(std::size_t i=0;i<f.personNames.size();++i){
        std::string id="C"+std::string(i+1<10?"0":"")+num(i+1);
        s->person(id,f.personNames[i],rs[f.personRoles[i]]);
    }
    try{
        DefinitionBuilder b; Director dir(b);
        s->definition=dir.construct(s->name,1,[&](WorkflowBuilder& w){
            w.beginSequentialStage(f.name);
            for(std::size_t i=0;i<f.steps.size();++i){
                const DesignStep& st=f.steps[i];
                if(st.kind==1) w.beginParallelStage(st.name);
                else if(st.kind==2) w.endStage();
                else{
                    w.addTask(st.name,rs[st.role],strategy);
                    if(st.priority>0) w.withPriority(st.priority);
                    if(!st.field.empty()) w.withValidation(std::vector<std::string>(1,st.field));
                    w.commitPendingTask();
                }
            }
            w.endStage();
        });
        s->definition->freeze();
    }catch(const std::exception& e){ err=e.what(); return 0; }
    return s.release();
}

Scenario* designWorkflow(AssignmentStrategy* strategy){
    Design d; d.name="My workflow";
    header(0);
    std::cout<<"  "<<BOLD<<"✨ Create your own workflow"<<RESET<<"\n";
    tivi("Let's build one together! You choose the roles, people and tasks.");
    std::cout<<"\n  1)  🛠️  Start from scratch\n  2)  🎂 Use a ready-made example (Birthday party)\n  0)  ↩  Back\n";
    pattern("Builder + Director","you'll collect the recipe here; at the end a Director tells the WorkflowBuilder to assemble it piece by piece.");
    int start=choose("\n  Your choice 👉 ",0,2); if(!start) return 0;
    if(start==2) loadStarter(d);
    else{ std::string nm=ask("\n  Name your workflow (ENTER = \"My workflow\"): "); if(!nm.empty()) d.name=nm; }

    for(;;){
        header(0);
        renderDesign(d);
        std::cout<<"\n  "<<BOLD<<"What next?"<<RESET<<"\n\n"
                 <<"  1)  👥 Add a role\n  2)  🧑 Add a person\n  3)  🧩 Add a task\n  4)  📂 Start a parallel group\n"
                 <<"  5)  🔚 End the group\n  6)  ↩️  Remove the last step\n  7)  🚀 Finish and build it!\n  0)  ❌ Cancel\n";
        tip("Add roles first, then people, then tasks. A task needs a role, and a role needs a person.");
        int c=choose("\n  Your choice 👉 ",0,7);
        if(c==0) return 0;
        if(c==1){
            std::string n=ask("\n  Role name (e.g. Chef): ");
            if(n.empty()){ std::cout<<"  "<<RED<<"A role needs a name."<<RESET<<"\n"; pause("Press ENTER"); continue; }
            std::cout<<"  Seniority: 1) junior  2) experienced  3) senior\n";
            int lvl=choose("  Level 👉 ",1,3); addRole(d,n,lvl);
        }else if(c==2){
            if(d.roleNames.empty()){ std::cout<<"\n  "<<YELLOW<<"Add a role first. Every person needs one."<<RESET<<"\n"; pause("Press ENTER"); continue; }
            std::string n=ask("\n  Person's name: ");
            if(n.empty()){ std::cout<<"  "<<RED<<"A person needs a name."<<RESET<<"\n"; pause("Press ENTER"); continue; }
            std::cout<<"\n"; for(std::size_t i=0;i<d.roleNames.size();++i) std::cout<<"  "<<i+1<<")  "<<d.roleNames[i]<<"\n";
            int r=choose("\n  Their role 👉 ",1,(int)d.roleNames.size()); addPerson(d,n,r-1);
        }else if(c==3){
            if(d.roleNames.empty()){ std::cout<<"\n  "<<YELLOW<<"Add a role first. Every task needs one."<<RESET<<"\n"; pause("Press ENTER"); continue; }
            std::string n=ask("\n  Task name (e.g. Prepare the sauce): ");
            if(n.empty()){ std::cout<<"  "<<RED<<"A task needs a name."<<RESET<<"\n"; pause("Press ENTER"); continue; }
            std::cout<<"\n"; for(std::size_t i=0;i<d.roleNames.size();++i) std::cout<<"  "<<i+1<<")  "<<d.roleNames[i]<<"\n";
            int r=choose("\n  Who does it (role) 👉 ",1,(int)d.roleNames.size());
            std::cout<<"\n  Priority: 0) normal  1) low  2) medium  3) high\n";
            int pr=choose("  Priority 👉 ",0,3);
            std::string f=ask("  Must a form field be filled in? Type its name, or ENTER to skip: ");
            addTaskStep(d,n,r-1,pr,f);
            if(pr>0||!f.empty()) pattern("Decorator","priority and validation are added as wrappers around the plain task.");
            if(pr>0||!f.empty()) pause("Press ENTER to continue");
        }else if(c==4){
            std::string n=ask("\n  Group name (ENTER = \"Parallel group\"): "); if(n.empty()) n="Parallel group";
            addMarker(d,1,n);
        }else if(c==5){
            if(openGroups(d)<=0){ std::cout<<"\n  "<<YELLOW<<"There's no open group to end."<<RESET<<"\n"; pause("Press ENTER"); }
            else if(d.steps.back().kind==1){ std::cout<<"\n  "<<YELLOW<<"This group is empty. Add a task first."<<RESET<<"\n"; pause("Press ENTER"); }
            else addMarker(d,2,"");
        }else if(c==6){
            if(d.steps.empty()){ std::cout<<"\n  "<<YELLOW<<"Nothing to remove."<<RESET<<"\n"; pause("Press ENTER"); }
            else d.steps.pop_back();
        }else if(c==7){
            std::vector<std::string> probs=designProblems(d);
            if(!probs.empty()){
                std::cout<<"\n  "<<YELLOW<<"Almost there! Please fix:"<<RESET<<"\n";
                for(std::size_t i=0;i<probs.size();++i) std::cout<<"   • "<<probs[i]<<"\n";
                pause("Press ENTER to keep editing"); continue;
            }
            std::string err; Scenario* s=buildCustom(d,strategy,err);
            if(!s){
                std::cout<<"\n  "<<RED<<BOLD<<"🛡️  The engine couldn't build that workflow."<<RESET<<"\n  "<<RED<<"Reason: "<<err<<RESET<<"\n";
                pause("Press ENTER to keep editing"); continue;
            }
            header(0);
            tivi("Your workflow is built and frozen. Let's play it!",GREEN);
            pattern("Builder + Director","the Director replayed your recipe through WorkflowBuilder, then froze the definition.");
            pattern("Factory","each time you play it, createInstance() makes a fresh run from the frozen recipe.");
            pause("Press ENTER to start");
            return s;
        }
    }
}

// ------------------------------------------------------------------ story picker
void pickScenario(std::unique_ptr<Running>& running,std::vector<std::unique_ptr<Scenario> >& sc,AssignmentStrategy* strategy){
    header(0);
    tivi("Hi! I'm Tivi. Let's run a workflow together!");
    std::cout<<"\n  "<<BOLD<<"Pick a story to play through:"<<RESET<<"\n\n";
    for(std::size_t i=0;i<sc.size();++i)
        std::cout<<"  "<<i+1<<")  "<<sc[i]->emoji<<"  "<<BOLD<<sc[i]->name<<RESET<<"\n      "<<DIM<<sc[i]->blurb<<RESET<<"\n\n";
    int n=(int)sc.size();
    std::cout<<"  "<<n+1<<")  ✨  "<<BOLD<<"Create my own workflow"<<RESET<<"\n\n"
             <<"  "<<n+2<<")  📖  "<<BOLD<<"How does this work?"<<RESET<<"\n\n"
             <<"  0)  👋  Exit\n";
    tip("Not sure? Choose 1. It's a great first story.");
    int c=choose("\n  Your choice 👉 ",0,n+2);
    if(c==0){ running.reset(); throw 0; }
    if(c==n+2){ showHelp(); return; }
    if(c==n+1){
        Scenario* s=designWorkflow(strategy);
        if(!s) return;
        sc.push_back(std::unique_ptr<Scenario>(s));
        c=(int)sc.size();
    }
    std::size_t idx=(std::size_t)(c-1);
    running.reset(new Running); running->scenario=sc[idx].get();
    if(idx==0) running->idBase="HOSP-001";
    else if(idx==1) running->idBase="LEGAL-001";
    else{ std::ostringstream o; o<<"CUSTOM-"<<std::setw(3)<<std::setfill('0')<<(idx-1); running->idBase=o.str(); }
    running->instance=sc[idx]->definition->createInstance(running->idBase);
    running->instance->start();
    running->history.push_back("Started a new workflow instance");
}

} // namespace

int main(){
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    HANDLE hOut=GetStdHandle(STD_OUTPUT_HANDLE); DWORD mode=0;
    if(hOut!=INVALID_HANDLE_VALUE && GetConsoleMode(hOut,&mode)) SetConsoleMode(hOut,mode|0x0004);
#endif
    RoleBasedAssignment strategy;
    std::vector<std::unique_ptr<Scenario> > scenarios;
    scenarios.push_back(std::unique_ptr<Scenario>(makeHospital(&strategy)));
    scenarios.push_back(std::unique_ptr<Scenario>(makeLegal(&strategy)));
    std::unique_ptr<Running> running;

    try{
        for(;;){
            if(!running){ pickScenario(running,scenarios,&strategy); if(running){ header(running.get()); pattern("Factory","createInstance() just made a fresh run of this workflow from its frozen definition."); pause("Press ENTER to continue"); } continue; }

            header(running.get());
            int total; int done=countDone(*running,total);
            progressBar(done,total);
            std::cout<<"\n  "<<BOLD<<"What would you like to do?"<<RESET<<"\n\n";
            std::cout<<"  "<<CYAN<<"DO THINGS"<<RESET<<"\n";
            std::cout<<"   1)  ▶️  Do a task                    "<<DIM<<"(guided, any pick allowed)"<<RESET<<"\n";
            std::cout<<"   2)  🔴 Send a task back for fixing\n";
            std::cout<<"   3)  🧪 Error lab: try to break it!\n";
            std::cout<<"   4)  🎬 Watch it run by itself        "<<DIM<<"(demo mode)"<<RESET<<"\n";
            std::cout<<"  "<<CYAN<<"LOOK AROUND"<<RESET<<"\n";
            std::cout<<"   5)  🗺️  See the big picture\n";
            std::cout<<"   6)  📜 What happened so far\n";
            std::cout<<"  "<<CYAN<<"OOPS?"<<RESET<<"\n";
            std::cout<<"   7)  🕰️  Time machine                 "<<DIM<<"(undo / redo / checkpoints)"<<RESET<<"\n";
            std::cout<<"  "<<CYAN<<"LEARN"<<RESET<<"\n";
            std::cout<<"   8)  🧠 Pattern guide\n";
            std::cout<<"   9)  📖 How does this work?\n";
            std::cout<<"  10)  🎓 Pattern notes: "<<(g_notes?std::string(GREEN)+"ON"+RESET:std::string(RED)+"OFF"+RESET)<<DIM<<"  (press to toggle)"<<RESET<<"\n";
            std::cout<<"  "<<CYAN<<"OTHER"<<RESET<<"\n";
            std::cout<<"  11)  🔄 Pick a story / create a workflow\n";
            std::cout<<"   0)  👋 Exit\n";
            if(running->instance->isFinished()) tip("Finished! Try the Time machine to undo steps, or option 11 for another story.");
            else tip("New here? Start with option 1. Made a mistake? Option 7 undoes it.");
            int c=choose("\n  Your choice 👉 ",0,11);
            if(c==0) break;
            else if(c==1) doNextTask(*running);
            else if(c==2) rejectTask(*running);
            else if(c==3) errorLab(*running);
            else if(c==4) autoRun(*running);
            else if(c==5) showMap(*running);
            else if(c==6) showHistory(*running);
            else if(c==7) timeMachine(*running);
            else if(c==8) showPatternGuide();
            else if(c==9) showHelp();
            else if(c==10) g_notes=!g_notes;
            else if(c==11) running.reset();
        }
    }catch(int){ }

    std::cout<<"\n  "<<CYAN<<"(\\_/)  Thanks for using TIVIDY! See you soon 🌸"<<RESET<<"\n\n";
    return 0;
}