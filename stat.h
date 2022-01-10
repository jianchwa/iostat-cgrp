#ifndef __STAT_H__
#define __STAT_H__

struct blk_stat {
	u64 bytes[OP_MAX];
	u64 ios[OP_MAX];
	u64 queue_lat[OP_MAX];
	u64 dev_lat[OP_MAX];
};

struct BlkStatCmp;

class StatBase {
public:
	virtual ~StatBase(void) {}
	virtual void input(char *) = 0;
	virtual void output(char *) = 0;
};

template <class T>
class Stat : public StatBase {
public:
	Stat(std::string &cn, char *tag, char *bn);
	~Stat(void);
	void input(char *);
	void output(char *);
	static void header(char *, const char *);

	T _delta;
private:
	std::string _cn; /* cgroup name */
	std::string _tag; /* used when lookup */
	std::string _bn; /* block device name, used when output */
	/*
	 * _index is now, (_index - 1) is prevoius
	 */
	u64 _index;
	T _stat[2];
};

class BlkData {
public:
	BlkData(void);
	void operator=(const char *);
	void operator=(const BlkData &);
	BlkData operator-(const BlkData &);
	bool operator>(const BlkData &);
	static int header(char *, const char *);
	int show(char *, int);
	bool empty(int);
private:
	u64 _bytes[OP_MAX];
	u64 _ios[OP_MAX];
	u64 _queue_lat[OP_MAX];
	u64 _dev_lat[OP_MAX];
};

struct StatNode {
	/* Indexed by block device name */
	std::map<std::string, StatBase*> map;
	explicit StatNode(void) {}
	~StatNode(void)
	{
		std::map<std::string, StatBase*>::iterator it;

		for (it = map.begin(); it != map.end(); ++it)
			delete it->second;

		map.clear();
	}
};

enum {
	STAT_INDEX_DATA,
	STAT_INDEX_META,
	STAT_INDEX_MAX
};

class StatRun {
public:
	StatRun(void);
	~StatRun(void);
	void process(std::string &);
	void sort();
	void output(void);
	void clear(void);
private:
	/* Used to lookup by cgroup name */
	std::map<std::string, StatNode*> _map;
	/* Used to sort blk output result */
	std::vector<StatBase *> _vector[STAT_INDEX_MAX];
	char *_run_buf;
	void input(std::string &cn, char *buf);
	void __output(int s);
	void read_stat(std::string &, std::vector<char *>&);
};

#endif
