/*
 * Device Object (/3) for SIMCOM A7672E
 *
 * LwM2M Object providing device information using SIMCOM SDK APIs.
 */

#ifndef DEVICE_OBJECT_H
#define DEVICE_OBJECT_H

#include <anjay/anjay.h>
#include <anjay/dm.h>

/**
 * Device Object definition pointer for registration.
 * Usage: anjay_register_object(anjay, &OBJECT_DEV);
 */
extern const anjay_dm_object_def_t **device_object_def_ptr(void);

/**
 * Cleanup device object resources.
 */
void device_object_cleanup(void);

#endif /* DEVICE_OBJECT_H */
