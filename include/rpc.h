#ifndef __RPC_H__
#define __RPC_H__
#include <brpc/server.h>
#include <brpc/controller.h>
#include "fsm.h"
#include "report.pb.h"
class ReportServiceImpl : public report::reportService
{
public:
    explicit ReportServiceImpl(StateMachineImpl *fsm) : _fsm(fsm) {}
    virtual ~ReportServiceImpl() {}

    virtual void rigister(::google::protobuf::RpcController *cntl_base,
                  const ::report::registerReq *request,
                  ::report::registerRes *response,
                  ::google::protobuf::Closure *done);

    virtual void heartbeat(::google::protobuf::RpcController *cntl_base,
                   const ::report::heartbeatReq *request,
                   ::report::heartbeatRes *response,
                   ::google::protobuf::Closure *done);

private:
    StateMachineImpl *_fsm;
};

#endif // __RPC_H__
