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
class LxdContainerizerProcess;

// Forward declaration.
class Lxd;


class LxdContainerizer : public Containerizer
{
public:
  static Try<LxdContainerizer*> create(
      const Flags& flags,
      Fetcher* fetcher,
      const Option<NvidiaComponents>& nvidia = None());

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



class LxdContainerizerProcess
  : public process::Process<LxdContainerizerProcess>
{
public:
  LxdContainerizerProcess(
      const Flags& _flags,
      Fetcher* _fetcher,
      const process::Owned<mesos::slave::ContainerLogger>& _logger,
      process::Shared<Docker> _docker,
      const Option<NvidiaComponents>& _nvidia)
    : flags(_flags),
      fetcher(_fetcher),
      logger(_logger),
      docker(_docker),
      nvidia(_nvidia) {}

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

  // force = true causes the containerizer to update the resources
  // for the container, even if they match what it has cached.
  virtual process::Future<Nothing> update(
      const ContainerID& containerId,
      const Resources& resources,
      bool force);

  virtual process::Future<ResourceStatistics> usage(
      const ContainerID& containerId);

  virtual process::Future<ContainerStatus> status(
      const ContainerID& containerId);

  virtual process::Future<Option<mesos::slave::ContainerTermination>> wait(
      const ContainerID& containerId);

  virtual process::Future<bool> destroy(
      const ContainerID& containerId,
      bool killed = true); // process is either killed or reaped.

  virtual process::Future<Nothing> fetch(
      const ContainerID& containerId,
      const SlaveID& slaveId);

  virtual process::Future<Nothing> pull(const ContainerID& containerId);

  virtual process::Future<hashset<ContainerID>> containers();

private:
  // Continuations and helpers.
  process::Future<Nothing> _fetch(
      const ContainerID& containerId,
      const Option<int>& status);

  Try<Nothing> checkpoint(
      const ContainerID& containerId,
      pid_t pid);

  process::Future<bool> _launch(
      const ContainerID& containerId,
      const Option<TaskInfo>& taskInfo,
      const ExecutorInfo& executorInfo,
      const std::string& directory,
      const SlaveID& slaveId);

  process::Future<Nothing> _recover(
      const Option<state::SlaveState>& state,
      const std::list<Docker::Container>& containers);

  process::Future<Nothing> __recover(
      const std::list<Docker::Container>& containers);

  // Starts the executor in a Docker container.
  process::Future<Docker::Container> launchExecutorContainer(
      const ContainerID& containerId,
      const std::string& containerName);

  // Starts the docker executor with a subprocess.
  process::Future<pid_t> launchExecutorProcess(
      const ContainerID& containerId);

  process::Future<pid_t> checkpointExecutor(
      const ContainerID& containerId,
      const Docker::Container& dockerContainer);

  // Reaps on the executor pid.
  process::Future<bool> reapExecutor(
      const ContainerID& containerId,
      pid_t pid);

  void _destroy(
      const ContainerID& containerId,
      bool killed);

  void __destroy(
      const ContainerID& containerId,
      bool killed,
      const process::Future<Nothing>& future);

  void ___destroy(
      const ContainerID& containerId,
      bool killed,
      const process::Future<Option<int>>& status);

  void ____destroy(
      const ContainerID& containerId,
      bool killed,
      const process::Future<Option<int>>& status);

  process::Future<Nothing> destroyTimeout(
      const ContainerID& containerId,
      process::Future<Nothing> future);

  process::Future<Nothing> _update(
      const ContainerID& containerId,
      const Resources& resources,
      const Docker::Container& container);

  process::Future<Nothing> __update(
      const ContainerID& containerId,
      const Resources& resources,
      pid_t pid);

  process::Future<Nothing> mountPersistentVolumes(
      const ContainerID& containerId);

  Try<Nothing> unmountPersistentVolumes(
      const ContainerID& containerId);

  Try<Nothing> updatePersistentVolumes(
    const ContainerID& containerId,
    const std::string& directory,
    const Resources& current,
    const Resources& updated);

#ifdef __linux__
  // Allocate GPU resources for a specified container.
  process::Future<Nothing> allocateNvidiaGpus(
      const ContainerID& containerId,
      const size_t count);

  process::Future<Nothing> _allocateNvidiaGpus(
      const ContainerID& containerId,
      const std::set<Gpu>& allocated);

  // Deallocate GPU resources for a specified container.
  process::Future<Nothing> deallocateNvidiaGpus(
      const ContainerID& containerId);

  process::Future<Nothing> _deallocateNvidiaGpus(
      const ContainerID& containerId,
      const std::set<Gpu>& deallocated);
#endif // __linux__

  Try<ResourceStatistics> cgroupsStatistics(pid_t pid) const;

  // Call back for when the executor exits. This will trigger
  // container destroy.
  void reaped(const ContainerID& containerId);

  // Removes the docker container.
  void remove(
      const std::string& containerName,
      const Option<std::string>& executor);

  const Flags flags;

  Fetcher* fetcher;

  process::Owned<mesos::slave::ContainerLogger> logger;

  process::Shared<Docker> docker;

  Option<NvidiaComponents> nvidia;

  struct Container
  {
    static Try<Container*> create(
        const ContainerID& id,
        const Option<TaskInfo>& taskInfo,
        const ExecutorInfo& executorInfo,
        const std::string& directory,
        const Option<std::string>& user,
        const SlaveID& slaveId,
        const std::map<std::string, std::string>& environment,
        bool checkpoint,
        const Flags& flags);

    static std::string name(const SlaveID& slaveId, const std::string& id)
    {
      return DOCKER_NAME_PREFIX + slaveId.value() + DOCKER_NAME_SEPERATOR +
        stringify(id);
    }

    Container(const ContainerID& id)
      : state(FETCHING), id(id) {}

    Container(const ContainerID& id,
              const Option<TaskInfo>& taskInfo,
              const ExecutorInfo& executorInfo,
              const std::string& directory,
              const Option<std::string>& user,
              const SlaveID& slaveId,
              bool checkpoint,
              bool symlinked,
              const Flags& flags,
              const Option<CommandInfo>& _command,
              const Option<ContainerInfo>& _container,
              const std::map<std::string, std::string>& _environment,
              bool launchesExecutorContainer)
      : state(FETCHING),
        id(id),
        task(taskInfo),
        executor(executorInfo),
        environment(_environment),
        directory(directory),
        user(user),
        slaveId(slaveId),
        checkpoint(checkpoint),
        symlinked(symlinked),
        flags(flags),
        launchesExecutorContainer(launchesExecutorContainer)
    {
      // NOTE: The task's resources are included in the executor's
      // resources in order to make sure when launching the executor
      // that it has non-zero resources in the event the executor was
      // not actually given any resources by the framework
      // originally. See Framework::launchExecutor in slave.cpp. We
      // check that this is indeed the case here to protect ourselves
      // from when/if this changes in the future (but it's not a
      // perfect check because an executor might always have a subset
      // of it's resources that match a task, nevertheless, it's
      // better than nothing).
      resources = executor.resources();

      if (task.isSome()) {
        CHECK(resources.contains(task.get().resources()));
      }

      if (_command.isSome()) {
        command = _command.get();
      } else if (task.isSome()) {
        command = task.get().command();
      } else {
        command = executor.command();
      }

      if (_container.isSome()) {
        container = _container.get();
      } else if (task.isSome()) {
        container = task.get().container();
      } else {
        container = executor.container();
      }
    }

    ~Container()
    {
      if (symlinked) {
        // The sandbox directory is a symlink, remove it at container
        // destroy.
        os::rm(directory);
      }
    }

    std::string name()
    {
      return name(slaveId, stringify(id));
    }

    Option<std::string> executorName()
    {
      if (launchesExecutorContainer) {
        return name() + DOCKER_NAME_SEPERATOR + "executor";
      } else {
        return None();
      }
    }

    std::string image() const
    {
      if (task.isSome()) {
        return task.get().container().docker().image();
      }

      return executor.container().docker().image();
    }

    bool forcePullImage() const
    {
      if (task.isSome()) {
        return task.get().container().docker().force_pull_image();
      }

      return executor.container().docker().force_pull_image();
    }

    // The DockerContainerizer needs to be able to properly clean up
    // Docker containers, regardless of when they are destroyed. For
    // example, if a container gets destroyed while we are fetching,
    // we need to not keep running the fetch, nor should we try and
    // start the Docker container. For this reason, we've split out
    // the states into:
    //
    //     FETCHING
    //     PULLING
    //     MOUNTING
    //     RUNNING
    //     DESTROYING
    //
    // In particular, we made 'PULLING' be it's own state so that we
    // can easily destroy and cleanup when a user initiated pulling
    // a really big image but we timeout due to the executor
    // registration timeout. Since we currently have no way to discard
    // a Docker::run, we needed to explicitly do the pull (which is
    // the part that takes the longest) so that we can also explicitly
    // kill it when asked. Once the functions at Docker::* get support
    // for discarding, then we won't need to make pull be it's own
    // state anymore, although it doesn't hurt since it gives us
    // better error messages.
    enum State
    {
      FETCHING = 1,
      PULLING = 2,
      MOUNTING = 3,
      RUNNING = 4,
      DESTROYING = 5
    } state;

    const ContainerID id;
    const Option<TaskInfo> task;
    const ExecutorInfo executor;
    ContainerInfo container;
    CommandInfo command;
    std::map<std::string, std::string> environment;

    // Environment variables that the command executor should pass
    // onto a docker-ized task. This is set by a hook.
    Option<std::map<std::string, std::string>> taskEnvironment;

    // The sandbox directory for the container. This holds the
    // symlinked path if symlinked boolean is true.
    std::string directory;

    const Option<std::string> user;
    SlaveID slaveId;
    bool checkpoint;
    bool symlinked;
    const Flags flags;

    // Promise for future returned from wait().
    process::Promise<mesos::slave::ContainerTermination> termination;

    // Exit status of executor or container (depending on whether or
    // not we used the command executor). Represented as a promise so
    // that destroying can chain with it being set.
    process::Promise<process::Future<Option<int>>> status;

    // Future that tells us the return value of last launch stage (fetch, pull,
    // run, etc).
    process::Future<bool> launch;

    // We keep track of the resources for each container so we can set
    // the ResourceStatistics limits in usage(). Note that this is
    // different than just what we might get from TaskInfo::resources
    // or ExecutorInfo::resources because they can change dynamically.
    Resources resources;

    // The docker pull future is stored so we can discard when
    // destroy is called while docker is pulling the image.
    process::Future<Docker::Image> pull;

    // Once the container is running, this saves the pid of the
    // running container.
    Option<pid_t> pid;

    // The executor pid that was forked to wait on the running
    // container. This is stored so we can clean up the executor
    // on destroy.
    Option<pid_t> executorPid;

#ifdef __linux__
    // GPU resources allocated to the container.
    std::set<Gpu> gpus;
#endif // __linux__

    // Marks if this container launches an executor in a docker
    // container.
    bool launchesExecutorContainer;
  };

  hashmap<ContainerID, Container*> containers_;
};


class Lxd
{
public:
  // Create Docker abstraction and optionally validate docker.
  static Try<process::Owned<Lxd>> create(
      const std::string& path,
      const std::string& socket,
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
  virtual process::Future<Option<int>> launch(
      const mesos::ContainerInfo& containerInfo,
      const mesos::CommandInfo& commandInfo,
      const std::string& containerName,
      const std::string& sandboxDirectory,
      const std::string& mappedDirectory,
      const Option<mesos::Resources>& resources = None(),
      const Option<std::map<std::string, std::string>>& env = None(),
      const process::Subprocess::IO& _stdout =
        process::Subprocess::FD(STDOUT_FILENO),
      const process::Subprocess::IO& _stderr =
        process::Subprocess::FD(STDERR_FILENO))
    const;

  virtual ~Lxd() {};
};


} // namespace slave {
} // namespace internal {
} // namespace mesos {

#endif // __LXD_CONTAINERIZER_HPP__
