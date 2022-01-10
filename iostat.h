#ifndef __IOSTAT_H__
#define __IOSTAT_H__

#define _CXX0X_WARNING_H 1

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <getopt.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <assert.h>
#include <sys/stat.h>
#include <dirent.h>
#include <ctype.h>
#include <signal.h>
#include <time.h>

#include <set>
#include <string>
#include <map>
#include <vector>
#include <algorithm>
#include <list>
#include <mutex>
#include <thread>
#include <atomic>
#include <climits>
#include <condition_variable>

enum {
	READ = 0,
	WRITE,
	DISCARD,
	OP_MAX,
};

typedef unsigned long u64;
typedef unsigned int u32;
typedef unsigned short u16;
typedef unsigned char u8;

#include "utils.h"
#include "stat.h"

extern int debug_open;

#define DBG(fmt, ...) \
do { \
	if (debug_open) \
		fprintf(stdout, "%s %d: " fmt, __func__, __LINE__, ##__VA_ARGS__); \
} while (0)

#define INFO(fmt, ...) fprintf(stdout, "%s %d: " fmt, __func__, __LINE__, ##__VA_ARGS__)
#define ERR(fmt, ...) fprintf(stderr, "%s %d: " fmt, __func__, __LINE__, ##__VA_ARGS__)
#define IOSTAT "blkio.iostat"
#define UNUSED(x) UNUSED_ ## x __attribute__((unused))
#define BLKIO "/sys/fs/cgroup/blkio"
#define BLKIO_LEN 20

enum {
	SORT_BW = 0,
	SORT_IOPS,
	SORT_QUEUE_LAT,
	SORT_DEV_LAT
};

enum {
	FORMAT_READ = 1 << READ,
	FORMAT_WRITE = 1 << WRITE,
	FORMAT_DISCARD = 1 << DISCARD,
	FORMAT_DATA = 1 << (DISCARD + 1),
	FORMAT_META = 1 << (DISCARD + 2)
};

class Conf {
public:
	std::list<std::pair<std::string, std::string>> cgdir_list;
	int interval;
	int sort_factor;
	int max_line;
	int format;
	bool stop;

	explicit Conf(void)
	{
		interval = 1;
		sort_factor = SORT_BW;
		stop = false;
		max_line = 8;
		format = FORMAT_READ | FORMAT_WRITE | FORMAT_DATA;
	}

	~Conf(void)
	{
		cgdir_list.clear();
	}
	bool bdev_is_needed(const char *bdev);
	bool parse_options(int argc, char *argv[]);
	void help(void);
	bool check(void);
private:
	/* block device name, major:minor */
	std::map<std::string, std::string> _bdev_map;
};

extern class Conf Conf;

#endif
