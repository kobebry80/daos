/**
 * (C) Copyright 2017 Intel Corporation.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * GOVERNMENT LICENSE RIGHTS-OPEN SOURCE SOFTWARE
 * The Government's rights to use, modify, reproduce, release, perform, display,
 * or disclose this software are subject to the terms of the Apache License as
 * provided in Contract No. B609815.
 * Any reproduction of computer software, computer software documentation, or
 * portions thereof marked with this legend must also reproduce the markings.
 */
/**
 * rdb: Paths
 */

#define DDSUBSYS DDFAC(rdb)

#include <daos_srv/rdb.h>

#include "rdb_internal.h"

/* Key for the root KVS */
daos_iov_t rdb_path_root_key;

static inline void
rdb_path_assert(const rdb_path_t *path)
{
	D__ASSERT(path->iov_buf != NULL && path->iov_buf_len > 0 &&
		 path->iov_buf_len <= rdb_iov_max);
	D__ASSERT(path->iov_len <= path->iov_buf_len);
}

/**
 * Initialize \a path. If successful, \a path is empty (i.e., does not
 * represent the root KVS).
 *
 * \param[out]	path	path
 *
 * \retval -DER_NOMEM	failed to allocate initial path buffer
 */
int
rdb_path_init(rdb_path_t *path)
{
	daos_iov_t p = {};

	p.iov_buf_len = 128;
	D__ALLOC(p.iov_buf, p.iov_buf_len);
	if (p.iov_buf == NULL)
		return -DER_NOMEM;
	*path = p;
	rdb_path_assert(path);
	return 0;
}

/**
 * Finalize \a path.
 *
 * \param[in,out]	path	path
 */
void
rdb_path_fini(rdb_path_t *path)
{
	rdb_path_assert(path);
	D__FREE(path->iov_buf, path->iov_buf_len);
}

/**
 * Clone \a path into \a new_path.
 *
 * \param[in]	path		existing path
 * \param[out]	new_path	new path
 *
 * \retval -DER_NOMEM		failed to allocate initial path buffer
 */
int
rdb_path_clone(const rdb_path_t *path, rdb_path_t *new_path)
{
	void *buf;

	rdb_path_assert(path);
	D__ALLOC(buf, path->iov_buf_len);
	if (buf == NULL)
		return -DER_NOMEM;
	memcpy(buf, path->iov_buf, path->iov_len);
	new_path->iov_buf = buf;
	new_path->iov_buf_len = path->iov_buf_len;
	new_path->iov_len = path->iov_len;
	return 0;
}

/**
 * Push \a key to the end of \a path. \a path must have been initialized by
 * rdb_path_init() already.
 *
 * \param[in,out]	path	path
 * \param[in]		key	key
 *
 * \retval -DER_NOMEM		failed to allocate initial path buffer
 * \retval -DER_OVERFLOW	path would become too large
 */
int
rdb_path_push(rdb_path_t *path, const daos_iov_t *key)
{
	size_t	len;
	size_t	n;

	rdb_path_assert(path);
	D__ASSERT(key->iov_len <= key->iov_buf_len);
	len = rdb_encode_iov(key, NULL /* buf */);
	if (path->iov_len + len > path->iov_buf_len) {
		size_t	buf_len = path->iov_buf_len;
		void   *buf;

		/* Not enough capacity; reallocate a larger buffer. */
		do {
			if (buf_len == rdb_iov_max)
				return -DER_OVERFLOW;
			buf_len = min(buf_len * 2, rdb_iov_max);
		} while (buf_len < path->iov_len + len);
		D__ALLOC(buf, buf_len);
		if (buf == NULL)
			return -DER_NOMEM;
		memcpy(buf, path->iov_buf, path->iov_len);
		D__FREE(path->iov_buf, path->iov_buf_len);
		path->iov_buf = buf;
		path->iov_buf_len = buf_len;
	}
	n = rdb_encode_iov(key, path->iov_buf + path->iov_len);
	D__ASSERTF(n == len, "%zu == %zu\n", n, len);
	path->iov_len += n;
	return 0;
}

/**
 * Pop a key from the end of \a path. \a path must have been initialized by
 * rdb_path_init() already.
 *
 * \param[in,out]	path	path
 *
 * \retval -DER_NONEXIST	path is empty
 */
int
rdb_path_pop(rdb_path_t *path)
{
	daos_iov_t	key;
	ssize_t		n;

	rdb_path_assert(path);
	if (path->iov_len == 0)
		return -DER_NONEXIST;
	n = rdb_decode_iov_backward(path->iov_buf + path->iov_len,
				    path->iov_len, &key);
	D__ASSERTF(n > 0, "%zd\n", n);
	path->iov_len -= n;
	return 0;
}

/* Iterate through each key in path. */
int
rdb_path_iterate(const rdb_path_t *path, rdb_path_iterate_cb_t cb, void *arg)
{
	void   *p = path->iov_buf;

	rdb_path_assert(path);
	while (p < path->iov_buf + path->iov_len) {
		ssize_t		n;
		daos_iov_t	key;
		int		rc;

		n = rdb_decode_iov(p, path->iov_buf + path->iov_len - p, &key);
		if (n < 0)
			return n;
		rc = cb(&key, arg);
		if (rc != 0) {
			if (rc == 1)
				rc = 0;
			return rc;
		}
		p += n;
	}
	return 0;
}
