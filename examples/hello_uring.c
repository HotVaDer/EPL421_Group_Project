/*
 * hello_uring.c - minimal liburing example.
 *
 * Reads a file with a single io_uring read request and prints it.
 * Usage: ./hello_uring [file]   (default: ../README.md)
 */
#include <fcntl.h>
#include <liburing.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define BUF_SIZE 4096

int main(int argc, char *argv[])
{
	const char *path = argc > 1 ? argv[1] : "../README.md";
	char buf[BUF_SIZE];
	struct io_uring ring;
	struct io_uring_sqe *sqe;
	struct io_uring_cqe *cqe;
	int fd, ret;

	fd = open(path, O_RDONLY);
	if (fd < 0) {
		perror("open");
		return 1;
	}

	ret = io_uring_queue_init(8, &ring, 0);
	if (ret < 0) {
		fprintf(stderr, "io_uring_queue_init: %s\n", strerror(-ret));
		close(fd);
		return 1;
	}

	sqe = io_uring_get_sqe(&ring);
	io_uring_prep_read(sqe, fd, buf, sizeof(buf) - 1, 0);
	io_uring_submit(&ring);

	ret = io_uring_wait_cqe(&ring, &cqe);
	if (ret < 0) {
		fprintf(stderr, "io_uring_wait_cqe: %s\n", strerror(-ret));
	} else {
		if (cqe->res < 0) {
			fprintf(stderr, "read: %s\n", strerror(-cqe->res));
			ret = cqe->res;
		} else {
			buf[cqe->res] = '\0';
			printf("Read %d bytes from %s via io_uring:\n\n%s\n",
			       cqe->res, path, buf);
		}
		io_uring_cqe_seen(&ring, cqe);
	}

	io_uring_queue_exit(&ring);
	close(fd);
	return ret < 0 ? 1 : 0;
}
