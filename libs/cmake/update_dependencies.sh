#!/bin/bash

# gappa - Genesis Applications for Phylogenetic Placement Analysis
# Copyright (C) 2017-2018 Lucas Czech and HITS gGmbH
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
# Lucas Czech <lucas.czech@h-its.org>
# Exelixis Lab, Heidelberg Institute for Theoretical Studies
# Schloss-Wolfsbrunnenweg 35, D-69118 Heidelberg, Germany

####################################################################################################
#    Update Git Submodule Commit Hashes
####################################################################################################

# Here, we update the commit hashes of the download scripts for our dependencies.
# The script uses git to get the hases that the submodules currently point to,
# and replaces them in the cmake download scripts.

# Change to main project dir. This ensures that the script can be called from any directory.
cd "$(dirname "$0")/../.."

function get_commit_hash() {
    local libname=${1}

    # Use two methods, to be sure.
    local hash_a=`git ls-files -s libs/${libname} | cut -d" " -f 2`
    local hash_b=`git ls-tree master libs/${libname} | awk -F "[ \t]" '{print $3}'`

    if [[ ${hash_a} != ${hash_b} ]]; then
        echo -e "\e[31mProblem with commit hash for ${libname}: ${hash_a} != ${hash_b}\e[0m"
        exit
    fi

    # Also, check against submodule
    local submod=`git submodule status | grep ${hash_a} | wc -l`
    if [[ ${submod} != 1 ]]; then
        echo -e "\e[31mProblem with commit hash for ${libname}: count ${submod}\e[0m"
        exit
    fi

    # "return" a string... bash...
    commit_hash=${hash_a}
}

function update_commit_hash() {
    local libname=${1}

    get_commit_hash ${libname}
    echo "${libname} @ ${commit_hash}"

    LINE="SET( ${libname}_COMMIT_HASH \"${commit_hash}\" ) #${libname}_COMMIT_HASH#"
    sed -i "s/.*#${libname}_COMMIT_HASH#/${LINE}/g" CMakeLists.txt
}

####################################################################################################
#    Submodule Dependencies
####################################################################################################

update_commit_hash "CLI11"
update_commit_hash "genesis"
update_commit_hash "sparsepp"
