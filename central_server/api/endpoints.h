#pragma once
#include "httplib.h"

void incoming_machine_data(httplib::Server& server);
void incoming_api(httplib::Server& server);
void run_user_cli();