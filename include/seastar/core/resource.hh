/*
 * This file is open source software, licensed to you under the terms
 * of the Apache License, Version 2.0 (the "License").  See the NOTICE file
 * distributed with this work for additional information regarding copyright
 * ownership.  You may not use this file except in compliance with the License.
 *
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */
/*
 * Copyright (C) 2014 Cloudius Systems, Ltd.
 */

#pragma once

#include <cassert>
#include <cstdlib>
#include <string>
#include <seastar/util/std-compat.hh>
#include <seastar/util/spinlock.hh>
#include <vector>
#include <set>
#include <sched.h>
#include <boost/any.hpp>
#include <unordered_map>

namespace seastar {

cpu_set_t cpuid_to_cpuset(unsigned cpuid);
class io_queue;
class io_group;

namespace resource {

using std::optional;

using cpuset = std::set<unsigned>;

struct configuration {
    optional<size_t> total_memory;
    optional<size_t> reserve_memory;  // if total_memory not specified
    optional<size_t> cpus;
    optional<cpuset> cpu_set;
    bool assign_orphan_cpus = false;
    std::vector<dev_t> devices;
    unsigned num_io_groups;
};

struct memory {
    size_t bytes;
    unsigned nodeid;

};

// Since this is static information, we will keep a copy at each CPU.
// This will allow us to easily find who is the IO coordinator for a given
// node without a trip to a remote CPU.
struct io_queue_topology {
    unsigned nr_queues;
    std::vector<unsigned> shard_to_group;
    unsigned nr_groups;
};

struct cpu {
    unsigned cpu_id;
    std::vector<memory> mem;
};

struct resources {
    std::vector<cpu> cpus;
    std::unordered_map<dev_t, io_queue_topology> ioq_topology;
};

struct device_io_topology {
    std::vector<io_queue*> queues;
    struct group {
        std::shared_ptr<io_group> g;
        unsigned attached = 0;
    };
    util::spinlock lock;
    std::vector<group> groups;

    device_io_topology() noexcept = default;
    device_io_topology(const io_queue_topology& iot) noexcept : queues(iot.nr_queues), groups(iot.nr_groups) {}
};

resources allocate(configuration c);
unsigned nr_processing_units();
}

// We need a wrapper class, because boost::program_options wants validate()
// (below) to be in the same namespace as the type it is validating.
struct cpuset_bpo_wrapper {
    resource::cpuset value;
};

// Overload for boost program options parsing/validation
extern
void validate(boost::any& v,
              const std::vector<std::string>& values,
              cpuset_bpo_wrapper* target_type, int);

}
