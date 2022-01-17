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
#    Update Genesis
####################################################################################################

# Here, we update genesis to its latest commit on master.
# This includes the submodule and the CMake tag.

# Change to main project dir. This ensures that the script can be called from any directory.
cd "$(dirname "$0")/../.."

# Get latest commit on master
cd libs/genesis/
git checkout master
git pull --all

# Get the commit hash
HASH=`git log | head -n 1 | sed "s/commit //g"`
git checkout $HASH

# Update the dependency
cd ../../
./tools/cmake/update_dependencies.sh
