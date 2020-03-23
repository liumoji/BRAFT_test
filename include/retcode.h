#ifndef __RETCODE_H__
#define __RETCODE_H__
enum class RET_CODE : int
{
    OK = 0,           //成功
    ERR_REDIRECT,     //非leader, 需要重定向
    ERR_SERIALIZE_REQ, //序列化request失败
    ERR_UPDATE_ALIVE    //更新存活状态失败
};

#endif // __RETCODE_H__
