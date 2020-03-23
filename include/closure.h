#ifndef __CLOSURE_H__
#define __CLOSURE_H__

#include <braft/raft.h>
#include <brpc/controller.h>
#include "state.h"
#include "retcode.h"
#include "report.pb.h"
#include "struct.pb.h"

struct SnapshotClosure
{
    ServerInfoList serverInfoList;
    braft::SnapshotWriter *writer;
    braft::Closure *done;
};

// Implements Closure which encloses RPC stuff
template <class stateMachine, class reqType, class resType>
class CommonClosure : public braft::Closure
{
public:
    CommonClosure(stateMachine *fsm,
                  const reqType *request,
                  resType *response,
                  google::protobuf::Closure *done)
        : _fsm(fsm),
          _request(request),
          _response(response),
          _done(done) {}
    virtual ~CommonClosure() {}

    const reqType *request() const { return _request; }
    resType *response() const { return _response; }
    virtual void Run() = 0;

protected:
    stateMachine *_fsm;
    const reqType *_request;
    resType *_response;
    google::protobuf::Closure *_done;
};

class StateMachineImpl;

#define DECLARE_CLOSURE(ClosureName, reqType, resType)                    \
    class ClosureName : public CommonClosure<StateMachineImpl,            \
                                             reqType,                     \
                                             resType>                     \
    {                                                                     \
        using CommonClosure::CommonClosure;                               \
        void Run()                                                        \
        {                                                                 \
            std::unique_ptr<ClosureName> self_guard(this);                \
            brpc::ClosureGuard done_guard(_done);                         \
            if (status().ok())                                            \
            {                                                             \
                return;                                                   \
            }                                                             \
            _response->set_ret(static_cast<int>(RET_CODE::ERR_REDIRECT)); \
        }                                                                 \
    };

//为proto中定义的rpc请求定义closure类
DECLARE_CLOSURE(RegisterClosure, report::registerReq, report::registerRes);
DECLARE_CLOSURE(HeartbeatClosure, report::heartbeatReq, report::heartbeatRes);

//将定义的closure类加入StateMachine的友元类
#define CLOSURE_AUTO_FRIEND_CLASS \
    friend class RegisterClosure; \
    friend class HeartbeatClosure;

#endif // __CLOSURE_H__
