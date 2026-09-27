#ifndef EMAIL_H
#define EMAIL_H

#ifdef __cplusplus
extern "C" {
#endif

void em_setup();
void em_open();
void em_loop();
int  em_is_active();
void em_request_open();
int  em_consume_open_request();

#ifdef __cplusplus
}
#endif

#endif