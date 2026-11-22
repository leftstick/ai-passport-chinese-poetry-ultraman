// 奥特曼诗词收集冒险: slow-paced collection adventure on the AI Passport.
#pragma once

#include "bsp_button.h"
#include "esp_err.h"

esp_err_t adventure_app_start(void);
void adventure_app_handle_key(bsp_btn_t btn, bsp_btn_ev_t event);
