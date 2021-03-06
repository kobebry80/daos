/**
 * (C) Copyright 2016 Intel Corporation.
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
 * This file is part of the DAOS server. It implements thread-local storage
 * (TLS) for DAOS service threads.
 */
#define DDSUBSYS       DDFAC(server)

#include <pthread.h>
#include "srv_internal.h"

/* The array remember all of registered module keys on one node. */
struct dss_module_key *dss_module_keys[DAOS_MODULE_KEYS_NR] = { NULL };

pthread_mutex_t dss_module_keys_lock = PTHREAD_MUTEX_INITIALIZER;

void
dss_register_key(struct dss_module_key *key)
{
	int i;

	pthread_mutex_lock(&dss_module_keys_lock);
	for (i = 0; i < DAOS_MODULE_KEYS_NR; i++) {
		if (dss_module_keys[i] == NULL) {
			dss_module_keys[i] = key;
			key->dmk_index = i;
			break;
		}
	}
	pthread_mutex_unlock(&dss_module_keys_lock);
	D__ASSERT(i < DAOS_MODULE_KEYS_NR);
}

void
dss_unregister_key(struct dss_module_key *key)
{
	if (key == NULL)
		return;
	D__ASSERT(key->dmk_index >= 0);
	D__ASSERT(key->dmk_index < DAOS_MODULE_KEYS_NR);
	pthread_mutex_lock(&dss_module_keys_lock);
	dss_module_keys[key->dmk_index] = NULL;
	pthread_mutex_unlock(&dss_module_keys_lock);
}

/**
 * Init thread context
 *
 * \param[in]dtls	Init the thread context to allocate the
 *                      local thread variable for each module.
 *
 * \retval		0 if initialization succeeds
 * \retval		negative errno if initialization fails
 */
static int
dss_thread_local_storage_init(struct dss_thread_local_storage *dtls)
{
	int rc = 0;
	int i;

	if (dtls->dtls_values == NULL) {
		D__ALLOC(dtls->dtls_values, ARRAY_SIZE(dss_module_keys) *
					 sizeof(dtls->dtls_values[0]));
		if (dtls->dtls_values == NULL)
			return -DER_NOMEM;
	}

	for (i = 0; i < DAOS_MODULE_KEYS_NR; i++) {
		struct dss_module_key *dmk = dss_module_keys[i];

		if (dmk != NULL && dtls->dtls_tag & dmk->dmk_tags) {
			D__ASSERT(dmk->dmk_init != NULL);
			dtls->dtls_values[i] = dmk->dmk_init(dtls, dmk);
			if (dtls->dtls_values[i] == NULL) {
				rc = -DER_NOMEM;
				break;
			}
		}
	}
	return rc;
}

/**
 * Finish module context
 *
 * \param[in]dtls	Finish the thread context to free the
 *                      local thread variable for each module.
 */
static void
dss_thread_local_storage_fini(struct dss_thread_local_storage *dtls)
{
	int i;

	if (dtls->dtls_values != NULL) {
		for (i = 0; i < DAOS_MODULE_KEYS_NR; i++) {
			struct dss_module_key *dmk = dss_module_keys[i];

			if (dtls->dtls_values[i] != NULL) {
				D__ASSERT(dmk != NULL);
				D__ASSERT(dmk->dmk_fini != NULL);
				dmk->dmk_fini(dtls, dmk, dtls->dtls_values[i]);
			}
		}
	}

	D__FREE(dtls->dtls_values,
	       ARRAY_SIZE(dss_module_keys) * sizeof(dtls->dtls_values[0]));
}

pthread_key_t dss_tls_key;

/*
 * Allocate dss_thread_local_storage for a particular thread and
 * store the pointer in a thread-specific value which can be
 * fetched at any time with dss_tls_get().
 */
struct dss_thread_local_storage *
dss_tls_init(int tag)
{
	struct dss_thread_local_storage *dtls;
	int		 rc;

	D__ALLOC_PTR(dtls);
	if (dtls == NULL)
		return NULL;

	dtls->dtls_tag = tag;
	rc = dss_thread_local_storage_init(dtls);
	if (rc != 0) {
		D__FREE_PTR(dtls);
		return NULL;
	}

	rc = pthread_setspecific(dss_tls_key, dtls);
	if (rc) {
		D__ERROR("failed to initialize tls: %d\n", rc);
		dss_thread_local_storage_fini(dtls);
		D__FREE_PTR(dtls);
		return NULL;
	}

	return dtls;
}

/*
 * Free DTC for a particular thread. Called upon thread termination via the
 * pthread key destructor.
 */
void
dss_tls_fini(void *arg)
{
	struct dss_thread_local_storage  *dtls;

	dtls = (struct dss_thread_local_storage  *)arg;
	dss_thread_local_storage_fini(dtls);
}
