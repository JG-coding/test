#include "data_send.h"
#include <cstdio>
#include <string>

ACE_INT32 Data_send::open() {
  ACE_INT32 ret = 0;
  ACE_INET_Addr remote_addr;
  get_peer().get_remote_addr(remote_addr);
  ACE_DEBUG((LM_DEBUG, "the remote addr is %s\n", remote_addr.get_host_addr()));
  ret = reactor()->register_handler(this, ACE_Event_Handler::READ_MASK);
  if (ret != -1) {
    ACE_DEBUG((LM_DEBUG, "handle data register ok!\n"));
  }
  return ret;
}

ACE_INT32 Data_send::handle_close(ACE_HANDLE, ACE_Reactor_Mask) {
  get_peer().close();
  ACE_DEBUG((LM_DEBUG, "Data_send close.\n"));
  delete this;
  return 0;
}

ACE_INT32 Data_send::handle_input(ACE_HANDLE) {
  ACE_TCHAR buf[512] = {0};
  ACE_INT32 len;
  len = get_peer().recv(buf, 512);
  ACE_DEBUG((LM_DEBUG, "trigger read event\n"));
  if (len > 0) {
    ACE_DEBUG((LM_DEBUG, "recv data is %s\n", buf));
    get_peer().send(buf, len);
  } else {
    ACE_DEBUG((LM_DEBUG, "recv data is %s\n", buf));
    get_peer().close();
    delete this;
    return -1;
  }

  return 0;
}

std::string Data_send::getFilePermissions(mode_t mode) {
  std::string permissions;
  permissions += (S_ISDIR(mode)) ? "d" : "-";
  permissions += (mode & S_IRUSR) ? "r" : "-";
  permissions += (mode & S_IWUSR) ? "w" : "-";
  permissions += (mode & S_IXUSR) ? "x" : "-";
  permissions += (mode & S_IRGRP) ? "r" : "-";
  permissions += (mode & S_IWGRP) ? "w" : "-";
  permissions += (mode & S_IXGRP) ? "x" : "-";
  permissions += (mode & S_IROTH) ? "r" : "-";
  permissions += (mode & S_IWOTH) ? "w" : "-";
  permissions += (mode & S_IXOTH) ? "x" : "-";

  return permissions;
}

std::string Data_send::formatTime(time_t t) {
  char buffer[80];
  struct tm *timeinfo = localtime(&t);
  strftime(buffer, sizeof(buffer), "%b %d %H:%M", timeinfo);
  return buffer;
}

// 接收ftp客户端发来的文件
int Data_send::recv_file(const std::string &filename) {
  ACE_HANDLE file = ACE_OS::open(filename.c_str(), O_WRONLY | O_CREAT);
  if (file == ACE_INVALID_HANDLE) {
    const char *errorMsg = "Failed to open file";
    peer_.send_n(errorMsg, strlen(errorMsg));
    return -1;
  }

  char buffer[1024];
  ssize_t bytesRead = 0;
  while ((bytesRead = peer_.recv(buffer, sizeof(buffer))) > 0) {
    ACE_OS::write(file, buffer, bytesRead);
  }
  ACE_OS::close(file);
  peer_.close();
  return 0;
}

int Data_send::send_file(const std::string &filename) {
  ACE_HANDLE file = ACE_OS::open(filename.c_str(), O_RDONLY);
  if (file == ACE_INVALID_HANDLE) {
    const char *errorMsg = "Failed to open file";
    peer_.send_n(errorMsg, strlen(errorMsg));
    return -1;
  }

  char buffer[1024];
  ssize_t bytesRead = 0;
  while ((bytesRead = ACE_OS::read(file, buffer, sizeof(buffer))) > 0) {
    peer_.send_n(buffer, bytesRead);
  }
  ACE_OS::close(file);
  peer_.close();
  return 0;
}

int Data_send::sendDetailedDirectoryListing() {
  DIR *dir;
  struct dirent *ent;
  ACE_TCHAR buffer[1024];
  ACE_OS::getcwd(buffer, sizeof(buffer));
  if ((dir = opendir(buffer)) != NULL) {
    std::vector<std::string> entries;

    while ((ent = readdir(dir)) != NULL) {
      ACE_TCHAR buf[512];
      struct stat fileStat;
      std::string filename = ent->d_name;

      // Skip the current directory and parent directory entries
      if (filename == "." || filename == "..") {
        continue;
      }
      // Skip hidden files
      if (filename[0] == '.') {
        continue;
      }

      if (stat(filename.c_str(), &fileStat) == 0) {
        std::string entry;
        sprintf(buf, "%10s  ", getFilePermissions(fileStat.st_mode).c_str());
        entry += buf;
        sprintf(buf, "%4s  ", std::to_string(fileStat.st_nlink).c_str());
        entry += buf;
        sprintf(buf, "%8s  ", std::to_string(fileStat.st_uid).c_str());
        entry += buf;
        sprintf(buf, "%8s  ", std::to_string(fileStat.st_gid).c_str());
        entry += buf;
        sprintf(buf, "%8s  ", std::to_string(fileStat.st_size).c_str());
        entry += buf;
        sprintf(buf, "%12s  ", formatTime(fileStat.st_mtime).c_str());
        entry += buf;
        entry += filename + "\r\n";
        peer_.send_n(entry.c_str(), entry.size());
      }
    }
    closedir(dir);
  } else {
    const char *errorMsg = "Failed to open directory";
    peer_.send_n(errorMsg, strlen(errorMsg));
    return -1;
  }
  peer_.close();
  return 0;
}

bool Data_send::fileExists(const std::string &filename) {
  struct stat buffer;
  return (stat(filename.c_str(), &buffer) == 0);
}