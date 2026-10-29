#ifndef PARTICIPANT_H
#define PARTICIPANT_H
#include <string>
/** STUB: replace with Cameron's real Participant. */
class Participant {
public:
    Participant(const std::string& id, const std::string& name) : id_(id), name_(name) {}
    std::string getParticipantID() const { return id_; }
    std::string getName() const { return name_; }
private:
    std::string id_, name_;
};
#endif
