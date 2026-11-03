#pragma once

#include "domain/validation/validation_finding.h"
#include "domain/source/inventory_item.h"
#include "domain/profile/release_profile.h"
#include "domain/release/release_draft.h"
#include <vector>

namespace shipdesk::domain {

class ReleaseValidator {
public:
    static std::vector<ValidationFinding> validateInventory(
        const std::vector<SourceInventoryItem>& inventory,
        const ReleaseProfile& profile,
        const ReleaseDraft& draft
    );
};

} // namespace shipdesk::domain
