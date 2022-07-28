//
// Created by samuel on 18-7-22.
//

#include "bluetooth.h"

#include <assert.h>
#include <ctype.h>
#include <esp_console.h>
#include <esp_log.h>
#include <string.h>
#include "esp_nimble_hci.h"
#include "host/ble_hs.h"
#include "host/ble_uuid.h"
#include "host/util/util.h"
#include "modlog/modlog.h"
#include "nimble/ble.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"


// bleprph.h
/** GATT server. */
#define GATT_SVR_SVC_ALERT_UUID               0x1811


// misc.c

void print_mac_address(const void *addr) {
    const uint8_t *u8p;

    u8p = addr;
    MODLOG_DFLT(INFO, "%02x:%02x:%02x:%02x:%02x:%02x",
                u8p[5], u8p[4], u8p[3], u8p[2], u8p[1], u8p[0]);
}


// gatt_svr.c

/**
 * The vendor specific security test service consists of two characteristics:
 *     o random-number-generator: generates a random 32-bit number each time
 *       it is read.  This characteristic can only be read over an encrypted
 *       connection.
 *     o static-value: a single-byte characteristic that can always be read,
 *       but can only be written over an encrypted connection.
 */

/* 59462f12-9543-9999-12c8-58b459a2712d */
static const ble_uuid128_t gatt_svr_svc_sec_test_uuid =
        BLE_UUID128_INIT(0x2d, 0x71, 0xa2, 0x59, 0xb4, 0x58, 0xc8, 0x12,
                         0x99, 0x99, 0x43, 0x95, 0x12, 0x2f, 0x46, 0x59);

/* 5c3a659e-897e-45e1-b016-007107c96df6 */
static const ble_uuid128_t random_characteristic_uuid =
        BLE_UUID128_INIT(0xf6, 0x6d, 0xc9, 0x07, 0x71, 0x00, 0x16, 0xb0,
                         0xe1, 0x45, 0x7e, 0x89, 0x9e, 0x65, 0x3a, 0x5c);

/* 5c3a659e-897e-45e1-b016-007107c96df7 */
static const ble_uuid128_t test_characteristic_uuid =
        BLE_UUID128_INIT(0xf7, 0x6d, 0xc9, 0x07, 0x71, 0x00, 0x16, 0xb0,
                         0xe1, 0x45, 0x7e, 0x89, 0x9e, 0x65, 0x3a, 0x5c);

static uint8_t test_characteristic_value;

static int handle_characteristic_call(uint16_t conn_handle, uint16_t attr_handle,
                                      struct ble_gatt_access_ctxt *context,
                                      void *arg);

static const struct ble_gatt_svc_def create_services_config[] = {
        {
                /*** Service: Security test. */
                .type = BLE_GATT_SVC_TYPE_PRIMARY,
                .uuid = &gatt_svr_svc_sec_test_uuid.u,
                .characteristics = (struct ble_gatt_chr_def[])
                        {{
                                 /*** Characteristic: Random number generator. */
                                 .uuid = &random_characteristic_uuid.u,
                                 .access_cb = handle_characteristic_call,
                                 .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_READ_ENC,
                         },
                         {
                                 /*** Characteristic: Static value. */
                                 .uuid = &test_characteristic_uuid.u,
                                 .access_cb = handle_characteristic_call,
                                 .flags = BLE_GATT_CHR_F_READ |
                                          BLE_GATT_CHR_F_WRITE | BLE_GATT_CHR_F_WRITE_ENC,
                         },
                         {
                                 0, /* No more characteristics in this service. */
                         }
                        },
        },

        {
                0, /* No more services. */
        },
};

static int characteristic_write_action(struct os_mbuf *om, uint16_t min_length, uint16_t max_length,
                                       void *destination, uint16_t *length) {
    uint16_t om_length;
    int rc;

    om_length = OS_MBUF_PKTLEN(om);
    if (om_length < min_length || om_length > max_length) {
        return BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN;
    }

    rc = ble_hs_mbuf_to_flat(om, destination, max_length, length);
    if (rc != 0) {
        return BLE_ATT_ERR_UNLIKELY;
    }

    return 0;
}

static int handle_characteristic_call(uint16_t conn_handle, uint16_t attr_handle,
                                      struct ble_gatt_access_ctxt *context,
                                      void *arg) {
    const ble_uuid_t *uuid;
    int random_number;
    int rc;

    uuid = context->chr->uuid;

    /* Determine which characteristic is being accessed by examining its
     * 128-bit UUID.
     */

    if (ble_uuid_cmp(uuid, &random_characteristic_uuid.u) == 0) {
        assert(context->op == BLE_GATT_ACCESS_OP_READ_CHR);

        /* Respond with a 32-bit random number. */
        random_number = rand();
        rc = os_mbuf_append(context->om, &random_number, sizeof random_number);
        return rc == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
    }

    if (ble_uuid_cmp(uuid, &test_characteristic_uuid.u) == 0) {
        switch (context->op) {
            case BLE_GATT_ACCESS_OP_READ_CHR:
                rc = os_mbuf_append(context->om, &test_characteristic_value,
                                    sizeof test_characteristic_value);
                return rc == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;

            case BLE_GATT_ACCESS_OP_WRITE_CHR:
                rc = characteristic_write_action(context->om,
                                                 sizeof test_characteristic_value,
                                                 sizeof test_characteristic_value,
                                                 &test_characteristic_value, NULL);
                return rc;

            default:
                assert(0);
                return BLE_ATT_ERR_UNLIKELY;
        }
    }

    /* Unknown characteristic; the nimble stack should not have called this
     * function.
     */
    assert(0);
    return BLE_ATT_ERR_UNLIKELY;
}

void on_resource_registered(struct ble_gatt_register_ctxt *context, void *arg) {
    char buf[BLE_UUID_STR_LEN];

    switch (context->op) {
        case BLE_GATT_REGISTER_OP_SVC:
            MODLOG_DFLT(DEBUG, "registered service %s with handle=%d\n",
                        ble_uuid_to_str(context->svc.svc_def->uuid, buf),
                        context->svc.handle);
            break;

        case BLE_GATT_REGISTER_OP_CHR:
            MODLOG_DFLT(DEBUG, "registering characteristic %s with "
                               "def_handle=%d val_handle=%d\n",
                        ble_uuid_to_str(context->chr.chr_def->uuid, buf),
                        context->chr.def_handle,
                        context->chr.val_handle);
            break;

        case BLE_GATT_REGISTER_OP_DSC:
            MODLOG_DFLT(DEBUG, "registering descriptor %s with handle=%d\n",
                        ble_uuid_to_str(context->dsc.dsc_def->uuid, buf),
                        context->dsc.handle);
            break;

        default:
            assert(0);
            break;
    }
}

int initialize_server(void) {
    int rc;

    ble_svc_gap_init();
    ble_svc_gatt_init();

    rc = ble_gatts_count_cfg(create_services_config);
    if (rc != 0) {
        return rc;
    }

    rc = ble_gatts_add_svcs(create_services_config);
    if (rc != 0) {
        return rc;
    }

    return 0;
}




// main.c

static const char *logging_tag = "NimBLE_BLE_PRPH";

static int on_server_event(struct ble_gap_event *event, void *arg);

static uint8_t own_address_type;

void ble_store_config_init(void);

/**
 * Logs information about a connection to the console.
 */
static void print_connection_descriptor(struct ble_gap_conn_desc *descriptor) {
    MODLOG_DFLT(INFO, "handle=%d our_ota_addr_type=%d our_ota_addr=",
                descriptor->conn_handle, descriptor->our_ota_addr.type);
    print_mac_address(descriptor->our_ota_addr.val);
    MODLOG_DFLT(INFO, " our_id_addr_type=%d our_id_addr=",
                descriptor->our_id_addr.type);
    print_mac_address(descriptor->our_id_addr.val);
    MODLOG_DFLT(INFO, " peer_ota_addr_type=%d peer_ota_addr=",
                descriptor->peer_ota_addr.type);
    print_mac_address(descriptor->peer_ota_addr.val);
    MODLOG_DFLT(INFO, " peer_id_addr_type=%d peer_id_addr=",
                descriptor->peer_id_addr.type);
    print_mac_address(descriptor->peer_id_addr.val);
    MODLOG_DFLT(INFO, " conn_itvl=%d conn_latency=%d supervision_timeout=%d "
                      "encrypted=%d authenticated=%d bonded=%d\n",
                descriptor->conn_itvl, descriptor->conn_latency,
                descriptor->supervision_timeout,
                descriptor->sec_state.encrypted,
                descriptor->sec_state.authenticated,
                descriptor->sec_state.bonded);
}

/**
 * Enables advertising with the following parameters:
 *     o General discoverable mode.
 *     o Undirected connectable mode.
 */
static void advertise(State *state) {
    struct ble_gap_adv_params adv_params;
    struct ble_hs_adv_fields fields;
    const char *name;
    int rc;

    state->bluetooth.connected = false;

    /**
     *  Set the advertisement data included in our advertisements:
     *     o Flags (indicates advertisement type and other general info).
     *     o Advertising tx power.
     *     o Device name.
     *     o 16-bit service UUIDs (alert notifications).
     */

    memset(&fields, 0, sizeof fields);

    /* Advertise two flags:
     *     o Discoverability in forthcoming advertisement (general)
     *     o BLE-only (BR/EDR unsupported).
     */
    fields.flags = BLE_HS_ADV_F_DISC_GEN |
                   BLE_HS_ADV_F_BREDR_UNSUP;

    /* Indicate that the TX power level field should be included; have the
     * stack fill this value automatically.  This is done by assigning the
     * special value BLE_HS_ADV_TX_PWR_LVL_AUTO.
     */
    fields.tx_pwr_lvl_is_present = 1;
    fields.tx_pwr_lvl = BLE_HS_ADV_TX_PWR_LVL_AUTO;

    name = ble_svc_gap_device_name();
    fields.name = (uint8_t *) name;
    fields.name_len = strlen(name);
    fields.name_is_complete = 1;

    fields.uuids16 = (ble_uuid16_t[]) {
            BLE_UUID16_INIT(GATT_SVR_SVC_ALERT_UUID)
    };
    fields.num_uuids16 = 1;
    fields.uuids16_is_complete = 1;

    rc = ble_gap_adv_set_fields(&fields);
    if (rc != 0) {
        MODLOG_DFLT(ERROR, "error setting advertisement data; rc=%d\n", rc);
        return;
    }

    /* Begin advertising. */
    memset(&adv_params, 0, sizeof adv_params);
    adv_params.conn_mode = BLE_GAP_CONN_MODE_UND;
    adv_params.disc_mode = BLE_GAP_DISC_MODE_GEN;
    rc = ble_gap_adv_start(own_address_type, NULL, BLE_HS_FOREVER,
                           &adv_params, on_server_event, state);
    if (rc != 0) {
        MODLOG_DFLT(ERROR, "error enabling advertisement; rc=%d\n", rc);
        return;
    }
}

/**
 * The nimble host executes this callback when a GAP event occurs.  The
 * application associates a GAP event callback with each connection that forms.
 * bleprph uses the same callback for all connections.
 *
 * @param event                 The type of event being signalled.
 * @param ctxt                  Various information pertaining to the event.
 * @param arg                   Application-specified argument; unused by
 *                                  bleprph.
 *
 * @return                      0 if the application successfully handled the
 *                                  event; nonzero on failure.  The semantics
 *                                  of the return code is specific to the
 *                                  particular GAP event being signalled.
 */
static int on_server_event(struct ble_gap_event *event, void *arg) {
    State *state = arg;
    struct ble_gap_conn_desc descriptor;
    int rc;

    switch (event->type) {
        case BLE_GAP_EVENT_CONNECT:
            /* A new connection was established or a connection attempt failed. */
            MODLOG_DFLT(INFO, "connection %s; status=%d ",
                        event->connect.status == 0 ? "established" : "failed",
                        event->connect.status);
            if (event->connect.status == 0) {
                rc = ble_gap_conn_find(event->connect.conn_handle, &descriptor);
                assert(rc == 0);
                print_connection_descriptor(&descriptor);
                state->bluetooth.connected = true;
            }
            MODLOG_DFLT(INFO, "\n");

            if (event->connect.status != 0) {
                /* Connection failed; resume advertising. */
                advertise(state);
            }
            return 0;

        case BLE_GAP_EVENT_DISCONNECT:
            MODLOG_DFLT(INFO, "disconnect; reason=%d ", event->disconnect.reason);
            print_connection_descriptor(&event->disconnect.conn);
            MODLOG_DFLT(INFO, "\n");

            /* Connection terminated; resume advertising. */
            advertise(state);
            return 0;

        case BLE_GAP_EVENT_CONN_UPDATE:
            /* The central has updated the connection parameters. */
            MODLOG_DFLT(INFO, "connection updated; status=%d ",
                        event->conn_update.status);
            rc = ble_gap_conn_find(event->conn_update.conn_handle, &descriptor);
            assert(rc == 0);
            print_connection_descriptor(&descriptor);
            MODLOG_DFLT(INFO, "\n");

            state->bluetooth.connected = true;
            return 0;

        case BLE_GAP_EVENT_ADV_COMPLETE:
            MODLOG_DFLT(INFO, "advertise complete; reason=%d",
                        event->adv_complete.reason);
            advertise(state);
            return 0;

        case BLE_GAP_EVENT_ENC_CHANGE:
            /* Encryption has been enabled or disabled for this connection. */
            MODLOG_DFLT(INFO, "encryption change event; status=%d ",
                        event->enc_change.status);
            rc = ble_gap_conn_find(event->enc_change.conn_handle, &descriptor);
            assert(rc == 0);
            print_connection_descriptor(&descriptor);
            MODLOG_DFLT(INFO, "\n");
            return 0;

        case BLE_GAP_EVENT_SUBSCRIBE:
            MODLOG_DFLT(INFO, "subscribe event; conn_handle=%d attr_handle=%d "
                              "reason=%d prevn=%d curn=%d previ=%d curi=%d\n",
                        event->subscribe.conn_handle,
                        event->subscribe.attr_handle,
                        event->subscribe.reason,
                        event->subscribe.prev_notify,
                        event->subscribe.cur_notify,
                        event->subscribe.prev_indicate,
                        event->subscribe.cur_indicate);
            return 0;

        case BLE_GAP_EVENT_MTU:
            MODLOG_DFLT(INFO, "mtu update event; conn_handle=%d cid=%d mtu=%d\n",
                        event->mtu.conn_handle,
                        event->mtu.channel_id,
                        event->mtu.value);
            return 0;

        case BLE_GAP_EVENT_REPEAT_PAIRING:
            /* We already have a bond with the peer, but it is attempting to
             * establish a new secure link.  This app sacrifices security for
             * convenience: just throw away the old bond and accept the new link.
             */

            ESP_LOGI(logging_tag, "BLE_GAP_EVENT_REPEAT_PAIRING\n");
            /* Delete the old bond. */
            rc = ble_gap_conn_find(event->repeat_pairing.conn_handle, &descriptor);
            assert(rc == 0);
            ble_store_util_delete_peer(&descriptor.peer_id_addr);

            /* Return BLE_GAP_REPEAT_PAIRING_RETRY to indicate that the host should
             * continue with the pairing operation.
             */
            return BLE_GAP_REPEAT_PAIRING_RETRY;

        case BLE_GAP_EVENT_PASSKEY_ACTION:
            ESP_LOGI(logging_tag, "BLE_GAP_EVENT_PASSKEY_ACTION\n");
            struct ble_sm_io pass_key = {0};

            if (event->passkey.params.action == BLE_SM_IOACT_DISP) {
                pass_key.action = event->passkey.params.action;
                pass_key.passkey = 123456; // This is the passkey to be entered on peer

                ESP_LOGI(logging_tag, "Enter passkey %d on the peer side", pass_key.passkey);
                rc = ble_sm_inject_io(event->passkey.conn_handle, &pass_key);
                ESP_LOGI(logging_tag, "ble_sm_inject_io result: %d\n", rc);

            } else if (event->passkey.params.action == BLE_SM_IOACT_NUMCMP) {
                ESP_LOGI(logging_tag, "Passkey on device's display: %d", event->passkey.params.numcmp);
                ESP_LOGI(logging_tag, "Accept or reject the passkey through console in this format -> key Y or key N");

                pass_key.action = event->passkey.params.action;
//                if (scli_receive_key(&key)) {
//                    pkey.numcmp_accept = key;
//                } else {
                pass_key.numcmp_accept = 0;
                ESP_LOGE(logging_tag, "Timeout! Rejecting the key");
//                }
                rc = ble_sm_inject_io(event->passkey.conn_handle, &pass_key);
                ESP_LOGI(logging_tag, "ble_sm_inject_io result: %d\n", rc);

            } else if (event->passkey.params.action == BLE_SM_IOACT_OOB) {
                static uint8_t tem_oob[16] = {0};
                pass_key.action = event->passkey.params.action;

                for (int i = 0; i < 16; i++) {
                    pass_key.oob[i] = tem_oob[i];
                }
                rc = ble_sm_inject_io(event->passkey.conn_handle, &pass_key);
                ESP_LOGI(logging_tag, "ble_sm_inject_io result: %d\n", rc);

            } else if (event->passkey.params.action == BLE_SM_IOACT_INPUT) {
                ESP_LOGI(logging_tag, "Enter the passkey through console in this format-> key 123456");

                pass_key.action = event->passkey.params.action;
//                if (scli_receive_key(&key)) {
//                    pkey.passkey = key;
//                } else {
                pass_key.passkey = 0;
                ESP_LOGE(logging_tag, "Timeout! Passing 0 as the key");
//                }
                rc = ble_sm_inject_io(event->passkey.conn_handle, &pass_key);
                ESP_LOGI(logging_tag, "ble_sm_inject_io result: %d\n", rc);
            }
            return 0;
        default:
            ESP_LOGI(logging_tag, "unknown server event: %d\n", event->type);
    }

    return 0;
}

static void on_host_reset(int reason) {
    MODLOG_DFLT(ERROR, "Resetting state; reason=%d\n", reason);
}

static State *global_state;
static void on_host_sync(void) {
    int rc;

    rc = ble_hs_util_ensure_addr(0);
    assert(rc == 0);

    /* Figure out address to use while advertising (no privacy for now) */
    rc = ble_hs_id_infer_auto(0, &own_address_type);
    if (rc != 0) {
        MODLOG_DFLT(ERROR, "error determining address type; rc=%d\n", rc);
        return;
    }

    /* Printing ADDR */
    uint8_t addr_val[6] = {0};
    rc = ble_hs_id_copy_addr(own_address_type, addr_val, NULL);

    MODLOG_DFLT(INFO, "Device Address: ");
    print_mac_address(addr_val);
    MODLOG_DFLT(INFO, "\n");
    /* Begin advertising. */
    advertise(global_state);
}

void bleprph_host_task(void *param) {
    ESP_LOGI(logging_tag, "BLE Host Task Started");
    /* This function will return only when nimble_port_stop() is executed */
    nimble_port_run();

    nimble_port_freertos_deinit();
}


void bluetooth_init(State *state) {
    printf("[Bluetooth] Initializing...\n");
    int rc;
    global_state = state;

    ESP_ERROR_CHECK(esp_nimble_hci_and_controller_init());

    nimble_port_init();

    /* Initialize the NimBLE host configuration. */
    ble_hs_cfg.reset_cb = on_host_reset;
    ble_hs_cfg.sync_cb = on_host_sync;
    ble_hs_cfg.gatts_register_cb = on_resource_registered;
    ble_hs_cfg.store_status_cb = ble_store_util_status_rr;

//    ble_hs_cfg.sm_io_cap = CONFIG_EXAMPLE_IO_TYPE;
#ifdef CONFIG_EXAMPLE_BONDING
    ble_hs_cfg.sm_bonding = 1;
#endif
#ifdef CONFIG_EXAMPLE_MITM
    ble_hs_cfg.sm_mitm = 1;
#endif
#ifdef CONFIG_EXAMPLE_USE_SC
    ble_hs_cfg.sm_sc = 1;
#else
    ble_hs_cfg.sm_sc = 0;   // Usage of secure connection (yes (1) or no (0))
#endif
#ifdef CONFIG_EXAMPLE_BONDING
    ble_hs_cfg.sm_our_key_dist = 1;
    ble_hs_cfg.sm_their_key_dist = 1;
#endif


    rc = initialize_server();
    assert(rc == 0);

    /* Set the default device name. */
    rc = ble_svc_gap_device_name_set(DEVICE_NAME);
    assert(rc == 0);

    /* XXX Need to have template for store */
    ble_store_config_init();

    nimble_port_freertos_init(bleprph_host_task);

    printf("[Bluetooth] Init done\n");
}