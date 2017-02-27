// Licensed to the Apache Software Foundation (ASF) under one
// or more contributor license agreements.  See the NOTICE file
// distributed with this work for additional information
// regarding copyright ownership.  The ASF licenses this file
// to you under the Apache License, Version 2.0 (the
// "License"); you may not use this file except in compliance
// with the License.  You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef __LXD_CONTAINERIZER_HPP__
#define __LXD_CONTAINERIZER_HPP__

#include <list>
#include <map>
#include <set>
#include <string>

#include <mesos/slave/container_logger.hpp>

#include <process/future.hpp>
#include <process/owned.hpp>
#include <process/subprocess.hpp>
#include <process/shared.hpp>

#include <stout/flags.hpp>
#include <stout/hashset.hpp>
#include <stout/duration.hpp>
#include <stout/json.hpp>
#include <stout/none.hpp>
#include <stout/nothing.hpp>
#include <stout/option.hpp>
#include <stout/path.hpp>
#include <stout/version.hpp>
#include <stout/os/rm.hpp>

#include "slave/containerizer/containerizer.hpp"

#include "slave/containerizer/mesos/isolators/gpu/components.hpp"

#include "mesos/resources.hpp"


namespace mesos {
namespace internal {
namespace slave {

// Prefix used to name LXD containers in order to distinguish those
// created by Mesos from those created manually.
extern const std::string LXD_NAME_PREFIX;

// Separator used to compose LXD container name, which is made up
// of slave ID and container ID.
extern const std::string LXD_NAME_SEPERATOR;

// Directory that stores all the symlinked sandboxes that is mapped
// into LXD containers. This is a relative directory that will
// joined with the slave path. Only sandbox paths that contains a
// colon will be symlinked due to the limitation of the Docker CLI.
extern const std::string LXD_SYMLINK_DIRECTORY;

// Forward declaration.
class Lxd;


class LxdContainerizer : public Containerizer
{
public:
  static Try<LxdContainerizer*> create(
      const Flags& flags,
      Fetcher* fetcher,
      const Option<NvidiaComponents>& nvidia = None());

  LxdContainerizer(
      const Flags& flags,
      Fetcher* fetcher,
      const process::Owned<mesos::slave::ContainerLogger>& logger,
      const Option<NvidiaComponents>& nvidia);

  virtual ~LxdContainerizer();

  virtual process::Future<Nothing> recover(
      const Option<state::SlaveState>& state);

  virtual process::Future<bool> launch(
      const ContainerID& containerId,
      const Option<TaskInfo>& taskInfo,
      const ExecutorInfo& executorInfo,
      const std::string& directory,
      const Option<std::string>& user,
      const SlaveID& slaveId,
      const std::map<std::string, std::string>& environment,
      bool checkpoint);

  virtual process::Future<Nothing> update(
      const ContainerID& containerId,
      const Resources& resources);

  virtual process::Future<ResourceStatistics> usage(
      const ContainerID& containerId);

  virtual process::Future<ContainerStatus> status(
      const ContainerID& containerId);

  virtual process::Future<Option<mesos::slave::ContainerTermination>> wait(
      const ContainerID& containerId);

  virtual process::Future<bool> destroy(const ContainerID& containerId);

  virtual process::Future<hashset<ContainerID>> containers();

private:
  process::Owned<Lxd> process;
};


class Lxd: public process::Process<Lxd>
{
public:
  // Create Docker abstraction and optionally validate docker.
  static Try<process::Owned<Lxd>> create(
      bool validate = true,
      const Option<JSON::Object>& config = None());


  // Create Docker abstraction and optionally validate docker.
  // Performs 'docker run IMAGE'. Returns the exit status of the
  // container. Note that currently the exit status may correspond
  // to the exit code from a failure of the docker client or daemon
  // rather than the container. Docker >= 1.10 [1] uses the following
  // exit statuses inherited from 'chroot':
  //     125 if the error is with Docker daemon itself.
  //     126 if the contained command cannot be invoked.
  //     127 if the contained command cannot be found.
  //     Exit code of contained command otherwise.
  //
  // [1]: https://github.com/docker/docker/pull/14012
  virtual process::Future<bool> launch(
    const mesos::ContainerID& containerId,
    const Option<mesos::TaskInfo>& taskInfo,
    const mesos::ExecutorInfo& executorInfo,
    const std::string& directory,
    const Option<std::string>& user,
    const mesos::SlaveID& slaveId,
    const std::map<std::string, std::string>& environment,
    bool checkpoint);

  virtual ~Lxd() {};
};


} // namespace slave {
} // namespace internal {
} // namespace mesos {

#endif // __LXD_CONTAINERIZER_HPP__
