/*
 * Connectivity Statistics Object (/7) for SIMCOM A7672E
 *
 * LwM2M Object providing network connectivity statistics.
 */

#ifndef CONN_STATS_OBJECT_H
#define CONN_STATS_OBJECT_H

#include <anjay/anjay.h>
#include <anjay/dm.h>

/**
 * Connectivity Statistics Object definition pointer for registration.
 * Usage: anjay_register_object(anjay, &OBJECT_CONN_STATS);
 */
extern const anjay_dm_object_def_t **conn_stats_object_def_ptr(void);

/**
 * Cleanup connectivity statistics object resources.
 */
void conn_stats_object_cleanup(void);

#endif /* CONN_STATS_OBJECT_H */
