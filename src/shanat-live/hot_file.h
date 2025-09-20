#ifndef HOT_FILE_H
#define HOT_FILE_H

#include <mutex>
#include <pthread.h>
#include <stdint.h>
#include <string>

class HotFile
{
  private:
    const std::string fname;
    bool running = true;
    pthread_t thread = 0;
    std::mutex mutex;
    int64_t modif;
    std::string content;

  private:
    static void *watcher_fun(void *arg);
    int64_t get_modif() const;
    void read_content(std::string &content);

  public:
    HotFile(const std::string &fname);
    ~HotFile();
    bool check_update(std::string &content, int64_t &modif);
};

#endif
