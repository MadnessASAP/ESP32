// DroneBridge display module - Header
//
// Uses an attached display module to provide feedback on drone & bridge status
// Author Michael "ASAP" Weinrich <michael@a5ap.net>

#ifndef DB_DISPLAY_H
#define DB_DISPLAY_H

typedef struct {
    volatile unsigned tx_data, rx_data;  // Accumulated bytes since the last update
    volatile unsigned tx_pkts, rx_pkts;  // Accumulated packets since the last update
    volatile unsigned num_conn;  // Number of clients connected, may not be applicable
    volatile unsigned errors;    // Number of errors encountered since last update
} db_link_status_t; 

// Significance of message being displayed, used to ensure vital messages
// take precedence over less important details.
typedef enum {
    INFO = 64,
    WARNING = 128,
    ERROR = 192
} db_err_level_t;

// Status of the DroneBridge
typedef struct {
    db_link_status_t wifi, uart;

    // Request to display a message on the display
    // param tag identifies message source
    // param level significance of the message range 0 = lowest to 255 = highest
    // param format printf style message format
    // returns 0 if message queued/displayed successfully, -1 if message queue is full
    int (*display_msg)(char * tag, db_err_level_t level, char * format, ...);
} db_status_t;

extern db_status_t db_status;

#ifdef __cplusplus
extern "C" { 
#endif
    void display_service();
#ifdef __cplusplus
}
#endif
#endif