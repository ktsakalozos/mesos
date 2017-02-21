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

#include <list>
#include <map>
#include <set>
#include <string>
#include <vector>

#include <mesos/slave/container_logger.hpp>

#include <process/check.hpp>
#include <process/collect.hpp>
#include <process/defer.hpp>
#include <process/delay.hpp>
#include <process/io.hpp>
#include <process/owned.hpp>
#include <process/reap.hpp>
#include <process/subprocess.hpp>

#include <stout/adaptor.hpp>
#include <stout/fs.hpp>
#include <stout/hashmap.hpp>
#include <stout/hashset.hpp>
#include <stout/jsonify.hpp>
#include <stout/os.hpp>
#include <stout/uuid.hpp>
#include <stout/error.hpp>
#include <stout/foreach.hpp>
#include <stout/json.hpp>
#include <stout/lambda.hpp>
#include <stout/os.hpp>
#include <stout/path.hpp>
#include <stout/result.hpp>
#include <stout/strings.hpp>
#include <stout/stringify.hpp>

#include <stout/os/killtree.hpp>
#include <stout/os/read.hpp>
#include <stout/os/write.hpp>

#include <process/check.hpp>
#include <process/collect.hpp>
#include <process/io.hpp>

#include "common/status_utils.hpp"

#include "hook/manager.hpp"

#ifdef __linux__
#include "linux/cgroups.hpp"
#include "linux/fs.hpp"
#include "linux/systemd.hpp"
#endif // __linux__

#include "slave/paths.hpp"
#include "slave/slave.hpp"

#include "slave/containerizer/containerizer.hpp"
#include "slave/containerizer/lxd/lxd.hpp"
#include "slave/containerizer/fetcher.hpp"

#include "usage/usage.hpp"

#ifdef __linux__
#include "linux/cgroups.hpp"
#endif // __linux__

#include "slave/constants.hpp"


using namespace process;

using std::list;
using std::map;
using std::set;
using std::string;
using std::vector;

using mesos::slave::ContainerLogger;
using mesos::slave::ContainerTermination;

using mesos::internal::slave::state::SlaveState;
using mesos::internal::slave::state::FrameworkState;
using mesos::internal::slave::state::ExecutorState;
using mesos::internal::slave::state::RunState;



using namespace mesos;
using namespace mesos::internal::slave;
using namespace process;


namespace mesos {
namespace internal {
namespace slave {


// Declared in header, see explanation there.
const string LXD_NAME_PREFIX = "mesos-";


// Declared in header, see explanation there.
const string LXD_NAME_SEPERATOR = ".";


Try<LxdContainerizer*> LxdContainerizer::create(
    const Flags& flags,
    Fetcher* fetcher,
    const Option<NvidiaComponents>& nvidia)
{
  LOG(INFO) << "**** Creating the containarizer ***** ";
  // Create and initialize the container logger module.
  Try<ContainerLogger*> logger =
    ContainerLogger::create(flags.container_logger);

  if (logger.isError()) {
    return Error("Failed to create container logger: " + logger.error());
  }

  return new LxdContainerizer(
      flags,
      fetcher,
      Owned<ContainerLogger>(logger.get()),
      nvidia);
}


LxdContainerizer::LxdContainerizer(
    const Flags& flags,
    Fetcher* fetcher,
    const Owned<ContainerLogger>& logger,
    const Option<NvidiaComponents>& nvidia)
{
    this->process = new Lxd.create()
}


LxdContainerizer::~LxdContainerizer()
{
}


Future<bool> LxdContainerizer::launch(
    const ContainerID& containerId,
    const Option<TaskInfo>& taskInfo,
    const ExecutorInfo& executorInfo,
    const string& directory,
    const Option<string>& user,
    const SlaveID& slaveId,
    const map<string, string>& environment,
    bool checkpoint)
{
  LOG(INFO) << "**** Launching a container with: " << output;
  return dispatch(
      process.get(),
      &Lxd::launch,
      containerId,
      taskInfo,
      executorInfo,
      directory,
      user,
      slaveId,
      environment,
      checkpoint);
}


Future<Nothing> LxdContainerizer::recover(
    const Option<SlaveState>& state)
{
  LOG(INFO) << "**** Recover a container ***** ";
  Future<Nothing> f = Nothing();
  return f;
}


Future<Nothing> LxdContainerizer::update(
    const ContainerID& containerId,
    const Resources& resources)
{
  LOG(INFO) << "**** Update a container ***** ";
  Future<Nothing> f = Nothing();
  return f;
}


Future<ResourceStatistics> LxdContainerizer::usage(
    const ContainerID& containerId)
{
  LOG(INFO) << "**** Usage a container ***** ";
  Future<Nothing> f = Nothing();
  return f;
}


Future<ContainerStatus> LxdContainerizer::status(
    const ContainerID& containerId)
{
  LOG(INFO) << "**** Status a container ***** ";
  Future<Nothing> f = Nothing();
  return f;
}


Future<Option<ContainerTermination>> LxdContainerizer::wait(
    const ContainerID& containerId)
{
  LOG(INFO) << "**** Wait a container ***** ";
  Future<Nothing> f = Nothing();
  return f;
}


Future<bool> LxdContainerizer::destroy(const ContainerID& containerId)
{
  LOG(INFO) << "**** Destroy a container ***** ";
  Future<Nothing> f = Nothing();
  return f;
}


Future<hashset<ContainerID>> LxdContainerizer::containers()
{
  LOG(INFO) << "**** Destroy a container ***** ";
  Future<Nothing> f = Nothing();
  return f;
}

void commandDiscarded(const Subprocess& s, const string& cmd)
{
  VLOG(1) << "'" << cmd << "' is being discarded";
  os::killtree(s.pid(), SIGKILL);
}




Try<Owned<Lxd>> Lxd::create()
{
  Owned<Lxd> lxd(new Lxd());

  return lxd;
}

Future<Option<int>> Lxd::launch(
    const ContainerInfo& containerInfo,
    const CommandInfo& commandInfo,
    const string& name,
    const string& sandboxDirectory,
    const string& mappedDirectory,
    const Option<Resources>& resources,
    const Option<map<string, string>>& env,
    const process::Subprocess::IO& _stdout,
    const process::Subprocess::IO& _stderr) const
{
  if (!containerInfo.has_docker()) {
    return Failure("No docker info found in container info");
  }

  const ContainerInfo::DockerInfo& dockerInfo = containerInfo.docker();

  vector<string> argv;
  argv.push_back("lxc");
  argv.push_back("launch");
  argv.push_back("ubuntu");
  argv.push_back(name);

  string cmd = strings::join(" ", argv);

  LOG(INFO) << "Running " << cmd;

  Try<Subprocess> s = subprocess(
      path,
      argv,
      Subprocess::PATH("/dev/null"),
      _stdout,
      _stderr);

  if (s.isError()) {
    return Failure("Failed to create subprocess '" + cmd + "': " + s.error());
  }

  s->status()
    .onDiscard(lambda::bind(&commandDiscarded, s.get(), cmd));
  return s->status();
}


} // namespace slave {
} // namespace internal {
} // namespace mesos {
