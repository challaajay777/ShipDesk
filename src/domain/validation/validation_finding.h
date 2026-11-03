#pragma once

#include "domain/types.h"
#include <string>

namespace shipdesk::domain {

struct ValidationFinding {
    std::string rule_id;
    std::string rule_version{"1.0"};
    FindingSeverity severity{FindingSeverity::Info};
    std::string title;
    std::string explanation;
    std::string evidence;
    std::string suggested_action;
    bool blocks_packaging{false};
    bool blocks_publishing{false};
    std::string timestamp_utc{currentUtcTimestamp()};

    bool isBlocker() const {
        return severity == FindingSeverity::Blocker || blocks_packaging || blocks_publishing;
    }
};

} // namespace shipdesk::domain
