#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <poll.h>
#include <stdio.h>

#include "common.h"
#include "srvpoll.h"

void handle_client_fsm(struct dbheader_t *dbhdr, struct employee_t *employees, clientstate_t *client) {
    dbproto_hdr_t *hdr = (dbproto_hdr_t*)client->buffer;

    hdr->type = ntohl(hdr->type);
    hdr->len = ntohs(hdr->len);

    if (client->state == STATE_HELLO) {
        if (hdr->type != MSG_HELLO_REQ || hdr->len != 1) {
            printf("Didn't get MSG_HELLO in HELLO state...\n");
            // TODO: send err msg
        }

        dbproto_hello_req* hello = (dbproto_hello_req*)&hdr[1];
        hello->proto = ntohs(hello->proto);
        if (hello->proto != PROTO_VER) {
            printf("Protocol mismatch...\n");
            // TODO: send err msg
        }

        // TODO: send hello resp
        client->state = STATE_MSG;
        printf("Client upgraded to STATE_MSG\n");
    }

    if (client->state == STATE_MSG) {

    }
}

void init_clients(clientstate_t* states) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        states[i].fd = -1;
        states[i].state = STATE_NEW;
        memset(&states[i].buffer, '\0', BUFF_SIZE);
    }
}

int find_free_slot(clientstate_t* states) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (states[i].fd == -1) {
            return i;
        }
    }
    return -1;
}

int find_slot_by_fd(clientstate_t* states, int fd) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (states[i].fd == fd) {
            return i;
        }
    }
    return -1;
}