#include "rpc.h"
#include "retcode.h"

//async
void ReportServiceImpl::rigister(::google::protobuf::RpcController *cntl_base,
                                 const ::report::registerReq *request,
                                 ::report::registerRes *response,
                                 ::google::protobuf::Closure *done)
{
    return _fsm->rigister(cntl_base, request, response, done);
}

void ReportServiceImpl::heartbeat(::google::protobuf::RpcController *cntl_base,
                                  const ::report::heartbeatReq *request,
                                  ::report::heartbeatRes *response,
                                  ::google::protobuf::Closure *done)
{
    return _fsm->heartbeat(request, response, done);
}
