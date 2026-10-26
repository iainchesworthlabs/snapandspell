// SPDX-License-Identifier: GPL-3.0-or-later
#include <catch2/catch_test_macros.hpp>
#include <string>

#include "sns/version.hpp"

TEST_CASE("the version block names the version and the build's provenance", "[version]") {
    CHECK_FALSE(sns::project_version.empty());
    CHECK_FALSE(sns::target_platform.empty());
    CHECK_FALSE(sns::build_target.empty());
    CHECK_FALSE(sns::git_commit.empty());
    CHECK_FALSE(sns::git_branch.empty());

    const std::string details = sns::version_details();
    CHECK(details.find(std::string{sns::project_version}) != std::string::npos);
    CHECK(details.find("release:") != std::string::npos);
    CHECK(details.find("commit:") != std::string::npos);
    CHECK(details.find(std::string{sns::git_commit}) != std::string::npos);
    CHECK(details.find("branch:") != std::string::npos);
    CHECK(details.find("target:") != std::string::npos);
    CHECK(details.find(std::string{sns::build_target}) != std::string::npos);
    // A dirty tree says so, honestly; a clean one says nothing about dirt.
    if (sns::git_dirty) {
        CHECK(details.find("dirty") != std::string::npos);
    } else {
        CHECK(details.find("dirty") == std::string::npos);
    }
}
