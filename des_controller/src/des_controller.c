#include "des.h"
#include <sys/neutrino.h>
#include <errno.h>
#include <stdbool.h>

State currentState = STATE_IDLE;
int display_pid;  // PID of the des_display process
int coid;
int doors;
bool left;

// Function prototypes for each state handler
State handle_idle(Message *msg);
State handle_wait_for_guard_unlock(Message *msg);
State handle_wait_for_guard_lock(Message *msg);
State handle_wait_for_door_open(Message *msg);
State handle_wait_for_scale(Message *msg);
State handle_wait_for_door_close(Message *msg);
State handle_scan(Message *msg);

// Function pointer for state transitions
State (*stateHandler)(Message *) = handle_idle;

// Send system status updates to `des_display`
void send_status_update(Message *msg) {
    MsgSend(coid, msg, sizeof(Message), NULL, 0);  // Send the message to the display process
}

// State Handlers
State handle_idle(Message *msg) {
	doors = 0;
    if (msg->event == EVENT_LEFT_SCAN || msg->event == EVENT_RIGHT_SCAN) {
        // If it's a scan event, transition to the scan state
        return handle_scan(msg);
    }
    return STATE_IDLE;
}

State handle_scan(Message *msg) {
    // Check the scan direction and set the status accordingly
    if (msg->event == EVENT_LEFT_SCAN) {
    	left = true;
        snprintf(msg->status, MAX_MSG_SIZE, "%s %d", outMessage[2], msg->person_id);
    } else if (msg->event == EVENT_RIGHT_SCAN) {
    	left = false;
        snprintf(msg->status, MAX_MSG_SIZE, "%s %d", outMessage[3], msg->person_id);
    }
    send_status_update(msg);  // Send status update to display

    msg->event = EVENT_STATUS;
    // After scanning, transition to the next state
    snprintf(msg->status, MAX_MSG_SIZE, "%s", outMessage[4]);
    send_status_update(msg);
    return STATE_WAIT_FOR_GUARD_UNLOCK;
}

State handle_wait_for_guard_unlock(Message *msg) {
	if (msg->event == EVENT_GUARD_LEFT_UNLOCK) {
		if (left == true) {
			snprintf(msg->status, MAX_MSG_SIZE, "%s", outMessage[5]);
		} else {
		return STATE_WAIT_FOR_GUARD_UNLOCK;
		}
	} else if (msg->event == EVENT_GUARD_RIGHT_UNLOCK) {
		if (left == false) {
			snprintf(msg->status, MAX_MSG_SIZE, "%s", outMessage[6]);
		} else {
			return STATE_WAIT_FOR_GUARD_UNLOCK;
		}
	}
    send_status_update(msg);
    return STATE_WAIT_FOR_DOOR_OPEN;
}

State handle_wait_for_guard_lock(Message *msg) {
    if (msg->event == EVENT_GUARD_LEFT_LOCK && left) {
    	if (doors == 0) {
    		snprintf(msg->status, MAX_MSG_SIZE, "%s", outMessage[7]);
    		left = false;
    	} else {
    		snprintf(msg->status, MAX_MSG_SIZE, "%s", outMessage[21]);
    	}
    } else if (msg->event == EVENT_GUARD_RIGHT_LOCK && !left) {
    	if (doors == 0) {
    		snprintf(msg->status, MAX_MSG_SIZE, "%s", outMessage[8]);
    		left = true;\
    	} else {
    		snprintf(msg->status, MAX_MSG_SIZE, "%s", outMessage[20]);
    	}
    } else {
        return STATE_WAIT_FOR_GUARD_LOCK;  // Stay in this state until we get the correct event
    }

    send_status_update(msg);
    doors++;

    if (doors == 1) {
        return STATE_WAIT_FOR_GUARD_UNLOCK;  // Go back to unlocking after first lock
    }
    return STATE_IDLE;  // If two locks are completed, go to idle
}


State handle_wait_for_door_open(Message *msg) {
    if (msg->event == EVENT_LEFT_OPEN) {
    	if (left == true) {
    		if (doors == 0) {
    			snprintf(msg->status, MAX_MSG_SIZE, "%s", outMessage[9]);
    		} else {
    			snprintf(msg->status, MAX_MSG_SIZE, "%s", outMessage[18]);
    		}
    	} else {
    		return STATE_WAIT_FOR_DOOR_OPEN;
    	}
    } else if (msg->event == EVENT_RIGHT_OPEN) {
    	if (left == false) {
    		if (doors == 0) {
    			snprintf(msg->status, MAX_MSG_SIZE, "%s", outMessage[10]);
    		} else {
    			snprintf(msg->status, MAX_MSG_SIZE, "%s", outMessage[19]);
    		}
    	} else {
    		return STATE_WAIT_FOR_DOOR_OPEN;
    	}
    }
        send_status_update(msg);
        return (doors < 1) ? STATE_WAIT_FOR_SCALE : STATE_WAIT_FOR_DOOR_CLOSE;
}

State handle_wait_for_scale(Message *msg) {
    if (msg->event == EVENT_WEIGHT_SCALE) {
        snprintf(msg->status, MAX_MSG_SIZE, "%s %d kg", outMessage[12], msg->weight);
        send_status_update(msg);
        msg->event = EVENT_STATUS;
        if (left == true) {
        	snprintf(msg->status, MAX_MSG_SIZE, "%s", outMessage[13]);
        } else {
        	snprintf(msg->status, MAX_MSG_SIZE, "%s", outMessage[14]);
        }
        send_status_update(msg);
        return STATE_WAIT_FOR_DOOR_CLOSE;
    }
    return STATE_WAIT_FOR_SCALE;
}

State handle_wait_for_door_close(Message *msg) {
    if (msg->event == EVENT_LEFT_CLOSED) {
    	if (left == true) {
    		snprintf(msg->status, MAX_MSG_SIZE, "%s", outMessage[16]);
    	} else {
    		   return STATE_WAIT_FOR_DOOR_CLOSE;
    	}
    } else if (msg->event == EVENT_RIGHT_CLOSED) {
    	if (left == false) {
    		snprintf(msg->status, MAX_MSG_SIZE, "%s", outMessage[17]);
    	}else {
    		   return STATE_WAIT_FOR_DOOR_CLOSE;
    	}
    }
        send_status_update(msg);
        return STATE_WAIT_FOR_GUARD_LOCK;
}

// Main Controller Process
int main(int argc, char *argv[]) {
    // Phase I: Get display's PID from command-line arguments
    if (argc != 2) {
        printf("Usage: %s <display_pid>\n", argv[0]);
        return EXIT_FAILURE;
    }
    display_pid = atoi(argv[1]);

    printf("Controller running. PID: %d\n", getpid());

    // Create a channel for the inputs process to attach
    int chid = ChannelCreate(0);
    if (chid == -1) {
        perror("ChannelCreate failed");
        return EXIT_FAILURE;
    }

    // Attach to the display's channel
    coid = ConnectAttach(0, display_pid, 1, 0, 0);
    if (coid == -1) {
    	perror("ConnectAttach failed");
    	return EXIT_FAILURE;
    }

    printf("Controller's PID: %d\n", getpid());

    // Phase II: Main loop to handle messages from inputs
    int rcvid;
    Message msg;
    while (1) {
    	// Phase II: Main loop to handle messages from inputs
    	int rcvid;
    	Message msg;
    	while (1) {
    	    rcvid = MsgReceive(chid, &msg, sizeof(msg), NULL);
    	    if (rcvid == -1) {
    	        perror("MsgReceive failed");
    	        continue;
    	    }

    	    if (msg.event == EVENT_EXIT) {
    	        // Send a reply before exiting to avoid blocking sender
    	        MsgReply(rcvid, EOK, NULL, 0);

    	        printf("Exiting Controller\n");  // Ensure this prints
    	        send_status_update(&msg);
    	        ChannelDestroy(chid);
    	        ConnectDetach(coid);
    	        return EXIT_SUCCESS;
    	    }

    	    // Transition state based on message event
    	    currentState = stateHandler(&msg);

    	    // Update state handler for the next state
    	    switch (currentState) {
    	        case STATE_IDLE: stateHandler = handle_idle; printf("Waiting for Person..."); fflush(stdout); break;
    	        case STATE_WAIT_FOR_GUARD_UNLOCK: stateHandler = handle_wait_for_guard_unlock; break;
    	        case STATE_WAIT_FOR_GUARD_LOCK: stateHandler = handle_wait_for_guard_lock; break;
    	        case STATE_WAIT_FOR_DOOR_OPEN: stateHandler = handle_wait_for_door_open; break;
    	        case STATE_WAIT_FOR_SCALE: stateHandler = handle_wait_for_scale; break;
    	        case STATE_WAIT_FOR_DOOR_CLOSE: stateHandler = handle_wait_for_door_close; break;
    	        default: stateHandler = handle_idle; break;
    	    }

    	    MsgReply(rcvid, EOK, NULL, 0);
    	}

    }
}


