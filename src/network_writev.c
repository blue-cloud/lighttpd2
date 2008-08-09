
#include "network.h"

#include <sys/uio.h>

#ifndef UIO_MAXIOV
# if defined(__FreeBSD__) || defined(__APPLE__) || defined(__NetBSD__)
/* FreeBSD 4.7 defines it in sys/uio.h only if _KERNEL is specified */
#  define UIO_MAXIOV 1024
# elif defined(__sgi)
/* IRIX 6.5 has sysconf(_SC_IOV_MAX) which might return 512 or bigger */
#  define UIO_MAXIOV 512
# elif defined(__sun)
/* Solaris (and SunOS?) defines IOV_MAX instead */
#  ifndef IOV_MAX
#   define UIO_MAXIOV 16
#  else
#   define UIO_MAXIOV IOV_MAX
#  endif
# elif defined(IOV_MAX)
#  define UIO_MAXIOV IOV_MAX
# else
#  error UIO_MAXIOV nor IOV_MAX are defined
# endif
#endif

network_status_t network_backend_writev(server *srv, connection *con, int fd, chunkqueue *cq) {
	const off_t max_write = 256 * 1024; /* 256k */
	off_t min_cq_len, max_chunks_len, we_have;
	ssize_t r;
	gboolean did_write_something = FALSE;
	chunkiter ci;
	chunk *c;

	GArray *chunks = g_array_sized_new(FALSE, TRUE, sizeof(struct iovec), UIO_MAXIOV);

	/* stop if chunkqueue length gets less or equal than min_cq_len */
	min_cq_len = cq->length - max_write;
	if (min_cq_len < 0) min_cq_len = 0;

	do {
		if (0 == cq->length) return NETWORK_STATUS_SUCCESS;

		ci = chunkqueue_iter(cq);

		if (MEM_CHUNK != (c = chunkiter_chunk(ci))->type) {
			NETWORK_FALLBACK(network_backend_write);
			did_write_something = TRUE;
			continue;
		}

		max_chunks_len = cq->length - min_cq_len;
		we_have = 0;
		do {
			guint i = chunks->len;
			off_t len = c->mem->len - c->offset;
			struct iovec *v;
			g_array_set_size(chunks, i + 1);
			v = &g_array_index(chunks, struct iovec, i);
			v->iov_base = c->mem->str + c->offset;
			if (len > max_write) len = max_write;
			v->iov_len = len;
			we_have += len;
		} while (we_have < max_chunks_len &&
		         chunkiter_next(&ci) &&
		         MEM_CHUNK == (c = chunkiter_chunk(ci))->type &&
		         chunks->len < UIO_MAXIOV);

		while (-1 == (r = writev(fd, (struct iovec*) chunks->data, chunks->len))) {
			switch (errno) {
			case EAGAIN:
#if EWOULDBLOCK != EAGAIN
			case EWOULDBLOCK
#endif
				g_array_free(chunks, TRUE);
				return did_write_something ? NETWORK_STATUS_SUCCESS : NETWORK_STATUS_WAIT_FOR_EVENT;
			case ECONNRESET:
			case EPIPE:
				g_array_free(chunks, TRUE);
				return NETWORK_STATUS_CONNECTION_CLOSE;
			case EINTR:
				break; /* try again */
			default:
				g_array_free(chunks, TRUE);
				CON_ERROR(srv, con, "oops, write to fd=%d failed: %s (%d)", fd, strerror(errno), errno );
				return NETWORK_STATUS_FATAL_ERROR;
			}
		}
		if (0 == r) {
			g_array_free(chunks, TRUE);
			return did_write_something ? NETWORK_STATUS_SUCCESS : NETWORK_STATUS_WAIT_FOR_EVENT;
		}
		chunkqueue_skip(cq, r);
		if (r != we_have) {
			g_array_free(chunks, TRUE);
			return NETWORK_STATUS_SUCCESS;
		}
		did_write_something = TRUE;
		g_array_set_size(chunks, 0);
	} while (cq->length > min_cq_len);

	g_array_free(chunks, TRUE);
	return NETWORK_STATUS_SUCCESS;
}