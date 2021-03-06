/* 
 * Copyright (C) 2020 Dyne.org foundation
 *
 * This file is subject to the terms and conditions of the GNU
 * General Public License (GPL) version 2. See the file LICENSE
 * for more details.
 *
 */

#include <wolfssl/ssl.h>

#include "shell.h"
#include "msg.h"
#include "nimble_scanner.h"
#include "net/bluetil/ad.h"
#include "nimble_scanlist.h"

/* default scan duration (1s) */
#define DEFAULT_DURATION        (1000000U)


void dp3t_print_entry(int *idx, nimble_scanlist_entry_t *e)
{
    char name[(BLE_ADV_PDU_LEN + 1)] = { 0 };
    char  peer_ephid[17] = { 0 };
    int res;
    bluetil_ad_t ad = BLUETIL_AD_INIT(e->ad, e->ad_len, e->ad_len);
    res = bluetil_ad_find_str(&ad, BLE_GAP_AD_NAME, name, sizeof(name));
    if (res != BLUETIL_AD_OK) {
        res = bluetil_ad_find_str(&ad, BLE_GAP_AD_NAME_SHORT, name, sizeof(name));
    }
    if (res != BLUETIL_AD_OK) {
        strncpy(name, "undefined", sizeof(name));
    }
    if (strncmp(name, "DP3T", 4) == 0) {
        res = bluetil_ad_find_str(&ad, 0xFF, peer_ephid, sizeof(peer_ephid));
        if (res != BLUETIL_AD_OK) {
            printf("[%02d] DP-3T: Invalid UUID\r\n", (*idx)++);
        } else {
            printf("[%02d] DP-3T: %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X\r\n",
                   (*idx)++,
                   peer_ephid[0], 
                   peer_ephid[1], 
                   peer_ephid[2], 
                   peer_ephid[3], 
                   peer_ephid[4], 
                   peer_ephid[5], 
                   peer_ephid[6], 
                   peer_ephid[7], 
                   peer_ephid[8], 
                   peer_ephid[9], 
                   peer_ephid[10], 
                   peer_ephid[11], 
                   peer_ephid[12], 
                   peer_ephid[13], 
                   peer_ephid[14], 
                   peer_ephid[15]); 
        }
    }
}

void dp3t_scanlist_print(void)
{
    int i = 0;
    nimble_scanlist_entry_t *e = nimble_scanlist_get_next(NULL);
    while (e) {
        dp3t_print_entry(&i, e);
        e = nimble_scanlist_get_next(e);
    }
}

int _cmd_scan(int argc, char **argv)
{
    uint32_t timeout = DEFAULT_DURATION;

    if ((argc == 2) && (memcmp(argv[1], "help", 4) == 0)) {
        printf("usage: %s [timeout in ms]\n", argv[0]);
        return 0;
    }
    if (argc >= 2) {
        timeout = (uint32_t)(atoi(argv[1]) * 1000);
    }

    nimble_scanlist_clear();
    printf("Scanning for %ums now ...", (unsigned)(timeout / 1000));
    nimble_scanner_start();
    xtimer_usleep(timeout);
    nimble_scanner_stop();
    puts(" done\n\nResults:");
    nimble_scanlist_print();
    dp3t_scanlist_print();
    puts("");

    return 0;
}

#ifdef WITH_RIOT_SOCKETS
#error RIOT-OS is set to use sockets but this DTLS app is configured for socks.
#endif

#define MAIN_QUEUE_SIZE     (8)
static msg_t _main_msg_queue[MAIN_QUEUE_SIZE];

extern int dtls_client(int argc, char **argv);
extern int dtls_server(int argc, char **argv);

#ifdef MODULE_WOLFCRYPT_TEST
extern int wolfcrypt_test(void* args);
static int wolftest(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    wolfcrypt_test(NULL);
    return 0;
}
#endif
extern int gatt_server(int argc, char *argv[]);

static const shell_command_t shell_commands[] = {
    { "dtlsc", "Start a DTLS client", dtls_client },
    { "dtlss", "Start and stop a DTLS server", dtls_server },
    { "scan", "trigger a BLE scan", _cmd_scan },
    { "srv", "Start gatt server", gatt_server },
#ifdef MODULE_WOLFCRYPT_TEST
    { "wolftest", "Perform wolfcrypt porting test", wolftest },
#endif
    { NULL, NULL, NULL }
};


int main(void)
{
    /* we need a message queue for the thread running the shell in order to
     * receive potentially fast incoming networking packets */
    msg_init_queue(_main_msg_queue, MAIN_QUEUE_SIZE);
    printf("RIOT wolfSSL DTLS testing implementation\r\n");
    wolfSSL_Init();
    wolfSSL_Debugging_ON();

    /* start shell */
    printf( "All up, running the shell now\r\n");
    char line_buf[SHELL_DEFAULT_BUFSIZE];
    shell_run(shell_commands, line_buf, SHELL_DEFAULT_BUFSIZE);
    /* should be never reached */
    return 0;
}
