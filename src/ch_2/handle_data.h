#include "ace/Event_Handler.h"
#include "ace/INET_Addr.h"
#include "ace/Reactor.h"
#include "ace/SOCK_Stream.h"
#include "ace/Time_Value.h"
#include "data_send.h"
#include <ace/Log_Msg.h>
#include <ace/Log_Priority.h>
#include <ace/SOCK_Acceptor.h>
#include <string>

class Handle_data : public ACE_Event_Handler {
public:
  Handle_data(ACE_Reactor *r = ACE_Reactor::instance());
  ACE_INT32 open();
  ACE_INT32 handle_input(ACE_HANDLE = ACE_INVALID_HANDLE);
  ACE_INT32 handle_close(ACE_HANDLE = ACE_INVALID_HANDLE,
                         ACE_Reactor_Mask mask = 0);

  ACE_HANDLE get_handle() const { return peer_.get_handle(); }
  ACE_SOCK_Stream &get_peer() { return peer_; }

  void set_data_send(Data_send *data_send) { data_send_ = data_send; }

  // 命令方法:
  int handle_pwd();
  int handle_user();
  int handle_pass();
  int handle_syst();
  int handle_feat();
  int handle_type();
  int handle_pasv();
  int handle_epsv();
  int handle_list();
  int handle_quit();
  int handle_cwd(const std::string &path);
  int handle_mkd(const std::string &file_name);
  int handle_rmd(const std::string &file_name);
  int handle_retr(const std::string &file_name);
  int handle_stor(const std::string &file_name);
  int handle_size(const std::string &file_name);
  int handle_dele(const std::string &file_name);

private:
  ~Handle_data() { ACE_DEBUG((LM_DEBUG, "handle data ~dctor .\n")); };

  // 回复命令
  int reply_cmd(std::string buf) {
    ACE_DEBUG((LM_DEBUG, "reply cmd: %s\n", buf.c_str()));
    return peer_.send_n(buf.c_str(), buf.length()) > 0 ? 0 : -1;
  }

private:
  ACE_SOCK_Stream peer_;
  ACE_SOCK_Acceptor acceptor_;
  Data_send *data_send_;
  std::string word_dir_root = "/home/wxj/";
  using Command_Handler_Func = std::function<int()>;

  // command to function map
  std::unordered_map<std::string, Command_Handler_Func> commands_;
};
