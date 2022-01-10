#include "iostat.h"

bool is_number(char *s)
{
	bool ret;

	for (size_t i = 0; i < strlen(s); i++) {
		if (!isdigit(s[i])) {
			ret = false;
			break;
		}
	}
	return ret;
}

bool is_bdev(char *name)
{
	char buf[64];
	struct stat st;

	sprintf(buf, "/dev/%s", name);

	if (lstat(buf, &st)) {
		DBG("lstat %s failed due to %s\n", buf, strerror(errno));
		return false;
	}
	
	if ((st.st_mode & S_IFMT) != S_IFBLK) {
		DBG("%s is not block device\n", name);
		return false;
	}

	return true;
}

bool is_cgroup(char *name)
{
	STACK_BUF(buf, 128);
	struct stat st;

	sprintf(buf, "%s/%s", BLKIO, name);
	if (lstat(buf, &st)) {
		DBG("lstat %s failed due to %s\n", buf, strerror(errno));
		return false;
	}

	if ((st.st_mode & S_IFMT) != S_IFDIR) {
		DBG("%s is not directory\n", buf);
		return false;
	}

	return true;
}

bool iostat_opened(const char *bdev)
{
	int fd, ret;
	char buf[64];

	sprintf(buf, "/sys/block/%s/queue/qos", bdev);
	fd = open(buf, O_RDONLY);
	if (fd < 0) {
		ERR("open %s failed due to %s\n", buf, strerror(errno));
		return false;
	}

	ret = read(fd, buf, 64);
	if (ret < 0) {
		ERR("read queue qos of %s failed due to %s\n",
				bdev, strerror(errno));
		ret = false;
	} else {
		ret = !!strstr(buf, "[iostat]");
	}
	close(fd);
	return ret;
}

int get_bdev(char *bdev, char *out)
{
	char buf[32];
	int fd, ret;

	sprintf(buf, "/sys/block/%s/dev", bdev);
	fd = open(buf, O_RDONLY);
	if (fd < 0) {
		ret = errno;
		DBG("failed to open %s due to %s\n", buf, strerror(ret));
		return ret;
	}

	ret = read(fd, buf, 32);
	if (ret < 0) {
		ret = errno;
		DBG("failed to read %s due to %s\n", buf, strerror(ret));
		close(fd);
		return ret;
	}

	/*
	 * There is a '\n' in the end of output
	 */
	buf[ret - 1] = 0;
	strcpy(out, buf);
	close(fd);

	return 0;
}

/*
 * Don't use strtok(NULL, ch) as it use a global variable.
 * It would run into error when there are multiple thread invokes it
 */
void str_split(char *buf, const char *ch, std::vector<char *> &v)
{
	char *tmp;
	int off, len;

	len = strlen(buf);
	off = 0;

	tmp = strtok(buf, ch);
	while (tmp) {
		v.push_back(tmp);
		off += strlen(tmp) + 1;
		if (off >= len)
			break;
		tmp = strtok(tmp + strlen(tmp) + 1, ch);
	}
}

struct _unit {
	double v;
	const char *u;
};

static struct _unit bw_units[4] = {
	{1, "B/s"},
	{1 << 10, "KB/s"},
	{1 << 20, "MB/s"},
	{1 << 30, "GB/s"}
};

static struct _unit time_units[4] = {
	{1, "ns"},
	{1000, "us"},
	{1000000, "ms"},
	{1000000000, "s"}
};

static struct _unit iops_units[4] = {
	{1, "/s"},
	{1 << 10, "K/s"},
	{1 << 20, "M/s"},
	{1 << 30, "G/s"}
};

static struct _unit sz_units[4] = {
	{1, "B"},
	{1 << 10, "K"},
	{1 << 20, "M"},
	{1 << 30, "G"}
};

static char *__convert(struct _unit *units, u64 v, char *buf)
{
	int i;

	if (!v) {
		sprintf(buf, "0");
		return buf;
	}

	for (i = 0; i < 4; i++) {
		if (units[i].v > v)
			break;
	}

	i--;

	sprintf(buf, "%6.2f%s", (double)v/units[i].v, units[i].u);
	return buf;
}

char *tobw(u64 bytes, char *buf)
{
	return __convert(bw_units, bytes, buf);
}

char *totime(u64 nsec, char *buf)
{
	return __convert(time_units, nsec, buf);
}

char *toiops(u64 cnt, char *buf)
{
	return __convert(iops_units, cnt, buf);
}

char *tosz(u64 cnt, char *buf)
{
	return __convert(sz_units, cnt, buf);
}

bool is_digits(const char *buf)
{
	std::string str(buf);
	return str.find_first_not_of("0123456789") == std::string::npos;
}

bool parse_iostat_tag(char *tag, char *bn, bool *meta)
{
	char *t, *n;

	for (t = tag, n = bn;
	     *t != '-' && *t != '\0';
	     t++, n++) {
		*n = *t;
	}

	if (*t == '\0')
		return false;

	t++;
	*meta = *t == 'm';

	return true;
}
