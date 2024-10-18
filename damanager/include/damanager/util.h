#ifndef DAMANAGER__UTIL_H
#define DAMANAGER__UTIL_H

struct pollfd;

namespace damanager
{
void init_shutdown_pollfds(int fd, int shutdown_eventds, pollfd* poll_fds);
int poll_or_shutdown(pollfd* poll_fds, int timeout, const char* from);

bool shutdown_manager(int shutdown_eventfd);
} // namespace damanager

#ifndef __FILE_NAME__
#define __FILE_NAME__ __FILE__
#endif

#define xstr(s) str(s)
#define str(s)  #s

#define DAMANAGER_POLL_OR_SHUTDOWN(poll_fds, timeout)                                              \
    damanager::poll_or_shutdown((poll_fds), (timeout), __FILE_NAME__ ":" xstr(__LINE__))
#endif
