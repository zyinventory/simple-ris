#pragma once

int load_config(BIO *err, CONF *cnf);
int password_callback(char *buf, int bufsiz, int verify, PW_CB_DATA *cb_tmp);
int setup_ui_method(void);
void destroy_ui_method(void);
