#include "hot_file.h"

#include <sys/stat.h>
#include <unistd.h>

HotFile::HotFile(const std::string &fname)
    : fname(fname)
{
    modif = get_modif();
    read_content(content);

    if (pthread_create(&thread, nullptr, watcher_fun, this) != 0)
    {
        fprintf(stderr, "Failed to start hotfile thread for '%s'\n", fname.c_str());
        exit(-1);
    }
}

HotFile::~HotFile()
{
    running = false;
    struct timespec timeout;
    clock_gettime(CLOCK_REALTIME, &timeout);
    timeout.tv_nsec += 100000000; // 100 msec
    int ret = pthread_timedjoin_np(thread, nullptr, &timeout);
    if (ret == ETIMEDOUT)
    {
        fprintf(stderr, "Hotfile thread failed to exit\n");
        exit(-1);
    }
}

void *HotFile::watcher_fun(void *arg)
{
    HotFile &hf = *(HotFile *)arg;

    std::string content;
    int64_t last_modif;
    {
        std::lock_guard<std::mutex> lock(hf.mutex);
        last_modif = hf.modif;
    }

    while (hf.running)
    {
        int64_t modif = hf.get_modif();
        if (modif != last_modif)
        {
            hf.read_content(content);
            last_modif = modif;
            std::lock_guard<std::mutex> lock(hf.mutex);
            hf.content.assign(content);
            hf.modif = modif;
        }
        // 100 msec
        usleep(100000);
    }
    return nullptr;
}

void HotFile::read_content(std::string &content)
{
    content.clear();
    FILE *file = fopen(fname.c_str(), "rb");
    if (!file) return;

    fseek(file, 0, SEEK_END);
    const size_t size = ftell(file);
    fseek(file, 0, SEEK_SET);

    content.resize(size + 1, '\0');
    fread(&content[0], 1, size, file);
    fclose(file);
}

int64_t HotFile::get_modif() const
{
    struct stat file_stat;
    if (stat(fname.c_str(), &file_stat) != 0)
        return 0;

    int64_t msec = file_stat.st_mtim.tv_nsec / 1000000;
    return file_stat.st_mtim.tv_sec * 1000 + msec;
}

bool HotFile::check_update(std::string &content, int64_t &modif)
{
    std::lock_guard<std::mutex> lock(mutex);
    if (modif == this->modif) return false;
    modif = this->modif;
    content.assign(this->content);
    return true;
}
