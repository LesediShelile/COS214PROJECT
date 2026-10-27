#ifndef PARTICIPANT_H
#define PARTICIPANT_H

#include <string>

class Role;

/**
 * @brief A person who can be assigned tasks
 *
 * Each participant has one role and keeps count of how many tasks
 * they are currently working on (their workload).
 */
class Participant {

    private:

        /** @brief Unique ID of the participant. */
        std::string participantID;

        /** @brief The participant's name. */
        std::string name;

        /** @brief The participant's role (not owned). */
        Role* role;

        /** @brief How many tasks the participant is currently working on. */
        int workload;

    public:

        /**
         * @brief Creates a participant with a workload of 0.
         * @param participantID Unique ID of the participant.
         * @param name The participant's name.
         * @param role The participant's role.
         */
        Participant(std::string participantID, std::string name, Role* role);

        /**
         * @brief Gets the participant's ID.
         * @return The participant ID.
         */
        std::string getParticipantID() const;

        /**
         * @brief Gets the participant's name.
         * @return The name.
         */
        std::string getName() const;

        /**
         * @brief Gets the participant's role.
         * @return The role.
         */
        Role* getRole() const;

        /**
         * @brief Gets how many tasks the participant is working on.
         * @return The workload.
         */
        int getWorkload() const;

        /** @brief Adds one task to the participant's workload. */
        void incrementWorkload();

        /** @brief Removes one task from the participant's workload. Never goes below 0. */
        void decrementWorkload();

        /**
         * @brief Checks whether the participant has a role.
         * @param role The role to check for.
         * @return True if the participant's role equals the given role.
         */
        bool hasRole(Role* role) const;
};

#endif