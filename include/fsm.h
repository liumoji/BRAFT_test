#ifndef __FSM_H__
#define __FSM_H__
#include <unordered_map>
#include <braft/raft.h>
#include "state.h"
#include "closure.h"
#include "struct.pb.h"

class StateMachineImpl : public braft::StateMachine
{
public:
    StateMachineImpl()
        : _node(NULL), _leader_term(-1)
    {
    }
    ~StateMachineImpl()
    {
        delete _node;
    }

    bool is_leader() const
    {
        return _leader_term.load(butil::memory_order_acquire) > 0;
    }

    void shutdown()
    {
        if (_node)
        {
            _node->shutdown(NULL);
        }
    }
    // Blocking this thread until the node is eventually down.
    void join()
    {
        if (_node)
        {
            _node->join();
        }
    }

    int start(std::string ip, int port, std::string conf,
              std::string group, int election_timeout_ms,
              int snapshot_interval, std::string data_path,
              bool disable_cli);

    std::string getRedirect();

    //--------rpc handler---------
    void rigister(::google::protobuf::RpcController *cntl_base,
                  const ::report::registerReq *request,
                  ::report::registerRes *response,
                  ::google::protobuf::Closure *done);
    void heartbeat(const ::report::heartbeatReq *request,
                   ::report::heartbeatRes *response,
                   ::google::protobuf::Closure *done);

private:
    CLOSURE_AUTO_FRIEND_CLASS;
    void on_apply(braft::Iterator &iter);
    void on_snapshot_save(braft::SnapshotWriter *writer, braft::Closure *done);
    int on_snapshot_load(braft::SnapshotReader *reader);

    void on_leader_start(int64_t term)
    {
        _leader_term.store(term, butil::memory_order_release);
        LOG(INFO) << "Node becomes leader";
    }
    void on_leader_stop(const butil::Status &status)
    {
        _leader_term.store(-1, butil::memory_order_release);
        LOG(INFO) << "Node stepped down : " << status;
    }

    void on_shutdown()
    {
        LOG(INFO) << "This node is down";
    }
    void on_error(const ::braft::Error &e)
    {
        LOG(ERROR) << "Met raft error " << e;
    }
    void on_configuration_committed(const ::braft::Configuration &conf)
    {
        LOG(INFO) << "Configuration of this group is " << conf;
    }
    void on_stop_following(const ::braft::LeaderChangeContext &ctx)
    {
        LOG(INFO) << "Node stops following " << ctx;
    }
    void on_start_following(const ::braft::LeaderChangeContext &ctx)
    {
        LOG(INFO) << "Node start following " << ctx;
    }

private:
    braft::Node *volatile _node;
    butil::atomic<int64_t> _leader_term;

private: // 状态机自定义私有成员
    std::shared_ptr<State> _state = std::make_shared<State>();
};

#endif // __FSM_H__
