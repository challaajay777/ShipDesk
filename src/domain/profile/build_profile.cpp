#include "domain/profile/build_profile.h"
#include <sstream>

namespace shipdesk::domain {

BuildProfile::BuildProfile(std::string id, std::string workspace_id, std::string name)
    : m_id(std::move(id))
    , m_workspaceId(std::move(workspace_id))
    , m_name(std::move(name))
{
}

std::string BuildProfile::computeApprovalFingerprint(const std::string& sourceRootIdentity) const {
    std::ostringstream ss;
    ss << "SRC:" << sourceRootIdentity << "|";
    for (const auto& step : m_steps) {
        if (!step.enabled) continue;
        ss << "EXE:" << step.executable << "|";
        ss << "WD:" << step.working_directory << "|";
        ss << "ARGS:";
        for (const auto& arg : step.arguments) {
            ss << arg << ",";
        }
        ss << "|ENV:";
        for (const auto& [k, v] : step.environment_overrides) {
            ss << k << "=" << v << ",";
        }
        ss << "|";
    }
    return ss.str();
}

} // namespace shipdesk::domain
