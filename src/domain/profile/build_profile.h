#pragma once

#include "domain/types.h"
#include <string>
#include <vector>
#include <map>
#include <cstdint>

namespace shipdesk::domain {

struct BuildStep {
    std::string id;
    std::string build_profile_id;
    int step_order{0};
    std::string executable;
    std::vector<std::string> arguments;
    std::string working_directory;
    std::map<std::string, std::string> environment_overrides;
    int timeout_seconds{300};
    bool enabled{true};
};

class BuildProfile {
public:
    BuildProfile() = default;
    BuildProfile(std::string id, std::string workspace_id, std::string name);

    const std::string& id() const { return m_id; }
    const std::string& workspaceId() const { return m_workspaceId; }
    const std::string& name() const { return m_name; }
    const std::string& approvalFingerprint() const { return m_approvalFingerprint; }
    const std::string& approvalExpiresAtUtc() const { return m_approvalExpiresAtUtc; }

    const std::vector<BuildStep>& steps() const { return m_steps; }
    void addStep(const BuildStep& step) { m_steps.push_back(step); }
    void clearSteps() { m_steps.clear(); }

    void setApproval(const std::string& fingerprint, const std::string& expiresAtUtc) {
        m_approvalFingerprint = fingerprint;
        m_approvalExpiresAtUtc = expiresAtUtc;
    }

    bool isApprovedFor(const std::string& fingerprint) const {
        if (m_approvalFingerprint.empty()) return false;
        return m_approvalFingerprint == fingerprint;
    }

    void invalidateApproval() {
        m_approvalFingerprint.clear();
        m_approvalExpiresAtUtc.clear();
    }

    // Generates a canonical fingerprint string from the steps and environment
    std::string computeApprovalFingerprint(const std::string& sourceRootIdentity) const;

private:
    std::string m_id;
    std::string m_workspaceId;
    std::string m_name;
    std::vector<BuildStep> m_steps;
    std::string m_approvalFingerprint;
    std::string m_approvalExpiresAtUtc;
};

} // namespace shipdesk::domain
