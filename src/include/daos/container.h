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
 * dc_cont: Container Client API
 */

#ifndef __DAOS_CONTAINER_H__
#define __DAOS_CONTAINER_H__

#include <daos_types.h>
#include <daos/pool_map.h>
#include <daos/tse.h>

int dc_cont_init(void);
void dc_cont_fini(void);

int dc_cont_tgt_idx2ptr(daos_handle_t coh, uint32_t tgt_idx,
			struct pool_target **tgt);
int dc_cont_hdl2uuid(daos_handle_t coh, uuid_t *hdl_uuid, uuid_t *con_uuid);
daos_handle_t dc_cont_hdl2pool_hdl(daos_handle_t coh);

int dc_cont_local2global(daos_handle_t coh, daos_iov_t *glob);
int dc_cont_global2local(daos_handle_t poh, daos_iov_t glob,
			 daos_handle_t *coh);

int dc_cont_local_open(uuid_t cont_uuid, uuid_t cont_hdl_uuid,
		       unsigned int flags, daos_handle_t ph,
		       daos_handle_t *coh);
int dc_cont_local_close(daos_handle_t ph, daos_handle_t coh);

int dc_cont_create(tse_task_t *task);
int dc_cont_open(tse_task_t *task);
int dc_cont_close(tse_task_t *task);
int dc_cont_destroy(tse_task_t *task);
int dc_cont_query(tse_task_t *task);
int dc_cont_attr_list(tse_task_t *task);
int dc_cont_attr_get(tse_task_t *task);
int dc_cont_attr_set(tse_task_t *task);
int dc_epoch_flush(tse_task_t *task);
int dc_epoch_discard(tse_task_t *task);
int dc_epoch_query(tse_task_t *task);
int dc_epoch_hold(tse_task_t *task);
int dc_epoch_slip(tse_task_t *task);
int dc_epoch_commit(tse_task_t *task);
int dc_epoch_wait(tse_task_t *task);
int dc_snap_list(tse_task_t *task);
int dc_snap_create(tse_task_t *task);
int dc_snap_destroy(tse_task_t *task);

#endif /* __DAOS_CONTAINER_H__ */
