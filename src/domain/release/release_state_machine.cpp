#include "domain/release/release_state_machine.h"

namespace shipdesk::domain {

bool ReleaseStateMachine::canTransition(ReleaseState current, ReleaseState target) {
    if (current == target) {
        return true;
    }

    switch (current) {
        case ReleaseState::Draft:
            return target == ReleaseState::Inspecting ||
                   target == ReleaseState::Archived ||
                   target == ReleaseState::Cancelled;

        case ReleaseState::Inspecting:
            return target == ReleaseState::Inspected ||
                   target == ReleaseState::Cancelled ||
                   target == ReleaseState::Interrupted ||
                   target == ReleaseState::NeedsReview;

        case ReleaseState::Inspected:
            return target == ReleaseState::Validating ||
                   target == ReleaseState::Inspecting ||
                   target == ReleaseState::Draft ||
                   target == ReleaseState::Cancelled ||
                   target == ReleaseState::Interrupted;

        case ReleaseState::Validating:
            return target == ReleaseState::Validated ||
                   target == ReleaseState::Cancelled ||
                   target == ReleaseState::Interrupted ||
                   target == ReleaseState::NeedsReview;

        case ReleaseState::Validated:
            return target == ReleaseState::Staging ||
                   target == ReleaseState::BuildPending ||
                   target == ReleaseState::Validating ||
                   target == ReleaseState::Inspecting ||
                   target == ReleaseState::Draft ||
                   target == ReleaseState::Cancelled ||
                   target == ReleaseState::Interrupted;

        case ReleaseState::BuildPending:
            return target == ReleaseState::Building ||
                   target == ReleaseState::Draft ||
                   target == ReleaseState::Cancelled;

        case ReleaseState::Building:
            return target == ReleaseState::BuildSucceeded ||
                   target == ReleaseState::BuildFailed ||
                   target == ReleaseState::Cancelled ||
                   target == ReleaseState::Interrupted ||
                   target == ReleaseState::NeedsReview;

        case ReleaseState::BuildFailed:
            return target == ReleaseState::BuildPending ||
                   target == ReleaseState::Draft ||
                   target == ReleaseState::Cancelled;

        case ReleaseState::BuildSucceeded:
            return target == ReleaseState::Staging ||
                   target == ReleaseState::BuildPending ||
                   target == ReleaseState::Cancelled;

        case ReleaseState::Staging:
            return target == ReleaseState::Staged ||
                   target == ReleaseState::Cancelled ||
                   target == ReleaseState::Interrupted ||
                   target == ReleaseState::NeedsReview;

        case ReleaseState::Staged:
            return target == ReleaseState::Packaging ||
                   target == ReleaseState::Staging ||
                   target == ReleaseState::Draft ||
                   target == ReleaseState::Cancelled ||
                   target == ReleaseState::Interrupted;

        case ReleaseState::Packaging:
            return target == ReleaseState::Packaged ||
                   target == ReleaseState::Cancelled ||
                   target == ReleaseState::Interrupted ||
                   target == ReleaseState::NeedsReview;

        case ReleaseState::Packaged:
            return target == ReleaseState::Verifying ||
                   target == ReleaseState::Draft ||
                   target == ReleaseState::Cancelled ||
                   target == ReleaseState::Interrupted;

        case ReleaseState::Verifying:
            return target == ReleaseState::Verified ||
                   target == ReleaseState::Cancelled ||
                   target == ReleaseState::Interrupted ||
                   target == ReleaseState::NeedsReview;

        case ReleaseState::Verified:
            return target == ReleaseState::ReadyToPublish ||
                   target == ReleaseState::Draft ||
                   target == ReleaseState::Archived;

        case ReleaseState::ReadyToPublish:
            return target == ReleaseState::Publishing ||
                   target == ReleaseState::Draft ||
                   target == ReleaseState::Archived;

        case ReleaseState::Publishing:
            return target == ReleaseState::Published ||
                   target == ReleaseState::PublishFailed ||
                   target == ReleaseState::Cancelled ||
                   target == ReleaseState::Interrupted;

        case ReleaseState::PublishFailed:
            return target == ReleaseState::Publishing ||
                   target == ReleaseState::ReadyToPublish ||
                   target == ReleaseState::NeedsReview ||
                   target == ReleaseState::Cancelled;

        case ReleaseState::Published:
            // Published releases are immutable; can only be archived
            return target == ReleaseState::Archived;

        case ReleaseState::Cancelled:
            return target == ReleaseState::Draft ||
                   target == ReleaseState::Archived;

        case ReleaseState::Archived:
            return target == ReleaseState::Draft;

        case ReleaseState::Interrupted:
            return target == ReleaseState::NeedsReview ||
                   target == ReleaseState::Draft ||
                   target == ReleaseState::Cancelled;

        case ReleaseState::NeedsReview:
            return target == ReleaseState::Draft ||
                   target == ReleaseState::Inspecting ||
                   target == ReleaseState::Staging ||
                   target == ReleaseState::Cancelled;
    }

    return false;
}

void ReleaseStateMachine::validateTransition(ReleaseState current, ReleaseState target) {
    if (!canTransition(current, target)) {
        throw ReleaseTransitionException(current, target, "Transition not permitted by state machine contract");
    }
}

bool ReleaseStateMachine::isTransientState(ReleaseState state) {
    return state == ReleaseState::Inspecting ||
           state == ReleaseState::Validating ||
           state == ReleaseState::Building ||
           state == ReleaseState::Staging ||
           state == ReleaseState::Packaging ||
           state == ReleaseState::Verifying ||
           state == ReleaseState::Publishing;
}

bool ReleaseStateMachine::isTerminalState(ReleaseState state) {
    return state == ReleaseState::Published ||
           state == ReleaseState::Archived;
}

ReleaseState ReleaseStateMachine::recoverTransientState(ReleaseState state) {
    if (isTransientState(state)) {
        return ReleaseState::Interrupted;
    }
    return state;
}

} // namespace shipdesk::domain
