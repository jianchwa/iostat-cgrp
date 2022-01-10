#include "iostat.h"

int debug_open = 0;

class Conf Conf;
class StatRun StatRun;

bool Conf::bdev_is_needed(const char *bdev)
{
	std::string bn(bdev);
	if (!_bdev_map.empty()) {
		std::map<std::string, std::string>::iterator it;
		it = _bdev_map.find(bdev);
		return it != _bdev_map.end();
	}

	return true;
}

void Conf::help(void)
{
	printf("iostat-cgrp -[options] [interval] bdev... cgroup...\n");
	printf("-h: show help page\n");
	printf("-r: show stats of root cgroup\n");
	printf("-l: max line of output\n");
	printf("-o: output data (r: read w: write d: discard D: data M: meta, 'rwD' by default)\n");
	printf("-s: factor of sorting (bw iops queue_lat dev_lat, bw by default)\n");
}

static int parse_format(char *arg)
{
	int f = 0;

	for (; *arg; arg++) {
		switch (*arg) {
		case 'r':
			f |= FORMAT_READ;
			break;
		case 'w':
			f |= FORMAT_WRITE;
			break;
		case 'd':
			f |= FORMAT_DISCARD;
			break;
		case 'D':
			f |= FORMAT_DATA;
			break;
		case 'M':
			f |= FORMAT_META;
			break;
		default:
			break;
		}
	}

	return f;
}

static int parse_sort_factor(char *arg)
{
	int ret = SORT_BW;

	if (!strcmp(arg, "bw"))
		ret = SORT_BW;
	else if(!strcmp(arg, "iops"))
		ret = SORT_IOPS;
	else if (!strcmp(arg, "queue_lat"))
		ret = SORT_QUEUE_LAT;
	else if (!strcmp(arg, "dev_lat"))
		ret = SORT_DEV_LAT;

	return ret;
}

bool Conf::parse_options(int argc, char *argv[])
{
	bool ret = true;
	char *myarg;
	char buf[32];
	int op;

	while(1) {
		op = getopt(argc, argv, "hs:vl:o:r");
		if (op == -1)
			break;

		switch (op) {
		case 's':
			this->sort_factor = parse_sort_factor(optarg);
			break;
		case 'l':
			this->max_line = atoi(optarg);
			break;
		case 'o':
			this->format = parse_format(optarg);
			break;
		case 'v':
			debug_open = 1;
			break;
		case 'r':
			cgdir_list.push_back(std::pair<std::string, std::string>(std::string(""), std::string("")));
			break;
		case 'h':
		default:
			ret = false;
			break;
		}
	}

	if (!ret) {
		help();
		return ret;
	}

	while (optind < argc) {
		myarg = argv[optind++];
		if (is_number(myarg)) {
			this->interval = atoi(myarg) ?: 1;
		} else if (is_bdev(myarg) && !get_bdev(myarg, buf)) {
			_bdev_map.insert(std::pair<std::string, std::string>(std::string(myarg), std::string(buf)));
		} else if (is_cgroup(myarg)) {
			cgdir_list.push_back(std::pair<std::string, std::string>(std::string(myarg), std::string("")));
		}
	}

	return check();
}

bool Conf::check(void)
{
	std::map<std::string, std::string>::iterator it;
	bool ret = true;

	if (cgdir_list.empty()) {
		ERR("Please provide valid cgroup directory\n");
		ret = false;
	}

	if (_bdev_map.empty()) {
		ERR("Please provide one bdev at least\n");
		ret = false;
	}

	for (it = _bdev_map.begin(); it != _bdev_map.end(); it++) {
		if (!iostat_opened(it->first.c_str())) {
			ERR("Please open iostat of %s\n", it->first.c_str());
			ret = false;
		}
	}

	if (!(this->format & (FORMAT_READ | FORMAT_WRITE | FORMAT_WRITE))) {
		ERR("Please provide one of 'rwd'\n");
		ret = false;
	}

	if (!(this->format & (FORMAT_DATA | FORMAT_META))) {
		ERR("Please provide one of 'DM'\n");
		ret = false;
	}

	return ret;
}

static void sig_cgroup(int UNUSED(signum),
		siginfo_t *UNUSED(info), void *UNUSED(priv))
{
	Conf.stop = true;
}

static void catch_signals(void (*act_fn)(int, siginfo_t*, void *))
{
	static struct sigaction act;

	memset(&act, 0, sizeof(act));

	act.sa_sigaction = act_fn;
	act.sa_flags = SA_SIGINFO;

	sigaction(SIGTERM, &act, NULL);
	sigaction(SIGINT, &act, NULL);
	sigaction(SIGALRM, &act, NULL);
}

static int cgroup(int expire)
{
	std::list<std::pair<std::string, std::string>>::iterator it;

	/*
	 * Most of time, the process is killed by SIGTERM
	 */
	while (!Conf.stop && expire) {
		expire--;
		sleep(Conf.interval);
		for (it = Conf.cgdir_list.begin(); it != Conf.cgdir_list.end(); ++it) {
			StatRun.process(it->first);
		}
		StatRun.sort();
		StatRun.output();
	}
	return 0;
}

int main(int argc, char *argv[])
{
	if (!Conf.parse_options(argc, argv))
		return -EINVAL;

	catch_signals(sig_cgroup);

	return cgroup(INT_MAX);
}
