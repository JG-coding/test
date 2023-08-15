#include "acceptor.h"
#include "ace/Log_Msg.h"
#include "handle_data.h"

ACE_INT32 main(ACE_INT32 argc, char **argv) {

  const char *newDir = "/home/wxj"; // 修改为目标目录的路径
  if (chdir(newDir) != 0)           // 修改当前工作目录
  {
    ACE_DEBUG((LM_DEBUG, "workDIr change error!\n"));
  }
  ACE_UINT16 port = 0;
  if (argc < 2)
    return -1;
  port = ACE_OS::atoi(argv[1]);
  Acceptor *accept;

  ACE_NEW_RETURN(accept, Acceptor(ACE_Reactor::instance()), -1);
  if (accept->open(port) == -1) {
    accept->handle_close();
    ACE_DEBUG((LM_DEBUG, "main open error!\n"));
    return -1;
  }
  if (ACE_Reactor::run_event_loop() == -1) {
    accept->handle_close();
    ACE_DEBUG((LM_DEBUG, "main run event loop error!\n"));
    return -1;
  }
  accept->handle_close();
  return 0;
}