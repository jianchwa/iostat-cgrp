#include "iostat.h"

BlkData::BlkData(void)
{
	memset(_bytes, 0, sizeof(u64) * OP_MAX);
	memset(_ios, 0, sizeof(u64) * OP_MAX);
	memset(_queue_lat, 0, sizeof(u64) * OP_MAX);
	memset(_dev_lat, 0, sizeof(u64) * OP_MAX);
}

void BlkData::operator=(const char *buf)
{
	/*
	 * bytes iops queue_lat dev_lat [ditto]  [ditto]
	 * \___________ ______________/    |        |
	 *             v                   v        v
	 *            read               write   discard
	 */
	sscanf(buf, "%lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu\n",
			&_bytes[READ], &_ios[READ], &_queue_lat[READ], &_dev_lat[READ],
			&_bytes[WRITE], &_ios[WRITE], &_queue_lat[WRITE], &_dev_lat[WRITE],
			&_bytes[DISCARD], &_ios[DISCARD], &_queue_lat[DISCARD], &_dev_lat[DISCARD]
			);
}

void BlkData::operator=(const BlkData &a)
{
	memcpy(&_bytes[0], &a._bytes[0], OP_MAX * sizeof(u64));
	memcpy(&_ios[0], &a._ios[0], OP_MAX * sizeof(u64));
	memcpy(&_queue_lat[0], &a._queue_lat[0], OP_MAX * sizeof(u64));
	memcpy(&_dev_lat[0], &a._dev_lat[0], OP_MAX * sizeof(u64));
}

BlkData BlkData::operator-(const BlkData &a)
{
	BlkData r;
	int i;

	for (i = 0; i < OP_MAX; i++) {
		r._bytes[i] = _bytes[i] - a._bytes[i];
		r._ios[i] = _ios[i] - a._ios[i];
		r._queue_lat[i] = _queue_lat[i] - a._queue_lat[i];
		r._dev_lat[i] = _dev_lat[i] - a._dev_lat[i];
	}

	return r;
}

bool BlkData::operator>(const BlkData &a)
{
	bool ret = false;
	bool r = Conf.format & FORMAT_READ;
	bool w = Conf.format & FORMAT_DISCARD;

	switch (Conf.sort_factor) {
	case SORT_BW:
		if (r && (_bytes[READ] || a._bytes[READ]))
			ret = _bytes[READ] > a._bytes[READ];
		else if (w && (_bytes[WRITE] || a._bytes[WRITE]))
			ret = _bytes[WRITE] > a._bytes[WRITE];
		break;
	case SORT_IOPS:
		if (r && ( _ios[READ] || a._ios[READ]))
			ret = _ios[READ] > a._ios[READ];
		else if (w && (_ios[WRITE] || a._ios[WRITE]))
			ret = _ios[WRITE] > a._ios[WRITE];
		break;
	case SORT_QUEUE_LAT:
		if (r && (_ios[READ] || a._ios[READ]))
			ret = _queue_lat[READ]/(_ios[READ] ?: 1) > a._queue_lat[READ]/(a._ios[READ] ?: 1);
		else if (w && (_ios[WRITE] || a._ios[WRITE]))
			ret = _queue_lat[WRITE]/(_ios[WRITE] ?: 1) > a._queue_lat[WRITE]/(a._ios[WRITE] ?: 1);
		break;
	case SORT_DEV_LAT:
		if (r && (_ios[READ] || a._ios[READ]))
			ret = _dev_lat[READ]/(_ios[READ] ?: 1) > a._dev_lat[READ]/(a._ios[READ] ?: 1);
		else if (w && (_ios[WRITE] || a._ios[WRITE]))
			ret = _dev_lat[WRITE]/(_ios[WRITE] ?: 1) > a._dev_lat[WRITE]/(a._ios[WRITE] ?: 1);
		break;
	default:
		break;
	}

	return ret;
}

int BlkData::header(char *buf, const char *type)
{
	return sprintf(buf, "%4s %10s %10s %10s %10s %10s",
			type, "IOPS", "BW", "RQSZ", "QLAT", "DLAT");
}

bool BlkData::empty(int op)
{
	return !_ios[op];
}

int BlkData::show(char *buf, int op)
{
	int cnt, intv;
	char tmp[128];

	/* intv is in seconds */
	intv = Conf.interval;
	/* IOPS */
	cnt = sprintf(buf, "%10s ", toiops(_ios[op] / intv, tmp));
	/* Bandwidth */
	cnt += sprintf(buf + cnt, "%10s ", tobw(_bytes[op] / intv, tmp));
	/* IO average size */
	cnt += sprintf(buf + cnt, "%10s ", tosz(_bytes[op] / (_ios[op] ?: 1), tmp));
	/* Queue Latency */
	cnt += sprintf(buf + cnt, "%10s ", totime(_queue_lat[op] / (_ios[op] ?: 1), tmp));
	/* Device Latency */
	cnt += sprintf(buf + cnt, "%10s ", totime(_dev_lat[op] / (_ios[op] ?: 1), tmp));

	return cnt;
}

template <class T>
Stat<T>::Stat(std::string &cn, char *tag, char *bn)
{
	_cn = cn;
	_tag.assign(tag);
	_bn.assign(bn);

	_index = 0;
}

template <class T>
Stat<T>::~Stat(void)
{
}

template <class T>
void Stat<T>::input(char *buf)
{
	_stat[_index % 2] = buf;

	if (_index > 0) {
		_delta = _stat[_index % 2] - _stat[(_index - 1) % 2];
	}

	_index++;
}

template <class T>
void Stat<T>::output(char *buf)
{
	static const char *op_str[] = {"R", "W", "D"};
	const char *bn = _bn.c_str();
	const char *cn = _cn.c_str();
	int cnt = 0, op;

	for (op = READ; op <= DISCARD; op++) {
		if (!(Conf.format & (1 << op)) || _delta.empty(op))
			continue;
	
		cnt += sprintf(buf + cnt, "%8s ", bn ? bn : "");
		cnt += sprintf(buf + cnt, "%4s ", op_str[op]);
		cnt += _delta.show(buf + cnt, op);
		cnt += sprintf(buf + cnt, "%s\n", cn ? cn : "");
	
		bn = cn = NULL;
	}

	if (bn)
		buf[0] = '\0';
}

template <class T>
void Stat<T>::header(char *buf, const char *tag)
{
	int cnt;

	cnt = sprintf(buf, "%8s ", "Device");
	cnt += T::header(buf + cnt, tag);
	cnt += sprintf(buf + cnt, "%s\n", " Cgroup");
}

template <class T>
struct StatCmp {
	bool operator() (StatBase *a, StatBase *b)
	{
		auto sa = dynamic_cast<T *>(a);
		auto sb = dynamic_cast<T *>(b);

		return sa->_delta > sb->_delta;
	}
};

StatCmp<Stat<BlkData>> BlkCmp;

StatRun::StatRun(void)
{
	_run_buf = (char *)malloc(4096);
}

void StatRun::clear(void)
{
	std::map<std::string, StatNode*>::iterator it;
	int s;

	for (s = STAT_INDEX_META; s < STAT_INDEX_MAX; s++)
		_vector[s].clear();

	for (it = _map.begin(); it != _map.end(); ++it)
		delete it->second;

	_map.clear();
}

StatRun::~StatRun(void)
{
	clear();
	free(_run_buf);
}

/*
 * The tag here is cgroup name
 */
void StatRun::input(std::string &cn, char *buf)
{
	std::map<std::string, StatNode*>::iterator ita;
	std::map<std::string, StatBase*>::iterator itb;
	StatBase *b;
	StatNode *n;
	char *tag, *val;
	char bn[16];
	bool meta;

	ita = _map.find(cn);
	if (ita == _map.end()) {
		n = new StatNode();
		_map.insert(std::pair<std::string, StatNode*>(cn, n));
	} else {
		n = ita->second;
	}

	/*
	 * sda-meta x x x x x ...
	 */
	val = strchr(buf, ' ');
	*val = 0;
	val++;
	tag = buf;

	if (!parse_iostat_tag(tag, bn, &meta)) {
		ERR("invalid iostat tag format %s\n", tag);
		return;
	}

	itb = n->map.find(tag);
	if (itb == n->map.end()) {
		if (Conf.bdev_is_needed(bn)) {
			b = new Stat<BlkData>(cn, tag, bn);
			if (meta)
				_vector[STAT_INDEX_META].push_back(b);
			else
				_vector[STAT_INDEX_DATA].push_back(b);
			n->map.insert(std::pair<std::string, StatBase*>(std::string(tag), b));
		} else {
			b = NULL;
		}
	} else {
		b = itb->second;
	}

	if (b)
		b->input(val);
}

void StatRun::read_stat(std::string &cgrp, std::vector<char *> &stats)
{
	int fd, ret;
	char *buf = _run_buf;

	sprintf(buf, "%s/%s/%s", BLKIO, cgrp.c_str(), IOSTAT);

	fd = open(buf, O_RDONLY);
	if (fd < 0) {
		ERR("Failed to open %s due to %s\n", buf, strerror(errno));
		return;
	}

	ret = read(fd, buf, 4096);
	if (ret < 0) {
		ERR("Failed to read due to %s\n", strerror(errno));
		close(fd);
		return;
	}
	buf[ret] = 0;

	close(fd);
	str_split(buf, "\n", stats);
}

void StatRun::process(std::string &cgrp)
{
	std::vector<char *> stats;

	read_stat(cgrp, stats);

	for (auto s : stats)
		input(cgrp, s);
}

void StatRun::sort(void)
{
	std::sort(_vector[STAT_INDEX_META].begin(),  _vector[STAT_INDEX_META].end(), BlkCmp);
	std::sort(_vector[STAT_INDEX_DATA].begin(),  _vector[STAT_INDEX_DATA].end(), BlkCmp);
}

void StatRun::__output(int s)
{
	unsigned int i;

	for (i = 0; i < _vector[s].size(); i++) {
		_vector[s][i]->output(_run_buf);
		printf("%s", _run_buf);
	}
}

void StatRun::output(void)
{
	if (Conf.format & FORMAT_DATA) {
		Stat<BlkData>::header(_run_buf, "DATA");
		printf("%s", _run_buf);
		__output(STAT_INDEX_DATA);
	}

	if (Conf.format & FORMAT_META) {
		Stat<BlkData>::header(_run_buf, "META");
		printf("%s", _run_buf);
		__output(STAT_INDEX_META);
	}
}
