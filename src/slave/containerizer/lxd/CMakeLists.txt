# Licensed to the Apache Software Foundation (ASF) under one
# or more contributor license agreements.  See the NOTICE file
# distributed with this work for additional information
# regarding copyright ownership.  The ASF licenses this file
# to you under the Apache License, Version 2.0 (the
# "License"); you may not use this file except in compliance
# with the License.  You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

# SOURCE FILES FOR THE CONTAINERIZER.
#####################################
set(LXD_CONTAINERIZER_EXECUTABLE_SRC
  ${LXD_CONTAINERIZER_EXECUTABLE_SRC}
  main.cpp
  )

# THE CONTAINERIZER EXECUTABLE.
###############################
add_executable(${LXD_CONTAINERIZER} ${LXD_CONTAINERIZER_EXECUTABLE_SRC})

# ADD LINKER FLAGS (generates, e.g., -lglog on Linux).
######################################################
target_link_libraries(${LXD_CONTAINERIZER}  ${AGENT_LIBS} ${MESOS_LIBS_TARGET})

# ADD BINARY DEPENDENCIES (tells CMake what to compile/build first).
####################################################################
add_dependencies(${MESOS_TARGET} ${LXD_CONTAINERIZER})
add_dependencies(${LXD_CONTAINERIZER} ${MESOS_LIBS_TARGET})
