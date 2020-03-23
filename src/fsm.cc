#include <uuid/uuid.h>
#include <butil/iobuf.h>
#include <brpc/controller.h>
#include <braft/util.h>
#include <braft/protobuf_file.h>
#include <time.h>
#include "fsm.h"
#include "retcode.h"
#include "closure.h"
#include "struct.pb.h"
#include "report.pb.h"

int StateMachineImpl::start(std::string ip, int port, std::string conf,
                            std::string group, int election_timeout_ms,
                            int snapshot_interval, std::string data_path,
                            bool disable_cli)
{
    butil::ip_t my_ip;
    CHECK_EQ(0, butil::str2ip(ip.c_str(), &my_ip)) << "Transform string to ip failed";
    butil::EndPoint addr(my_ip, port);
    braft::NodeOptions node_options;
    if (node_options.initial_conf.parse_from(conf) != 0)
    {
        LOG(ERROR) << "Fail to parse configuration `" << conf << '\'';
        return -1;
    }
    node_options.election_timeout_ms = election_timeout_ms;
    node_options.fsm = this;
    node_options.node_owns_fsm = false;
    node_options.snapshot_interval_s = snapshot_interval;
    std::string prefix = "local://" + data_path;
    node_options.log_uri = prefix + "/wal";
    node_options.raft_meta_uri = prefix + "/raft_meta";
    node_options.snapshot_uri = prefix + "/snapshot";
    node_options.disable_cli = disable_cli;
    braft::Node *node = new braft::Node(group, braft::PeerId(addr));
    if (node->init(node_options) != 0)
    {
        LOG(ERROR) << "Fail to init raft node";
        delete node;
        return -1;
    }
    _node = node;
    return 0;
}

void StateMachineImpl::on_apply(braft::Iterator &iter)
{
    for (; iter.valid(); iter.next())
    {
        braft::AsyncClosureGuard done_guard(iter.done());
        butil::IOBuf data = iter.data();
        butil::IOBufAsZeroCopyInputStream wrapper(data);
        RaftMessage raftMsg;
        raftMsg.ParseFromZeroCopyStream(&wrapper);

        uint8_t opType = raftMsg.type();

        if (opType == RaftTaskType::RIGISTER)
        {
            // int size = raftMsg.server_info_size();
            ServerInfo svri = raftMsg.server_info(0);
            std::string id = svri.id();
            if (iter.done())
            {
                RegisterClosure *c = static_cast<RegisterClosure *>(iter.done());
                report::registerRes *res = c->response();
                res->set_ret(static_cast<google::protobuf::int32>(RET_CODE::OK));
                res->set_id(id);
            }
            _state->rigisterServer(svri);
        }
        else if (opType == RaftTaskType::HEARTBEAT)
        {
            HeartbeatInfo hearti = raftMsg.heartbeat_info(0);
            int ret_code = _state->updateServerHeart(hearti);
            if (iter.done())
            {
                HeartbeatClosure *h = static_cast<HeartbeatClosure *>(iter.done());
                report::heartbeatRes *res = h->response();
                res->set_ret(static_cast<google::protobuf::int32>(ret_code));
            }
        }
        else
        {
            CHECK(false) << "Unknown type=" << static_cast<int>(opType);
            break;
        }
    }
}

void StateMachineImpl::on_snapshot_save(braft::SnapshotWriter *writer,
                                        braft::Closure *done)
{
    _state->saveSnapshot(writer, done);
}

int StateMachineImpl::on_snapshot_load(braft::SnapshotReader *reader)
{
    if (is_leader())
    {
        LOG(ERROR) << "Leader is not supposed to load snapshot";
        return -1;
    }
    return _state->loadSnapshot(reader);
}

std::string StateMachineImpl::getRedirect()
{
    if (_node)
    {
        braft::PeerId leader = _node->leader_id();
        if (!leader.is_empty())
        {
            return leader.to_string();
        }
    }
    return "";
}

//------rpc handler------
void StateMachineImpl::rigister(::google::protobuf::RpcController *cntl_base,
                                const ::report::registerReq *request,
                                ::report::registerRes *response,
                                ::google::protobuf::Closure *done)
{
    brpc::ClosureGuard done_guard(done);

    const int64_t term = _leader_term.load(butil::memory_order_relaxed);
    if (term < 0)
    {
        response->set_ret(static_cast<int>(RET_CODE::ERR_REDIRECT));
        auto redirect = getRedirect();
        response->set_redirect(redirect);
        return;
    }

    //为首次注册的服务器生成uuid
    uuid_t temp_uuid;
    char server_uuid[37];
    uuid_generate(temp_uuid);
    uuid_unparse(temp_uuid, server_uuid);

    RaftMessage raftMsg;
    raftMsg.set_type(RaftTaskType::RIGISTER);

    ServerInfo *svrInfo;
    svrInfo = raftMsg.add_server_info();
    svrInfo->set_type(request->type());
    svrInfo->set_id(server_uuid);

    brpc::Controller *cntl = static_cast<brpc::Controller *>(cntl_base);
    auto remoteEndpoint = cntl->remote_side();
    svrInfo->set_ip(butil::ip2str(remoteEndpoint.ip).c_str());
    svrInfo->set_port(remoteEndpoint.port);
    svrInfo->set_status(request->status());
    svrInfo->set_health_grade(request->health_grade());

    butil::IOBuf syncLog;
    butil::IOBufAsZeroCopyOutputStream syncLogWrapper(&syncLog);

    if (!raftMsg.SerializeToZeroCopyStream(&syncLogWrapper))
    {
        LOG(ERROR) << "Fail to serialize request";
        response->set_ret(static_cast<int>(RET_CODE::ERR_SERIALIZE_REQ));
        return;
    }
    braft::Task task;
    task.data = &syncLog;
    task.done = new RegisterClosure(this, request, response,
                                    done_guard.release());

    task.expected_term = term;
    return _node->apply(task);
}

void StateMachineImpl::heartbeat(const ::report::heartbeatReq *request,
                                 ::report::heartbeatRes *response,
                                 ::google::protobuf::Closure *done)
{
    brpc::ClosureGuard done_guard(done);
    if (!is_leader())
    {
        //判断是leader节点，不是leader节点则不处理，返回客户端 非leader, 需要重定向
        response->set_ret(static_cast<int>(RET_CODE::ERR_REDIRECT));
        auto redirect = getRedirect();
        response->set_redirect(redirect);
        return;
    }
    //是leader节点，开始处理心跳逻辑
    RaftMessage raftMsg;
    raftMsg.set_type(RaftTaskType::HEARTBEAT);

    HeartbeatInfo *heartInfo;
    heartInfo = raftMsg.add_heartbeat_info();
    heartInfo->set_id(request->id());
    heartInfo->set_type(request->type());
    heartInfo->set_health_grade(request->health_grade());
    time_t curSec = time(NULL);
    heartInfo->set_active_time(curSec);

    butil::IOBuf syncLog;
    butil::IOBufAsZeroCopyOutputStream syncLogWrapper(&syncLog);

    if (!raftMsg.SerializeToZeroCopyStream(&syncLogWrapper))
    {
        LOG(ERROR) << "Fail to serialize request";
        response->set_ret(static_cast<int>(RET_CODE::ERR_SERIALIZE_REQ));
        return;
    }
    braft::Task task;
    task.data = &syncLog;
    task.done = new HeartbeatClosure(this, request, response,
                                     done_guard.release());

    const int64_t term = _leader_term.load(butil::memory_order_relaxed);
    task.expected_term = term;
    return _node->apply(task);
}