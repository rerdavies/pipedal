#include "AuxIn.hpp"
#include <stdexcept>
#include <thread>

#include "ss.hpp"
#include "Finally.hpp"
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <iostream>
#include <atomic>

#include <poll.h>
#include <sys/types.h>

namespace fs = std::filesystem;

namespace pipedal
{

    class AuxInAlsaDeviceImpl : public AuxInAlsaDevice
    {
    public:
        AuxInAlsaDeviceImpl(const std::filesystem::path &path);
        virtual ~AuxInAlsaDeviceImpl() override;
        void Open(const fs::path &path);
        void Close();

    private:
        fs::path path;
        void ThreadProc();

        std::unique_ptr<std::thread> thread;
        int fd = -1;
        int cancelWrite = -1;
        int cancelRead = -1;
        size_t total_read = 0;
    };

    AuxInAlsaDevice::ptr AuxInAlsaDevice::Create(const std::filesystem::path &path)
    {
        return std::make_shared<AuxInAlsaDeviceImpl>(path);
    }

}

using namespace pipedal;

AuxInAlsaDeviceImpl::AuxInAlsaDeviceImpl(const fs::path &path)
{
    this->path = path;
    Open(path);

    this->thread = std::make_unique<std::thread>([this]()
                                                 { ThreadProc(); });
}

void AuxInAlsaDeviceImpl::Close()
{
    if (thread)
    {
        char buff[1];
        int nWritten = write(cancelWrite, buff, 1);
        (void)nWritten; // ignore return.
        thread->join();
        thread = nullptr;
    }
    if (cancelWrite != -1)
    {
        close(cancelWrite);
        cancelWrite = 1;
    }
    if (cancelRead != -1)
    {
        close(cancelRead);
        cancelRead = -1;
    }
    if (fd != -1)
    {
        close(fd);
        fd = -1;
    }
}

AuxInAlsaDeviceImpl::~AuxInAlsaDeviceImpl()
{
    Close();
}

AuxInAlsaDevice::AuxInAlsaDevice()
{
}
AuxInAlsaDevice::~AuxInAlsaDevice()
{
}

static void setNonBlocking(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}
static void setBlocking(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags & (~O_NONBLOCK));
}
void AuxInAlsaDeviceImpl::Open(const fs::path &path)
{
    total_read = 0;

    fd = open(path.c_str(), O_RDONLY | O_NONBLOCK); // non-blocking so we can open it
    if (fd == -1)
    {
        throw std::runtime_error(SS("Can't open file " << path));
    }
    setBlocking(fd); // but use blocking semantics when reading.
    int cancel_pipe[2];
    // create a socket pair to use when cancelling.
    if (pipe(cancel_pipe) == -1)
    {
        throw std::runtime_error("Can't create pipe.");
    }
    this->cancelWrite = cancel_pipe[1];
    this->cancelRead = cancel_pipe[0];
    setNonBlocking(cancelRead);
}

void AuxInAlsaDeviceImpl::ThreadProc()
{
    constexpr size_t BUFFER_SIZE = 65536;
    ssize_t bytes_read;
    int16_t samples[BUFFER_SIZE];

    while (1)
    {
        // Read audio data from the FIFO
        struct pollfd poll_fds[2];
        poll_fds[0].fd = fd;
        poll_fds[0].events = POLLIN;
        poll_fds[1].fd = cancelRead;
        poll_fds[1].events = POLLIN;

        int rc = poll(poll_fds, 2, -1);
        if (rc < 00)
        {
            throw std::runtime_error("select failed.");
        }
        if (poll_fds[1].revents)
        {
            break;
        }
        if (poll_fds[0].revents)
        {
            ssize_t bytes_read = read(fd, &samples, sizeof(samples));
            if (bytes_read < 0)
            {
                throw std::runtime_error("Auxin read failed.");
            }
            if (poll_fds[0].revents & POLLHUP)
            {
                std::cout << " End of stream. " << total_read << std::endl;
                close(fd);
                fd = -1;
                // reopen the fifo.
                total_read = 0;
                fd = open(path.c_str(), O_RDONLY | O_NONBLOCK);
                if (fd == -1)
                {
                    throw std::runtime_error(SS("Can't open file " << path));
                }
                setBlocking(fd);
            }
            else if (bytes_read > 0)
            {
                total_read += bytes_read;
                std::cout << " Read " << bytes_read << std::endl;
                sleep(1);
                // Process the audio data, mix it with your main input
                // Your mixing code here...

                // For example, if main_buffer has your main input:
                // for (int i = 0; i < bytes_read/sizeof(int16_t); i++) {
                //     mixed_buffer[i] = main_buffer[i] + buffer[i];
                // }
            }
        }
    }
}
