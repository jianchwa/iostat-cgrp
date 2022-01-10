#ifndef __UTILS_H__
#define __UTILS_H__
bool is_cgroup(char *name);
bool is_number(char *s);
bool is_bdev(char *name);
char *tobw(u64 bytes, char *buf);
char *totime(u64 nsec, char *buf);
char *toiops(u64 cnt, char *buf);
char *tosz(u64 cnt, char *buf);
int get_bdev(char *bdev, char *out);
void str_split(char *buf, const char *ch, std::vector<char *> &l);
bool is_digits(const char *buf);
bool parse_iostat_tag(char *tag, char *bn, bool *meta);
bool iostat_opened(const char *bdev);

struct StackBuf {
	char *buf;

	StackBuf(int sz)
	{
		buf = (char *)malloc(sz);
	}
	~StackBuf()
	{
		free(buf);
	}
};

#define STACK_BUF(name, sz) 			\
	StackBuf _StackBuf_##name(sz); 		\
	char *name = _StackBuf_##name.buf; 	\

#endif
