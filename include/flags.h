#ifndef __FLAGS_H__
#define __FLAGS_H__

#include <gflags/gflags.h> // DEFINE_*
DEFINE_bool(check_term, true, "Check if the leader changed to another term");
DEFINE_bool(disable_cli, false, "Don't allow raft_cli access this node");
DEFINE_bool(log_applied_task, false, "Print notice log when a task is applied");
DEFINE_int32(election_timeout_ms, 5000,
             "Start election in such milliseconds if disconnect with the leader");
DEFINE_string(ip, "", "Bind ip of this peer");
DEFINE_int32(port, 8100, "Listen port of this peer");
DEFINE_int32(snapshot_interval, 30, "Interval between each snapshot");
DEFINE_string(conf, "", "Initial configuration of the replication group");
DEFINE_string(data_path, "./data", "Path of data stored on");
DEFINE_string(group, "center_svr", "Id of the replication group");

#endif // __FLAGS_H__
