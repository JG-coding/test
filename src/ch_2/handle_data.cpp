#include "handle_data.h"
#include "ace/Log_Msg.h"
#include "ace/Reactor.h"
#include "ace/SOCK_Stream.h"
#include "ace/Timer_Queue.h"
#include "data_send.h"
#include <ace/OS_NS_stdio.h>
#include <ace/ace_wchar.h>
#include <functional>
#include <iterator>
#include <string>
#include <strings.h>
#include <unistd.h>

Handle_data::Handle_data(ACE_Reactor *r) : ACE_Event_Handler(r) {
  commands_ = {{"pwd", std::bind(&Handle_data::handle_pwd, this)},
               {"user", std::bind(&Handle_data::handle_user, this)},
               {"pass", std::bind(&Handle_data::handle_pass, this)},
               {"syst", std::bind(&Handle_data::handle_syst, this)},
               {"feat", std::bind(&Handle_data::handle_feat, this)},
               {"epsv", std::bind(&Handle_data::handle_epsv, this)},
               {"list", std::bind(&Handle_data::handle_list, this)},
               {"quit", std::bind(&Handle_data::handle_quit, this)},
               {"type", std::bind(&Handle_data::handle_type, this)}};
}

ACE_INT32 Handle_data::open() {
  ACE_INT32 ret = 0;
  ACE_INET_Addr remote_addr;
  get_peer().get_remote_addr(remote_addr);
  ACE_DEBUG((LM_DEBUG, "the remote addr is %s\n", remote_addr.get_host_addr()));
  get_peer().send_n("220\r\n", 5);
  ret = reactor()->register_handler(this, ACE_Event_Handler::READ_MASK);
  if (ret != -1) {
    ACE_DEBUG((LM_DEBUG, "handle data register ok!\n"));
  }
  return ret;
}

ACE_INT32 Handle_data::handle_close(ACE_HANDLE, ACE_Reactor_Mask) {
  get_peer().close();
  ACE_DEBUG((LM_DEBUG, "handle data close.\n"));
  delete this;
  return 0;
}

ACE_INT32 Handle_data::handle_input(ACE_HANDLE) {
  ACE_TCHAR buf[512] = {0};
  ACE_INT32 len;
  len = get_peer().recv(buf, 512);

  if (len > 0) {
    std::string recv = buf;
    auto it = recv.find("\r\n");
    auto it_space = recv.find_first_of(" ");
    std::string command;
    std::string parm;
    if (it_space == std::string::npos) {
      command = recv.substr(0, it);
    } else {
      command = recv.substr(0, it_space);
      parm = recv.substr(it_space + 1, it - it_space - 1);
    }
    ACE_DEBUG((LM_DEBUG, "command is %s, parm is %s\n", command.c_str(),
               parm.c_str()));
    if (command == "USER" && parm == "wxj") {
      return commands_["user"]();

    } else if (command == "PASS" && parm == "wxj196560427") {
      return commands_["pass"]();

    } else if (command == "SYST") {
      return commands_["syst"]();

    } else if (command == "TYPE") {
      return commands_["type"]();

    } else if (command == "PASV") {
      get_peer().send_n("227\r\n", 5);

    } else if (command == "RETR" && !parm.empty()) {
      std::string file_name = parm;
      return handle_retr(file_name);
    }

    else if (command == "QUIT") {
      return commands_["quit"]();

    } else if (command == "CWD") {
      std::string path = parm;
      return handle_cwd(path);

    } else if (command.find("PORT") != std::string::npos) {
      get_peer().send_n("200\r\n", 5);

    } else if (command == "STOR") {
      std::string file_name = parm;
      return handle_stor(file_name);

    } else if (command == "MKD") {
      return handle_mkd(parm);

    } else if (command == "PWD") {
      return commands_["pwd"]();

    } else if (command == "EPSV") {
      return commands_["epsv"]();

    } else if (command.find("FEAT") != std::string::npos) {
      return commands_["feat"]();

    } else if (command == "LIST") {
      return commands_["list"]();

    } else if (command == "RMD") {
      return handle_rmd(parm);

    } else if (command == "DELE") {
      return handle_dele(parm);
    }

    else {
      get_peer().send_n("500\r\n", 5);
    }
    return 0;
  } else if (len == 0) {
    ACE_DEBUG((LM_DEBUG, "recv data len is 0, client exit.\n"));
    return -1;
  } else {
    ACE_DEBUG((LM_DEBUG, "recv data error len < 0"));
    return -1;
  }
}

int Handle_data::handle_pwd() {
  ACE_TCHAR buffer[100] = {0};
  getcwd(buffer, sizeof(buffer));
  ACE_DEBUG((LM_DEBUG, "path : %s\n", buffer));
  std::string path = buffer;
  std::string dir = path.substr(strlen("/home/wxj")) == ""
                        ? "/"
                        : path.substr(strlen("/home/wxj"));
  ACE_OS::sprintf(buffer, "257 \"%s\" is current directory.\r\n", dir.c_str());
  ACE_DEBUG((LM_DEBUG, "PWD : %s\n", buffer));
  return reply_cmd(buffer);
}

int Handle_data::handle_user() {
  ACE_TCHAR buffer[100] = {0};
  ACE_OS::sprintf(buffer, "331 Please specify the password.\r\n");
  ACE_DEBUG((LM_DEBUG, "USER : %s\n", buffer));
  return reply_cmd(buffer);
}

int Handle_data::handle_pass() {
  ACE_TCHAR buffer[100] = {0};
  ACE_OS::sprintf(buffer, "230 Login successful.\r\n");
  ACE_DEBUG((LM_DEBUG, "PASS : %s\n", buffer));
  return reply_cmd(buffer);
}

int Handle_data::handle_syst() {
  ACE_TCHAR buffer[100] = {0};
  ACE_OS::sprintf(buffer, "215 UNIX Type: L8\r\n");
  ACE_DEBUG((LM_DEBUG, "SYST : %s\n", buffer));
  return reply_cmd(buffer);
}

int Handle_data::handle_feat() {
  ACE_TCHAR buffer[100] = {0};
  ACE_OS::sprintf(buffer, "211-Features:\r\n");
  ACE_OS::sprintf(buffer, "%s211 End\r\n", buffer);
  ACE_DEBUG((LM_DEBUG, "FEAT : %s\n", buffer));
  return reply_cmd(buffer);
}
int Handle_data::handle_type() {
  ACE_TCHAR buffer[100] = {0};
  ACE_OS::sprintf(buffer, "200 Switching to Binary mode.\r\n");
  ACE_DEBUG((LM_DEBUG, "TYPE : %s\n", buffer));
  return reply_cmd(buffer);
}

// 被动模式命令
int Handle_data::handle_pasv() {
  ACE_TCHAR buffer[100] = {0};
  ACE_OS::sprintf(buffer, "227 Entering Passive Mode (127,0,0,1,0,0)\r\n");
  ACE_DEBUG((LM_DEBUG, "PASV : %s\n", buffer));
  return reply_cmd(buffer);
}

int Handle_data::handle_epsv() {
  Data_send *data_send = 0;
  ACE_NEW_RETURN(data_send, Data_send(reactor()), -1);
  set_data_send(data_send);
  ACE_INET_Addr listenAddr(ACE_LOCALHOST, 0); // 使用 localhost 和随机端口
  if (acceptor_.open(listenAddr) == -1) {
    ACE_DEBUG((LM_DEBUG, "accept open error\n"));
    return -1;
  }

  reactor()->register_handler(this, ACE_Event_Handler::ACCEPT_MASK);
  ACE_DEBUG((LM_DEBUG, "accept event register\n"));

  ACE_TCHAR buf[512] = {0};
  ACE_INET_Addr localAddr;
  auto port = acceptor_.get_local_addr(localAddr);
  ACE_DEBUG((LM_DEBUG, "port is : %d\n", port));
  ACE_OS::sprintf(buf, "229 Entering Extended Passive Mode (|||%d|)\r\n",
                  localAddr.get_port_number());
  ACE_DEBUG((LM_DEBUG, "EPSV : %s\n", buf));

  int ret = reply_cmd(buf);

  if (acceptor_.accept(data_send->get_peer()) == -1) {
    ACE_DEBUG((LM_DEBUG, "data_send input accept error!\n"));
    data_send->handle_close();
    return -1;
  } else if (data_send->open() == -1) {
    ACE_DEBUG((LM_DEBUG, "data_send input open error!\n"));
    data_send->handle_close();
    return -1;
  }

  return ret;
}

int Handle_data::handle_list() {
  ACE_TCHAR buf[512] = {0};
  ACE_OS::sprintf(buf, "150 Here comes the directory listing.\r\n");
  ACE_DEBUG((LM_DEBUG, "LIST : %s.\n", buf));
  if (reply_cmd(buf) == -1) {
    return -1;
  }
  if (data_send_->sendDetailedDirectoryListing() == 0) {
    bzero(buf, std::size(buf));
    ACE_OS::sprintf(buf, "226 Directory send OK.\r\n");
    if (reply_cmd(buf) == -1) {
      return -1;
    }
    ACE_DEBUG((LM_DEBUG, "LIST : %s", buf));
  }

  return 0;
}

int Handle_data::handle_cwd(const std::string &path) {
  ACE_TCHAR buffer[100] = {0};
  getcwd(buffer, sizeof(buffer));
  std::string path_tmp = buffer;
  bzero(buffer, 100);
  getcwd(buffer, 100);
  std::string dir = std::string(buffer) + "/" + path;
  bzero(buffer, 100);
  if (ACE_OS::chdir(dir.c_str()) == -1) {
    ACE_OS::sprintf(buffer, "550 Failed to change directory.\r\n");
    ACE_DEBUG((LM_DEBUG, "CWD : %s\n", buffer));
    return reply_cmd(buffer);
  } else {
    bzero(buffer, 100);
    ACE_OS::sprintf(buffer, "250 Directory successfully changed.\r\n");
    ACE_DEBUG((LM_DEBUG, "CWD : %s\n", buffer));
    return reply_cmd(buffer);
  }
}

int Handle_data::handle_quit() {
  ACE_TCHAR buffer[100] = {0};
  ACE_OS::sprintf(buffer, "221 Goodbye.\r\n");
  ACE_DEBUG((LM_DEBUG, "QUIT : %s\n", buffer));
  return reply_cmd(buffer);
}

int Handle_data::handle_size(const std::string &file_name) {
  ACE_TCHAR buffer[100] = {0};
  ACE_stat stat;
  if (ACE_OS::stat(file_name.c_str(), &stat) == -1) {
    ACE_OS::sprintf(buffer, "550 Could not get file size.\r\n");
    return reply_cmd(buffer);
  } else {
    ACE_OS::sprintf(buffer, "213 %ld\r\n", stat.st_size);
    return reply_cmd(buffer);
  }
}

// 客户端发送RETR filename命令，服务器端接收到后，先发送150 Opening BINARY mode
// data connection for filename (xxx bytes).，然后再发送文件内容，最后发送226
// Transfer
// complete.，表示文件传输完成。
int Handle_data::handle_retr(const std::string &file_name) {
  ACE_DEBUG((LM_DEBUG, "file_name : %s\n", file_name.c_str()));
  ACE_TCHAR buffer[100] = {0};
  ACE_OS::sprintf(buffer, "150 Opening BINARY mode data connection for %s.\r\n",
                  file_name.c_str());
  ACE_DEBUG((LM_DEBUG, "RETR : %s\n", buffer));
  if (reply_cmd(buffer) == -1) {
    return -1;
  }
  if (data_send_->send_file(file_name) == 0) {
    bzero(buffer, std::size(buffer));
    ACE_OS::sprintf(buffer, "226 Transfer complete.\r\n");
    if (reply_cmd(buffer) == -1) {
      return -1;
    }
    ACE_DEBUG((LM_DEBUG, "RETR : %s", buffer));
  }

  return 0;
}

int Handle_data::handle_stor(const std::string &file_name) {
  ACE_TCHAR buffer[100] = {0};
  ACE_OS::sprintf(buffer, "150 Ok to send data.\r\n");
  ACE_DEBUG((LM_DEBUG, "STOR : %s\n", buffer));
  if (reply_cmd(buffer) == -1) {
    return -1;
  }
  if (data_send_->recv_file(file_name) == 0) {
    bzero(buffer, std::size(buffer));
    ACE_OS::sprintf(buffer, "226 Transfer complete.\r\n");
    if (reply_cmd(buffer) == -1) {
      return -1;
    }
    ACE_DEBUG((LM_DEBUG, "STOR : %s", buffer));
  }

  return 0;
}

int Handle_data::handle_mkd(const std::string &file_name) {
  ACE_TCHAR buffer[100] = {0};
  if (ACE_OS::mkdir(file_name.c_str()) == -1) {
    ACE_OS::sprintf(buffer, "550 Failed to create directory.\r\n");
    ACE_DEBUG((LM_DEBUG, "MKD : %s\n", buffer));
    return reply_cmd(buffer);
  } else {
    ACE_OS::sprintf(buffer, "257 Directory successfully created.\r\n");
    ACE_DEBUG((LM_DEBUG, "MKD : %s\n", buffer));
    return reply_cmd(buffer);
  }
}
int Handle_data::handle_rmd(const std::string &file_name) {
  ACE_TCHAR buffer[100] = {0};
  if (ACE_OS::rmdir(file_name.c_str()) == -1) {
    ACE_OS::sprintf(buffer, "550 Failed to remove directory.\r\n");
    ACE_DEBUG((LM_DEBUG, "RMD : %s\n", buffer));
    return reply_cmd(buffer);
  } else {
    ACE_OS::sprintf(buffer, "250 Directory successfully removed.\r\n");
    ACE_DEBUG((LM_DEBUG, "RMD : %s\n", buffer));
    return reply_cmd(buffer);
  }
}

// ftp客户端删除文件时，先发送DELE filename命令，服务器端接收到后，先发送250
// Delete
// operation successful.，然后再发送226 Transfer complete.，表示文件删除成功。
int Handle_data::handle_dele(const std::string &file_name) {
  if (remove(file_name.c_str()) == 0) {
    ACE_TCHAR buffer[100] = {0};
    ACE_OS::sprintf(buffer, "250 Delete operation successful.\r\n");
    ACE_DEBUG((LM_DEBUG, "DELE : %s\n", buffer));
    return reply_cmd(buffer);
  } else {
    ACE_TCHAR buffer[100] = {0};
    ACE_OS::sprintf(buffer, "550 Delete operation failed.\r\n");
    ACE_DEBUG((LM_DEBUG, "DELE : %s\n", buffer));
    return reply_cmd(buffer);
  }
}