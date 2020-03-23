/**
 * 状态机存储的状态
 */
#ifndef __STATE_H__
#define __STATE_H__
#include <vector>
#include <unordered_map>
#include <braft/protobuf_file.h>
#include "closure.h"
#include "struct.pb.h"

typedef std::vector<ServerInfo> ServerList;
typedef std::unordered_map<std::string, ServerInfo> ServerMap;
typedef std::unordered_map<std::int32_t, ServerMap> ServerManagerMap;

class State
{
public:
    State() {}
    ~State() {}
    void rigisterServer(ServerInfo serverInfo);
    int updateServerHeart(HeartbeatInfo heartbeatInfo);
    void saveSnapshot(braft::SnapshotWriter *writer, braft::Closure *done);
    int loadSnapshot(braft::SnapshotReader *reader);

private:
    static void *saveSnapshot(void *arg);
    ServerMap _serverMap;
    ServerManagerMap _serverManagerMap;
};

#endif // __STATE_H__
