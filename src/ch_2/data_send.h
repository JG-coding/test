#pragma once
#include "ace/Event_Handler.h"
#include <ace/Log_Msg.h>
#include <ace/Log_Priority.h>
#include <ace/OS_NS_unistd.h>
#include <ace/Reactor.h>
#include <ace/SOCK_Stream.h>
#include <ace/ace_wchar.h>
#include <dirent.h>
#include <grp.h>
#include <pwd.h>

class Data_send : public ACE_Event_Handler {
public:
  Data_send(ACE_Reactor *r = ACE_Reactor::instance()) : ACE_Event_Handler(r) {}
  ACE_INT32 open();
  ACE_INT32 handle_input(ACE_HANDLE = ACE_INVALID_HANDLE);
  ACE_INT32 handle_close(ACE_HANDLE = ACE_INVALID_HANDLE,
                         ACE_Reactor_Mask mask = 0);

  ACE_HANDLE get_handle() const { return peer_.get_handle(); }
  ACE_SOCK_Stream &get_peer() { return peer_; }

  int sendDetailedDirectoryListing();

  int send_file(const std::string &filename);

  // 接收ftp客户端发来的文件
  int recv_file(const std::string &filename);

private:
  ~Data_send() { ACE_DEBUG((LM_DEBUG, "Data_send ~dctor .\n")); };
  std::string getFilePermissions(mode_t mode);
  std::string formatTime(time_t t);

  // 检测文件/目录是否存在
  bool fileExists(const std::string &filename);

private:
  ACE_SOCK_Stream peer_;
};