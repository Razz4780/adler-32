#include <minix/drivers.h>
#include <minix/chardriver.h>
#include <stdio.h>
#include <stdlib.h>
#include <minix/ds.h>

#define OUTPUT_SIZE 8
#define ADLER_ALG_PRIME 65521

static int adler_open(devminor_t, int, endpoint_t);

static int adler_close(devminor_t);

static ssize_t adler_read(devminor_t, u64_t, endpoint_t, cp_grant_id_t, size_t, int, cdev_id_t);

static ssize_t adler_write(devminor_t, u64_t, endpoint_t, cp_grant_id_t, size_t, int, cdev_id_t);

static void sef_local_startup(void);

static int sef_cb_init(int, sef_init_info_t *);

static int sef_cb_lu_state_save(int);

static int lu_state_restore(void);

static struct chardriver adler_tab =
        {
                .cdr_read    = adler_read,
                .cdr_write   = adler_write,
        };

static unsigned int adler_sum_a;
static unsigned int adler_sum_b;

static void init_sums() {
    adler_sum_a = 1;
    adler_sum_b = 0;
}

static ssize_t adler_read(devminor_t UNUSED(minor), u64_t UNUSED(position),
                          endpoint_t endpt, cp_grant_id_t grant, size_t size,
                          int UNUSED(flags), cdev_id_t UNUSED(id)) {
    unsigned char buf[OUTPUT_SIZE];
    int ret;
    if (size < OUTPUT_SIZE) {
        return EINVAL;
    }
    sprintf(buf, "%08x", adler_sum_a + (adler_sum_b << 16));
    ret = sys_safecopyto(endpt, grant, 0, (vir_bytes) buf, OUTPUT_SIZE);
    if (ret == OK) {
        init_sums();
        return OUTPUT_SIZE;
    }
    return ret;
}

static ssize_t adler_write(devminor_t UNUSED(minor), u64_t UNUSED(position),
                           endpoint_t endpt, cp_grant_id_t grant, size_t size,
                           int UNUSED(flags), cdev_id_t UNUSED(id)) {
    unsigned char buf[1024];
    int ret;
    size_t bytes_copied = 0;
    while (bytes_copied < size) {
        size_t to_copy = size - bytes_copied < 1024 ? size - bytes_copied : 1024;
        ret = sys_safecopyfrom(endpt, grant, (vir_bytes) bytes_copied, (vir_bytes) buf, to_copy);
        if (ret != OK) {
            return ret;
        }
        bytes_copied += to_copy;
        for (size_t i = 0; i < to_copy; ++i) {
            adler_sum_a += buf[i];
            adler_sum_a %= ADLER_ALG_PRIME;
            adler_sum_b += adler_sum_a;
            adler_sum_b %= ADLER_ALG_PRIME;
        }
    }
    return size;
}

static int sef_cb_lu_state_save(int UNUSED(state)) {
    ds_publish_u32("adler_sum", adler_sum_a + (adler_sum_b << 16), DSF_OVERWRITE);
    return OK;
}

static int lu_state_restore() {
    unsigned int adler_sum;
    ds_retrieve_u32("adler_sum", &adler_sum);
    ds_delete_u32("adler_sum");
    adler_sum_a = adler_sum % (1 << 16);
    adler_sum_b = adler_sum >> 16;
    return OK;
}

static void sef_local_startup() {
    sef_setcb_init_fresh(sef_cb_init);
    sef_setcb_init_lu(sef_cb_init);
    sef_setcb_init_restart(sef_cb_init);
    
    sef_setcb_lu_prepare(sef_cb_lu_prepare_always_ready);
    sef_setcb_lu_state_isvalid(sef_cb_lu_state_isvalid_standard);
    sef_setcb_lu_state_save(sef_cb_lu_state_save);
    
    sef_startup();
}

static int sef_cb_init(int type, sef_init_info_t *UNUSED(info)) {
    if (type == SEF_INIT_LU) {
        lu_state_restore();
    } else {
        init_sums();
    }
    return OK;
}

int main(void) {
    sef_local_startup();
    chardriver_task(&adler_tab);
    return OK;
}
