// SPDX-License-Identifier: GPL-3.0-or-later
#include "sns/version.hpp"

#include <format>
#include <string>

namespace sns {

std::string version_details() {
    std::string details = std::format(
        "Snap and Spell {}\n"
        "  release: {}\n"
        "  commit:  {}\n"
        "  branch:  {}\n"
        "  target:  {}",
        project_version, git_describe, git_commit, git_branch, build_target);
    if (git_dirty) {
        details += "\n  state:   dirty (uncommitted changes)";
    }
    return details;
}

}  // namespace sns
