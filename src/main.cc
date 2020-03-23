
#include <gflags/gflags.h>
#include <butil/logging.h>
#include <brpc/controller.h>
#include <brpc/server.h>
#include <braft/raft.h>
#include <braft/util.h>
#include "rpc.h"
#include "flags.h"

// #include <butil/time.h>
int main(int argc, char **argv)
{
    GFLAGS_NS::ParseCommandLineFlags(&argc, &argv, true);
    butil::AtExitManager exit_manager;

    brpc::Server server;
    StateMachineImpl fsm;
    ReportServiceImpl service(&fsm);

    LOG(INFO) << "current time:" << butil::gettimeofday_us();
    LOG(INFO) << "IP" << FLAGS_ip;
    // Add your service into RPC rerver
    if (server.AddService(&service, brpc::SERVER_DOESNT_OWN_SERVICE) != 0)
    {
        LOG(ERROR) << "Fail to add service";
        return -1;
    }
    if (braft::add_service(&server, FLAGS_port) != 0)
    {
        LOG(ERROR) << "Fail to add raft service";
        return -1;
    }
    if (server.Start(FLAGS_port, NULL) != 0)
    {
        LOG(ERROR) << "Fail to start Server";
        return -1;
    }
    if (fsm.start(FLAGS_ip, FLAGS_port, FLAGS_conf, FLAGS_group,
                  FLAGS_election_timeout_ms, FLAGS_snapshot_interval,
                  FLAGS_data_path, FLAGS_disable_cli) != 0)
    {
        LOG(ERROR) << "Fail to start fsm";
        return -1;
    }

    LOG(INFO) << "center server is running on " << server.listen_address();

    while (!brpc::IsAskedToQuit())
    {
        sleep(1);
    }

    LOG(INFO) << "center server is going to quit";

    fsm.shutdown();
    server.Stop(0);

    fsm.join();
    server.Join();

    return 0;
}
