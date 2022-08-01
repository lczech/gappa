#!/bin/bash

# gappa - Genesis Applications for Phylogenetic Placement Analysis
# Copyright (C) 2017-2022 Lucas Czech
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#
# Contact:
# Lucas Czech <lczech@carnegiescience.edu>
# Department of Plant Biology, Carnegie Institution For Science
# 260 Panama Street, Stanford, CA 94305, USA

####################################################################################################
#    Helper functions
####################################################################################################

function print_separator() {
    echo
    echo -en "\e[32m==========================================================================="
    echo     "========================="
    echo -e  "          ${1}"
    echo -en "================================================================================="
    echo -e  "===================\e[0m"
    echo
}

function get_user_confirmation() {
    if [[ ${1} != "" ]]; then
        text=$1
    else
        text="Do you want to continue?"
    fi

    response="x"
    while [[ ${response} != "y" ]]; do
        read -p "${text} [y/n] " -n 1 -r response
        echo

        if [[ $response == "y" ]]; then
            return 1
        fi
        if [[ $response == "n" ]]; then
            return 0
        fi
    done
}

####################################################################################################
#    Check preconditions
####################################################################################################

print_separator "Check preconditions"

# Change to top level of git repo.
# This ensures that the script can be called from any directory.
cd `git rev-parse --show-toplevel`

# Check repo mame.
# If applied to a different repo, this script might do weird stuff, so better check.
base_name=`git rev-parse --show-toplevel | xargs basename`
if [[ ${base_name} != gappa* ]]; then
    echo -e "\e[31mWrong repository. Expect gappa.\e[0m"
    exit
else
    echo "Repository: ${base_name}"
fi

# Get and check current branch.
branch=`git rev-parse --abbrev-ref HEAD`
if [[ ${branch} != "master" ]]; then
    echo -e "\e[31mNot on master branch. Aborting.\e[0m"
    exit
else
    echo "Branch: ${branch}"
fi

# Check changed files.
changed_files=`git diff --name-only HEAD`
if [[ ${changed_files} != "" ]]; then
    echo -e "\e[31mUncommitted files. Aborting.\e[0m"
    exit
else
    echo "No uncommitted files."
fi

# Check stash.
stashed_files=`git stash list`
if [[ ${stashed_files} != "" ]]; then
    # echo -e "\e[31mStashed files. Aborting.\e[0m"
    # exit

    echo "There are staged files:"
    echo ${stashed_files}
    echo

    get_user_confirmation
    cont=$?
    if [[ $cont == 0 ]]; then
        echo -e "\e[31mAborted.\e[0m"
        exit
    fi
else
    echo "No stashed files."
fi

# Check for unmerged branches.
unmerged=`git branch --no-merged`
if [[ ${unmerged} != "" ]]; then
    echo "There are unmerged branches:"
    echo ${unmerged}
    echo

    get_user_confirmation
    cont=$?
    if [[ $cont == 0 ]]; then
        echo -e "\e[31mAborted.\e[0m"
        exit
    fi
else
    echo "No unmerged branches."
fi

# Check header guards
header_guards=`./tools/deploy/check_header_guards.sh`
if [[ ${header_guards} != "" ]]; then
    echo -e "\n\e[31mHeader guards inconsistent:\e[0m"
    ./tools/deploy/check_header_guards.sh
    echo -e "\n\e[31mAborted.\e[0m"
    exit
else
    echo "Header guards okay."
fi

# Check submodule commit hashes
function get_commit_hash() {
    local libname=${1}

    # Disect the git submodule output to get the commit that is currently checked out.
    # This output is of the form ".<hash> libs/genesis (v0.19.0-14-g5b77dba)",
    # where the first dot is a space, plus sign or some other git status symbol.
    # This also sets the "return" to the outside scope... bash...
    commit_hash=`git submodule status | grep ${libname} | sed "s/.\([0-9a-f]*\) .*/\1/g" | tr -d "\n"`

    # Use two methods for checking.
    # This fails if the submodule is checked out, but not commited.
    # In that case, we only print a warning.
    local hash_a=`git ls-files -s libs/${libname} | cut -d" " -f 2`
    local hash_b=`git ls-tree master libs/${libname} | awk -F "[ \t]" '{print $3}'`

    if [[ ${hash_a} != ${hash_b} ]]; then
        echo -e "\e[31mProblem with commit hash for ${libname}: ${hash_a} != ${hash_b}\e[0m"
    fi
    if [[ ${hash_a} != ${commit_hash} ]]; then
        echo -e "\e[31mNot yet commited submodule ${libname}: ${commit_hash} > ${hash_a}\e[0m"
    fi
}

get_commit_hash "CLI11"
get_commit_hash "genesis"

####################################################################################################
#    Build all
####################################################################################################

print_separator "Build all"

unset GENESIS_DEBUG
make clean
make -j 4

####################################################################################################
#    Version
####################################################################################################

print_separator "Version"

# Output current version.
last_tag=`git describe --abbrev=0 --tags`
echo -en "Latest version tag:    \e[34m${last_tag}\e[0m\n\n"

# Read version tag.
read -p "Enter new version tag: v" version
version="v${version}"
# echo ${version}
echo

# Replace version line in gappa header file.
echo "Replace version in src/tools/version.hpp"
sed -i "s/    return \".*\"; \/\/ #GAPPA_VERSION#/    return \"${version}\"; \/\/ #GAPPA_VERSION#/g" src/tools/version.hpp

# Make sure that the dependencies are updated.
./tools/cmake/update_dependencies.sh

####################################################################################################
#    Build with version
####################################################################################################

print_separator "Build with version"

# Build again, this time with the updated version tag.
make update -j 4

####################################################################################################
#    Commit and Tag
####################################################################################################

print_separator "Commit and Tag"

# Confirm changes.
echo -e "\e[34mCurrent git status:\e[0m\n"
git status
echo -e "\n\e[34mAbout to commit changes and to create version tag ${version}\e[0m\n"

get_user_confirmation
cont=$?
if [[ $cont == 0 ]]; then
    echo -e "\e[31mAborted.\e[0m"
    exit
fi
echo

# Commit and Tag.
echo "Make commit, create tag..."
git commit --allow-empty -am "Release ${version}"
git tag -a "${version}" -m "Release ${version}"
echo -e "...done."

####################################################################################################
#    Publish to GitHub
####################################################################################################

print_separator "Publish to GitHub"

# Show changes.
echo -e "\e[34mCurrent git status:\e[0m\n"
git status
echo

# Confirm.
get_user_confirmation "Do you want to push the commit and tag to GitHub?"
cont=$?

# Push.
if [[ $cont == 1 ]]; then
    echo
    git push --all --follow-tags
fi

####################################################################################################
#    Prepare GitHub Release
####################################################################################################

print_separator "Prepare GitHub Release"

# Get current (the new) tag.
new_tag=`git describe --abbrev=0 --tags`

if [[ ${new_tag} != ${version} ]]; then
    echo "Weird, version ${version} and tag ${new_tag} differ..."
    echo
fi

# Get all important commits after the last tag and format them for Markdown.
echo -e "\e[34m### Notable Changes ###\e[0m\n"
git log ${last_tag}..${new_tag} --oneline | cut -d " " -f 1 --complement | egrep -iv "^(Minor|Merge|Release)" | sed "s/^/  \* /g"
echo -e "\nUse this list for the release message.\n"

# Ask user for github page.
get_user_confirmation "Do you want to open the GitHub release page?"
cont=$?

# Open github release page.
if [[ $cont == 1 ]]; then
    github_release_url="https://github.com/lczech/gappa/releases/new"
    if which xdg-open > /dev/null
    then
        xdg-open ${github_release_url}
    elif which gnome-open > /dev/null
    then
        gnome-open ${github_release_url}
    else
        echo "Cannot open page."
    fi
    sleep 1
fi

####################################################################################################
#    Finished
####################################################################################################

print_separator "Finished"
