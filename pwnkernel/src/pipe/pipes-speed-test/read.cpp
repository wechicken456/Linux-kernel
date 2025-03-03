#include "common.hpp"

static struct timespec tspec;

static double get_millis()
{
    if (clock_gettime(CLOCK_REALTIME, &tspec) < 0)
    {
        fail("could not get time: %s", strerror(errno));
    }
    return ((double)tspec.tv_sec) * 1000.0 + ((double)tspec.tv_nsec) / 1000000.0;
}

NOINLINE
static size_t with_read(const Options &options, char *buf)
{
    if (options.busy_loop)
    {
        if (fcntl(STDIN_FILENO, F_SETFL, O_NONBLOCK) < 0)
        {
            fail("could not mark stdout pipe as non blocking: %s", strerror(errno));
        }
    }
    struct pollfd pollfd;
    pollfd.fd = STDOUT_FILENO;
    pollfd.events = POLLIN | POLLPRI;
    size_t read_count = 0;
    while (read_count < options.bytes_to_pipe)
    {
        if (options.poll && options.busy_loop)
        {
            while (poll(&pollfd, 1, 0) == 0)
            {
            }
        }
        else if (options.poll)
        {
            poll(&pollfd, 1, -1);
        }
        ssize_t ret = read(STDIN_FILENO, buf, options.buf_size);
        if (ret < 0 && errno == EAGAIN)
        {
            continue;
        }
        if (ret < 0)
        {
            fail("read failed: %s", strerror(errno));
        }
        read_count += ret;
    }
    return read_count;
}

NOINLINE
static size_t with_splice(const Options &options)
{
    struct pollfd pollfd;
    pollfd.fd = STDOUT_FILENO;
    pollfd.events = POLLIN | POLLPRI;
    size_t read_count = 0;
    int devnull = open("/dev/null", O_WRONLY);
    while (read_count < options.bytes_to_pipe)
    {
        if (options.poll && options.busy_loop)
        {
            while (poll(&pollfd, 1, 0) == 0)
            {
            }
        }
        else if (options.poll)
        {
            poll(&pollfd, 1, -1);
        }
        ssize_t ret = splice(
            STDIN_FILENO, NULL, devnull, NULL, options.buf_size,
            (options.busy_loop ? SPLICE_F_NONBLOCK : 0) | (options.gift ? SPLICE_F_MOVE : 0));
        if (ret < 0 && errno == EAGAIN)
        {
            continue;
        }
        if (ret < 0)
        {
            fail("splice failed: %s", strerror(errno));
        }
        read_count += ret;
    }
    close(devnull);
    return read_count;
}

int main(int argc, char **argv)
{
    Options options;
    parse_options(argc, argv, options);

    char bytes_to_pipe_str[128];
    write_size_str(options.bytes_to_pipe, bytes_to_pipe_str);
    log("will read %s\n", bytes_to_pipe_str);

    double t0 = get_millis();
    size_t read_count;
    if (options.read_with_splice)
    {
        read_count = with_splice(options);
    }
    else
    {
        char *buf = allocate_buf(options);
        read_count = with_read(options, buf);
    }
    double t1 = get_millis();
    double gigabytes_per_second = (((double)read_count) / 1000000) / (t1 - t0);
    // `pv` uses GiB, not GB
    double gibibytes_per_second = gigabytes_per_second * 0.931323;
    if (options.csv)
    {
        printf(
            "%f,%zu,%zu,%zu,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\n",
            gibibytes_per_second,
            options.bytes_to_pipe,
            options.buf_size,
            options.pipe_size,
            options.busy_loop,
            options.poll,
            options.huge_page,
            options.check_huge_page,
            options.write_with_vmsplice,
            options.read_with_splice,
            options.gift,
            options.lock_memory,
            options.dont_touch_pages,
            options.same_buffer);
    }
    else
    {
        char buf_size_str[128];
        write_size_str(options.buf_size, buf_size_str);
        printf(
            "%.1fGiB/s, %s buffer, %zu iterations (%s piped)\n",
            gibibytes_per_second,
            buf_size_str,
            options.bytes_to_pipe / options.buf_size,
            bytes_to_pipe_str);
    }

    return 0;
}
