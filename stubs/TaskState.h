#ifndef TASKSTATE_H
#define TASKSTATE_H
#include <string>
class Task;
class Participant;

/** STUB: replace with Cameron's real State hierarchy. Default behaviour for
 *  every operation is "invalid transition" (throws std::logic_error). */
class TaskState {
public:
    virtual ~TaskState() {}
    virtual void makeAvailable(Task* t);
    virtual void assign(Task* t, Participant* p);
    virtual void start(Task* t);
    virtual void complete(Task* t);
    virtual void reject(Task* t, const std::string& reason);
    virtual void cancel(Task* t);
    virtual std::string getName() const = 0;
    virtual TaskState* clone() const = 0;
    virtual bool isFinal() const { return false; }
    virtual bool isAvailable() const { return false; }
protected:
    void invalid(const char* op) const;
};

#define TIVIDY_STUB_STATE(NAME, EXTRA) \
class NAME : public TaskState { \
public: \
    virtual std::string getName() const { return #NAME; } \
    virtual TaskState* clone() const { return new NAME(*this); } \
    EXTRA \
};

TIVIDY_STUB_STATE(Created,   virtual void makeAvailable(Task*); virtual void cancel(Task*);)
TIVIDY_STUB_STATE(Available, virtual void assign(Task*, Participant*); virtual void cancel(Task*); virtual bool isAvailable() const { return true; })
TIVIDY_STUB_STATE(Assigned,  virtual void start(Task*); virtual void reject(Task*, const std::string&); virtual void cancel(Task*);)
TIVIDY_STUB_STATE(StartedState,   virtual void complete(Task*); virtual void reject(Task*, const std::string&); virtual void cancel(Task*);)
TIVIDY_STUB_STATE(RejectedState,  virtual void makeAvailable(Task*); virtual void cancel(Task*);)
TIVIDY_STUB_STATE(Completed, virtual bool isFinal() const { return true; })
TIVIDY_STUB_STATE(Cancelled, virtual bool isFinal() const { return true; })

#endif
