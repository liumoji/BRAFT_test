#include <butil/logging.h>
#include "state.h"

void State::rigisterServer(ServerInfo serverInfo)
{
    _serverManagerMap[serverInfo.type()][serverInfo.id()] = serverInfo;
}

int State::updateServerHeart(HeartbeatInfo heartbeatInfo)
{
    if (_serverManagerMap.end() != _serverManagerMap.find(heartbeatInfo.type()) ||
        _serverManagerMap[heartbeatInfo.type()].end() != _serverManagerMap[heartbeatInfo.type()].find(heartbeatInfo.id())) {
        _serverManagerMap[heartbeatInfo.type()][heartbeatInfo.id()].set_health_grade(heartbeatInfo.health_grade());
        _serverManagerMap[heartbeatInfo.type()][heartbeatInfo.id()].set_active_time(heartbeatInfo.active_time());
        return static_cast<int> (RET_CODE::OK);
    }
    else {
        return static_cast<int>(RET_CODE::ERR_UPDATE_ALIVE);
    }
}

void State::saveSnapshot(braft::SnapshotWriter *writer, braft::Closure *done)
{
    SnapshotClosure *sc = new SnapshotClosure;
    auto fIter = _serverManagerMap.begin();
    for (; fIter != _serverManagerMap.end(); fIter++)
    {
        for (auto sIter = fIter->second.begin(); sIter != fIter->second.end(); sIter++) {
            auto ptr = sc->serverInfoList.add_server_info();
            ptr->CopyFrom(sIter->second);
        }
    }
    sc->writer = writer;
    sc->done = done;
    bthread_t tid;
    bthread_start_urgent(&tid, NULL, saveSnapshot, sc);
}

int State::loadSnapshot(braft::SnapshotReader *reader)
{
    std::string snapshot_path = reader->get_path();
    snapshot_path.append("/server_list");
    braft::ProtoBufFile pb_file(snapshot_path);
    ServerInfoList *_serverInfoListPtr = new ServerInfoList;
    if (pb_file.load(_serverInfoListPtr) != 0)
    {
        LOG(ERROR) << "Fail to load snapshot from " << snapshot_path;
        return -1;
    }
    int size = _serverInfoListPtr->server_info_size();
    _serverManagerMap.clear();
    for (int i = 0; i < size; i++)
    {
        ServerInfo svri = _serverInfoListPtr->server_info(i);
        _serverManagerMap[svri.type()][svri.id()] = svri;
    }
    return 0;
}

void *State::saveSnapshot(void *arg)
{
    SnapshotClosure *sc = (SnapshotClosure *)arg;
    std::unique_ptr<SnapshotClosure> sc_guard(sc);
    brpc::ClosureGuard done_guard(sc->done);
    std::string snapshot_path = sc->writer->get_path() + "/server_list";
    LOG(INFO) << "Saving snapshot serverList to " << snapshot_path;
    braft::ProtoBufFile pb_file(snapshot_path);
    if (pb_file.save(&(sc->serverInfoList), true) != 0)
    {
        sc->done->status().set_error(EIO, "Fail to save pb_file");
        return NULL;
    }
    if (sc->writer->add_file("server_list") != 0)
    {
        sc->done->status().set_error(EIO, "Fail to add file to writer");
        return NULL;
    }
    return NULL;
}
