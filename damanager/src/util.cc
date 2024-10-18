#include <damanager/util.h>

#include <cerrno>

#include <poll.h>
#include <unistd.h>

#include <dasim/log.h>

namespace damanager
{
void init_shutdown_pollfds(int fd, int shutdown_eventfs, pollfd* poll_fds)
{
    poll_fds[0].fd     = fd;
    poll_fds[0].events = POLLIN;
    poll_fds[1].fd     = shutdown_eventfs;
    poll_fds[1].events = POLLIN;
}

int poll_or_shutdown(pollfd* poll_fds, int timeout, const char* from)
{
    int ret;
    do {
        ret = poll(poll_fds, 2, -1);
        if (ret == -1) {
            if (errno == EINTR)
                continue;

            DASIM_ERROR("poll() failed: %s  (from %s)", POSIX_ERRSTR, from);
            return -1;
        }
        if (ret == 0) {
            if (timeout < 0)
                continue;
            return 0;
        }
        break;
    } while (1);

    short revents;

    if ((revents = poll_fds[1].revents)) {
        if (revents & ~POLLIN) {
            DASIM_ERROR("shutdown_eventfd invalid event (from %s)", from);
            return -1;
        }

        DASIM_INFO("shutdown_eventfd ready (from %s)", from);
        return -2;
    }

    revents = poll_fds[0].revents;
    if (revents & ~POLLIN) {
        DASIM_WARNING("main fd invalid event (from %s)", from);
        return -1;
    }

    DASIM_TRACE("main fd ready (from %s)", from);
    return 1;
}

bool shutdown_manager(int shutdown_eventfd)
{
    int one = 1;
    return write(shutdown_eventfd, &one, sizeof(one)) == sizeof(one);
}
} // namespace damanager
