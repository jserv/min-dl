struct __dirstream
{
	int fd;
	off_t tell;
	int buf_pos;
	int buf_end;
	volatile int lock[1];
	char buf[2048];
};
