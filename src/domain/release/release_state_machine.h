#pragma once

#include "domain/types.h"
#include <string>
#include <vector>
#include <stdexcept>

namespace shipdesk::domain {

class ReleaseTransitionException : public std::runtime_error {
public:
    ReleaseTransitionException(ReleaseState from, ReleaseState to, const std::string& reason = "")
        : std::runtime_error("Invalid state transition from " + releaseStateToString(from) +
                             " to " + releaseStateToString(to) + (reason.empty() ? "" : ": " + reason))
        , m_from(from)
        , m_to(to)
    {}

    ReleaseState fromState() const { return m_from; }
    ReleaseState toState() const { return m_to; }

private:
    ReleaseState m_from;
    ReleaseState m_to;
};

class ReleaseStateMachine {
public:
    static bool canTransition(ReleaseState current, ReleaseState target);
    static void validateTransition(ReleaseState current, ReleaseState target);
    static bool isTransientState(ReleaseState state);
    static bool isTerminalState(ReleaseState state);
    static ReleaseState recoverTransientState(ReleaseState state);
};

} // namespace shipdesk::domain
