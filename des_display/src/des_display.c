#include "../../des_controller/src/des.h"
#include <sys/neutrino.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main() {
    int chid;
    int rcvid;
    Message msg;

    // Phase I: Create a channel for the controller to attach
    chid = ChannelCreate(0);
    if (chid == -1) {
        perror("ChannelCreate failed");
        return EXIT_FAILURE;
    }

    printf("Display running. PID: %d\n", getpid());

    // Phase II: Main message loop
    while (1) {
        rcvid = MsgReceive(chid, &msg, sizeof(msg), NULL);
        if (rcvid == -1) {
            perror("MsgReceive failed");
            continue;
        }
        // Acknowledge receipt
        MsgReply(rcvid, EOK, NULL, 0);

        // Process message
        switch (msg.event) {
            case EVENT_LEFT_SCAN:
            case EVENT_RIGHT_SCAN:
                printf("Person scanned at %s entrance, ID: %d\n",
                       (msg.event == EVENT_LEFT_SCAN) ? "Left" : "Right",
                       msg.person_id);
                break;

            case EVENT_WEIGHT_SCALE:
                printf("Person weighed: %d kg\n", msg.weight);
                break;

            case EVENT_EXIT:
                goto cleanup;

            default:
                printf("%s\n", msg.status);
                break;
        }
    }

cleanup:
    // Phase III: Destroy channel and exit
	printf("Exiting Display\n");
    ChannelDestroy(chid);
    return EXIT_SUCCESS;
}
