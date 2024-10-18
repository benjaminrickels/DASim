#include <memory>
#include <string>

#include <getopt.h>
#include <poll.h>
#include <signal.h>
#include <sys/eventfd.h>
#include <sys/signalfd.h>

#include <damanager/buffer_manager.h>
#include <damanager/client_manager.h>
#include <damanager/demo_buffer_manager.h>
#include <damanager/util.h>
#include <dasim/log.h>

namespace damanager
{
struct args
{
    size_t nr_pages_ram;
    size_t nr_pages_swap;
    bool hugepages;

    std::string dir_ram;
    std::string dir_swap;
    std::string dir_pers;
};

namespace
{
const char* usage_msg = //
    "usage: damanager [--hugepages] [--dir-ram <path>] [--dir-swap <path>]\n"
    "                 [--dir-pers <path>] [--dax-swap] [--dax-pers] [-h|--help]\n"
    "                 <nr_pages_ram> <nr_pages_ram>\n"
    "\n"
    "positional arguments:\n"
    "  <nr_pages_ram>       Number of RAM pages available (must be >= 2)\n"
    "  <nr_pages_swap>      Number of swap pages available (must be >= 0)\n"
    "\n"
    "options:\n"
    "  --dir-ram <path>     The path where the RAM file is located. Should be\n"
    "                       something like \"/dev/shm/\" or \"/hugetlbfs/\".\n"
    "                       Defaults to \"/dev/shm/\".\n"
    "  --dir-swap <path>    The path where the swapfile is stored.\n"
    "                       Defaults to \"/tmp/\".\n"
    "  --dir-pers <path>    The path where persistent files are stored\n"
    "                       Defaults to \"/tmp/\".\n"
    "  -h|--help            Show this message.\n";

bool is_valid_dir(const char* path)
{
    size_t l = strlen(path);
    return l != 0 && path[0] == '/' && path[l - 1] == '/';
}

bool parse_size_t(const char* str, size_t& out)
{
    errno = 0;
    char* endptr;
    out = strtoul(str, &endptr, 10);

    if (errno)
        return false;

    if (endptr == str || *endptr != '\0')
        return false;

    return true;
}

bool parse_args(int argc, char** argv, args& out)
{
    size_t nr_pages_ram  = 0;
    size_t nr_pages_swap = 0;
    const char* dir_ram  = NULL;
    const char* dir_swap = NULL;
    const char* dir_pers = NULL;
    int help             = false;
    int error            = false;

    static const char* shortopts          = "h";
    static const struct option longopts[] = //
        {
            {"dir-ram", required_argument, NULL, 1},
            {"dir-swap", required_argument, NULL, 2},
            {"dir-pers", required_argument, NULL, 3},
            {"help", no_argument, NULL, 'h'},
            /*             */
            {0, 0, 0, 0} /**/
        };

    while (true) {
        int c = getopt_long(argc, argv, shortopts, longopts, NULL);

        if (c == -1)
            break;

        switch (c) {
        case 0:
            break;
        case 1:
            dir_ram = optarg;
            break;
        case 2:
            dir_swap = optarg;
            break;
        case 3:
            dir_pers = optarg;
            break;
        case 'h':
        case '?':
            help = true;
            break;
        default:
            error = true;
            break;
        }
    }

    if (argc - optind < 2) {
        fprintf(stderr, "missing positional argument(s)\n");
        error = true;
    }
    else {
        if (!parse_size_t(argv[optind], nr_pages_ram) || nr_pages_ram < 2) {
            fprintf(stderr, "\"nr_pages_ram\" invalid\n");
            error = true;
        }

        if (!parse_size_t(argv[optind + 1], nr_pages_swap)) {
            fprintf(stderr, "\"nr_pages_swap\" invalid\n");
            error = true;
        }
    }

    if (!dir_ram)
        dir_ram = "/dev/shm/";

    if (!dir_swap)
        dir_swap = "/tmp/";

    if (!dir_pers)
        dir_pers = "/tmp/";

    if (!is_valid_dir(dir_ram) || !is_valid_dir(dir_swap) || !is_valid_dir(dir_pers)) {
        fprintf(stderr, "invalid direcory/directories specified\n");
        error = true;
    }

    if (help || error) {
        fprintf((error ? stderr : stdout), "%s%s", (error ? "\n" : ""), usage_msg);
        return false;
    }

    out.nr_pages_ram  = nr_pages_ram;
    out.nr_pages_swap = nr_pages_swap;

    out.dir_ram  = dir_ram;
    out.dir_swap = dir_swap;
    out.dir_pers = dir_pers;

    return true;
}

int create_sigint_signalfd()
{
    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGINT);
    sigaddset(&mask, SIGTERM);

    if (sigprocmask(SIG_BLOCK, &mask, NULL) == -1) {
        DASIM_FATAL("sigprocmask() failed: %s", POSIX_ERRSTR);
        return -1;
    }

    int sigfd = signalfd(-1, &mask, 0);
    if (sigfd == -1) {
        DASIM_FATAL("signalfd() failed: %s", POSIX_ERRSTR);
    }

    return sigfd;
}
} // namespace
} // namespace damanager

int main(int argc, char** argv)
{
    damanager::args args;
    if (!damanager::parse_args(argc, argv, args))
        return EXIT_FAILURE;

    int shutdown_eventfd;
    if ((shutdown_eventfd = eventfd(0, EFD_NONBLOCK | EFD_SEMAPHORE)) < 0) {
        DASIM_FATAL("eventfd() failed: %s", POSIX_ERRSTR);
    }
    int sigint_fd = damanager::create_sigint_signalfd();

    pollfd poll_fds[2];
    damanager::init_shutdown_pollfds(sigint_fd, shutdown_eventfd, poll_fds);

    /* extra scope where the client_manger can go out of scope but the fds remain valid */
    {
        std::unique_ptr<damanager::buffer_manager> buffer_manager =
            std::make_unique<damanager::demo_buffer_manager>(
                args.nr_pages_ram, args.nr_pages_swap, args.dir_ram, args.dir_swap, args.dir_pers);
        damanager::client_manager manager(shutdown_eventfd, std::move(buffer_manager));

        DAMANAGER_POLL_OR_SHUTDOWN(poll_fds, -1);
        damanager::shutdown_manager(shutdown_eventfd);
    }
    close(sigint_fd);
    close(shutdown_eventfd);

    DASIM_INFO("exit");

    return 0;
}
