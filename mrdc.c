#include "libstem_gamepad_1.4.2/include/gamepad/Gamepad.h"
#include "gattlib/include/gattlib.h"
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <math.h>
#include <pthread.h>

// Change/add constants here
#define L_LRINDEX    0
#define L_UDINDEX    1
#define R_LRINDEX    3
#define R_UDINDEX    4
#define INTERVAL     0.05
#define TIMEOUT      1
#define MAX_REVERSE  110
#define MIN_REVERSE  167
#define MIN_FORWARD  187
#define MAX_FORWARD  244
#define JOY_CENTER   177
#define SCALE_RANGE  67
#define SLEEP_MICROS 7500
// Some nice color defines to print stuff
#define ANSI_RED     "\x1b[31m"
#define ANSI_GREEN   "\033[1;32m"
#define ANSI_YELLOW  "\x1b[33m"
#define ANSI_BLUE    "\x1b[34m"
#define ANSI_MAGENTA "\x1b[35m"
#define ANSI_CYAN    "\x1b[36m"
#define ANSI_RESET   "\x1b[0m"

/** Bluetooth stuff.
 *    connection      - connection to bluetooth device
 *    addr            - device address
 *    handle          - characteristic handle to write to
 *    buf             - buffer to send bytes to device
 *    cur_timestamp   - used to send data at intervals
 */
static gatt_connection_t *connection;
static char *addr = "78:04:73:05:60:37";
static float cur_timeout = 0;
static uint16_t handle = 0x0025;
static unsigned char buf[4];
double cur_timestamp = 0;

// Only gamepad thing.
static struct Gamepad_device *dev;

// Keep trying to connect to arduino.
static void init_bluetooth() {
    while (connection == NULL) {
        printf("Trying to init bluetooth...\n");
        connection = gattlib_connect(NULL, addr, BDADDR_LE_PUBLIC, BT_SEC_LOW, 0, 0);
    }
    printf(ANSI_GREEN "Bluetooth initialized" ANSI_RESET "\n");
}

// Try to reconnect to bluetooth if connection ever lost.
// TODO: make work pls.
static void *reconnect(void *arg) {
    while (1) {
        usleep(1500);
        // Stuck writing, reset connection basically.
        if (cur_timeout > TIMEOUT) {
            gattlib_disconnect(connection);
            connection = NULL;
            init_bluetooth();
            cur_timeout = 0;
            continue;
        }
        cur_timeout += .001;
    }
    exit(0);
}

// Keeps trying to connect to BLE device and Gamepad
static void init_bluetooth_gamepad() {
    init_bluetooth();
    Gamepad_init();
    while (dev == NULL) {
        printf("Trying to init gamepadd...\n");
        // Only 1 xbox controller should be connected to laptop.
        dev = Gamepad_deviceAtIndex(0);
    }
    printf(ANSI_GREEN "Gamepad initialized" ANSI_RESET "\n");
}

// Get rid of any joystick noise to not send needless bytes.
static void copy_joystick_data(float *joystick_data, float *replace) {
    for (int i = 0; i < 9; i++) {
        // Make up positive, down negative for both joysticks.
        replace[i] = joystick_data[i];
        if (i == 1 || i == 4) {
            replace[i] *= -1;
        }
    }
}

// Use a pointer to not deal with left/right up/down directly in code.
// Pass in one pointer at a time.
static void parse_data(float *temp, int *ret) {
    int scale = SCALE_RANGE;
    int forward_offset = MIN_FORWARD, rev_offset = MIN_REVERSE;
    float data = *temp;
    // Check if negative first, and update ret if not NULL. Also update
    // scaling for turning to account for some stuff.
    if (data < 0 && ret != NULL) {
        *ret = 1;
    }
    if (data < 0) {
        data = data*scale + JOY_CENTER;
        if (data > rev_offset) {
            data = JOY_CENTER;
        }
    } else {
        data = data*scale + JOY_CENTER;
        if (data < forward_offset) {
            data = JOY_CENTER;
        }
    }
    *temp = data;
}

// Updates motor data based on joystick left or right. Send in global
// buffer to normalize for right joystick too.
static void lr_update(unsigned char *buffer, float *temp, int neg, int rev) {
    float data = *temp;
    int change = 0, off = 1;
    if (neg == 1) {
        change = 1; off = 0;
    }
    // Update right motor if neg == 0, left otherwise
    if ((int)data == MAX_FORWARD && rev != 0) {
        buffer[change] = MAX_REVERSE;
        buffer[off] = JOY_CENTER;
    } else if ((int)data == MAX_FORWARD) {
        buffer[change] = MAX_FORWARD;
        buffer[off] = JOY_CENTER;
    } else {
        if (buffer[change] != JOY_CENTER) {
            return;
        }
        int offset = (data - MIN_FORWARD);
        if (rev == 0) {
            if (offset + buffer[change] > MAX_FORWARD) {
                buffer[change] = MAX_FORWARD;
            } else {
                buffer[change] += offset;
            }
        } else {
            if (buffer[change] - offset < MAX_REVERSE) {
                buffer[change] = MAX_REVERSE;
            } else {
                buffer[change] -= offset;
            }
        }
    }
}

// Translates data to send to arduino.
static void translate_data(float *temp) {
    int left = 0, reverse = 0;
    // Need to know for left or right if negative, not for up/down.
    parse_data(&temp[L_LRINDEX], &left);
    parse_data(&temp[L_UDINDEX], NULL);
    /**
     *  Reverse range: 110 - 162. Any more than that = center.
     *  Forward range: 192 - 245. Any less than that = center.
     *  If temp[1] < 0, reverse. Otherwise, forward. Set both pwms
     *  to motors as same value initially.
     */
    buf[1] = temp[1];
    buf[2] = temp[1];
    if (temp[1] != JOY_CENTER && temp[0] == MAX_FORWARD) {
        temp[0] = MAX_FORWARD - 1;
    }
    if ((int)temp[2] == 1) {
        reverse = 1;
    }
    /**
     *  Turning left range: 110 - 157. Any more than that = center.
     *  Turning right range: 197 - 244. Any less than that = center.
     *  If temp[0] < 0, left. Otherwise, right.
     */
    if (temp[0] != JOY_CENTER) {
        if (left) {
            temp[0] = JOY_CENTER - temp[0] + JOY_CENTER;
        }
        lr_update(buf+1, temp, left, reverse);
    }
    // Only get up and down values for right joystick.
    parse_data(&temp[R_UDINDEX], NULL);
}

// Get data to send to the arduino, parsing to make sure sending good data.
static void get_data(struct Gamepad_device *dev, unsigned int axis, float val, float lastVal, double timestamp, void *context) {
    unsigned int axes = dev->numAxes;
    float *joy_data = dev->axisStates;
    // IMPORTANT: dev->axisStates uses previous state to modify next
    // state. Make a copy and modify copy ONLY. Length is 9.
    float temp[9];
    int length = 0, offset = 3;
    cur_timestamp = timestamp;
    copy_joystick_data(joy_data, temp);
    // Left joystick stuff, handle negatives
    translate_data(temp);
}

/**
 * Send parsed data to the arduino, another thread for it due to
 * get_data not always being called if no changes to gamepad recently.
 */
static void *send_data(void *arg) {
    while (1) {
        usleep(SLEEP_MICROS);
        cur_timeout = 0;
        printf("WRITING\n");
        gattlib_write_char_by_handle(connection, handle, buf, 4);
    }
}

int main() {
    pthread_t keep_alive, sender_thread;
    init_bluetooth_gamepad();
    Gamepad_axisMoveFunc(get_data, NULL);
    // buf[0] == left joystick, buf[3] == right joystick
    buf[0] = 0x1;
    buf[3] = 0x2;
    pthread_create(&keep_alive, NULL, reconnect, NULL);
    pthread_create(&sender_thread, NULL, send_data, NULL);
    // Main loop processing joystick input, hella cpu usage cause
    // just a while 1 loop.
    while (1) {
        Gamepad_processEvents();
    }
    Gamepad_shutdown();
    return 0;
}
