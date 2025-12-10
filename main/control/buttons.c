//
// Created by samuel on 2024/12/06.
//

#include <stdio.h>
#include "buttons.h"

#include "cruise_control.h"
#include "../utils.h"
#include "../peripherals/canbus/canbus.h"
#include "../peripherals/gpsgsm/gpsgsm.h"
#include "../error_codes.h"

void next_option(int *option, int max) {
    (*option)++;
    if (*option >= max) {
        *option = 0;
    }
}

void previous_option(int *option, int max) {
    if (*option <= 0) {
        *option = max;
    }
    (*option)--;
}

void control_buttons_handle(State *state, Button button) {
    switch (button) {
        case BUTTON_NONE:
            break;
        case BUTTON_CONFIRM:
            printf("Button pressed: BUTTON_UP\n");
            if (state->display.current_screen == Screen_Menu) {
                switch (state->display.menu_option_selection) {
                    case ScreenMenuOption_CruiseControl:
                        state->display.current_screen = Screen_Speed;
                        break;
                    case ScreenMenuOption_Sensors:
                        state->display.current_screen = Screen_Sensors;
                        break;
                    case ScreenMenuOption_Actions:
                        state->display.current_screen = Screen_Actions;
                        break;
                    case ScreenMenuOption_About:
                        state->display.current_screen = Screen_About;
                        break;
                    default:
                        break;
                }
                break;
            }
            if (state->display.current_screen == Screen_Actions) {
                switch (state->display.actions_option_selection) {
                    case ScreenActionsOptions_LockDoors:
                        canbus_send_lock_doors(state, true);
                        break;
                    case ScreenActionsOptions_ActivateSeatbelt:
                        canbus_send_seatbelt_message(state, true);
                        break;
                    case ScreenActionsOptions_ActivateDiagnostics:
                        state->diagnostics.status = DiagnosticsStep_Off + 1;
                        break;
                    case ScreenActionsOptions_ActivateSim:
#ifdef SIM_ACTIVATE_CONTACT_NUMBER
                        gsm_send_sms(&(state->gsm), SIM_ACTIVATE_CONTACT_NUMBER, "Carcomputer SIM activation");
#else
                        set_error(state, ERROR_SMS_FAILED);
#endif
                        break;
                    case ScreenActionsOptions_Reboot:
                        utils_reboot(state);
                        break;
                    case ScreenActionsOptions_PidConfig:
                        state->display.current_screen = Screen_PidConfig;
                        state->display.pid_config.selected_type = 0;
                        break;
                    default:
                        break;
                }
                break;
            }
            if (state->display.current_screen == Screen_PidConfig) {
                if (state->display.subscreen.pid_config == SubScreenPidConfig_Select) {
                    if (state->display.pid_config.selected_type == PidConfigScreenOptionsType_SpeedLimiter) {
                        next_option((int *) &state->speed_control.cruise_control.speed_limiter_type, CruiseControlSpeedLimiterType_MAX_VALUE);
                    } else {
                        state->display.subscreen.pid_config = SubScreenPidConfig_Edit;
                    }
                } else if (state->display.subscreen.pid_config == SubScreenPidConfig_Edit) {
                    if (state->display.pid_config.edit_selection == PidConfigScreenOptionsEditField_Factor) {
                        next_option((int *) &state->display.pid_config.edit_selection, PidConfigScreenOptionsEditField_MAX_VALUE);
                    }
                }
            }

            if (state->display.current_screen == Screen_Speed) {
                if (state->display.subscreen.speed == SubScreenSpeed_CruiseControl) {
                    if (!state->speed_control.cruise_control.enabled) {
                        state->speed_control.pedal_control.enabled = false;
                        state->speed_control.cruise_control.enabled = true;
                        state->display.subscreen.cruise_control = 0;
                    } else {
                        state->speed_control.cruise_control.eta_target_speed = state->speed_control.cruise_control.target_speed;
                        next_option((int *) &state->display.subscreen.cruise_control, SubScreenCruiseControl_MAX_VALUE);
                    }
                } else if (state->display.subscreen.speed == SubScreenSpeed_PedalControl) {
                    state->speed_control.pedal_control.enabled = true;
                    state->speed_control.cruise_control.enabled = false;
                }
            }
            break;
        case BUTTON_INCREASE:
            printf("Button pressed: BUTTON_VOLUME_UP\n");
            if (state->display.current_screen == Screen_Menu) {
                previous_option((int *) &state->display.menu_option_selection, ScreenMenuOption_MAX_VALUE);
                break;
            }
            if (state->display.current_screen == Screen_Actions) {
                previous_option((int *) &state->display.actions_option_selection, ScreenActionsOptions_MAX_VALUE);
                break;
            }
            if (state->display.current_screen == Screen_Sensors) {
                previous_option((int *) &state->display.subscreen.sensors, ScreenSensors_MAX_VALUE);
                break;
            }
            if (state->display.current_screen == Screen_About) {
                previous_option((int *) &state->display.subscreen.about, ScreenAbout_MAX_VALUE);
                break;
            }
            if (state->display.current_screen == Screen_PidConfig) {
                if (state->display.subscreen.pid_config == SubScreenPidConfig_Select) {
                    previous_option((int *) &state->display.pid_config.selected_type, PidConfigScreenOptionsType_MAX_VALUE);
                } else if (state->display.subscreen.pid_config == SubScreenPidConfig_Edit) {
                    if (state->display.pid_config.edit_selection == PidConfigScreenOptionsEditField_Factor) {
                        state->display.pid_config.factor++;
                    } else if (state->display.pid_config.edit_selection == PidConfigScreenOptionsEditField_Apply) {
                        cruise_control_config_apply_factor(state, 1);
                    }
                }
                break;
            }

            if (state->display.current_screen == Screen_Speed) {
                if (state->speed_control.cruise_control.enabled) {
                    if (state->display.subscreen.cruise_control == SubScreenCruiseControl_ETA) {
                        printf("Increaser cruise control eta\n");
                        state->speed_control.cruise_control.eta_target_speed++;
                    } else {
                        printf("Increaser cruise control\n");
                        state->speed_control.cruise_control.target_speed++;
                    }
                } else if (state->speed_control.pedal_control.enabled) {
                    printf("Increaser pedal control\n");
                    state->speed_control.pedal_control.target_value += 0.01;
                    if (state->speed_control.pedal_control.target_value > 1) {
                        state->speed_control.pedal_control.target_value = 1;
                    }
                } else {
                    printf("Increaser screen\n");
                    previous_option((int *) &state->display.subscreen.speed, SubScreenSpeed_MAX_VALUE);
                }
            }
            break;
        case BUTTON_DECREASE:
            printf("Button pressed: BUTTON_VOLUME_DOWN\n");
            if (state->display.current_screen == Screen_Menu) {
                next_option((int *) &state->display.menu_option_selection, ScreenMenuOption_MAX_VALUE);
                break;
            }
            if (state->display.current_screen == Screen_Actions) {
                next_option((int *) &state->display.actions_option_selection, ScreenActionsOptions_MAX_VALUE);
                break;
            }
            if (state->display.current_screen == Screen_Sensors) {
                next_option((int *) &state->display.subscreen.sensors, ScreenSensors_MAX_VALUE);
                break;
            }
            if (state->display.current_screen == Screen_About) {
                next_option((int *) &state->display.subscreen.about, ScreenAbout_MAX_VALUE);
                break;
            }
            if (state->display.current_screen == Screen_PidConfig) {
                if (state->display.subscreen.pid_config == SubScreenPidConfig_Select) {
                    next_option((int *) &state->display.pid_config.selected_type, PidConfigScreenOptionsType_MAX_VALUE);
                } else if (state->display.subscreen.pid_config == SubScreenPidConfig_Edit) {
                    if (state->display.pid_config.edit_selection == PidConfigScreenOptionsEditField_Factor) {
                        state->display.pid_config.factor--;
                    } else if (state->display.pid_config.edit_selection == PidConfigScreenOptionsEditField_Apply) {
                        cruise_control_config_apply_factor(state, -1);
                    }
                }
                break;
            }

            if (state->display.current_screen == Screen_Speed) {
                if (state->speed_control.cruise_control.enabled) {
                    if (state->display.subscreen.cruise_control == SubScreenCruiseControl_ETA) {
                        state->speed_control.cruise_control.eta_target_speed--;
                        if (state->speed_control.cruise_control.eta_target_speed < 0) {
                            state->speed_control.cruise_control.eta_target_speed = 0;
                        }
                    } else {
                        state->speed_control.cruise_control.target_speed--;
                        if (state->speed_control.cruise_control.target_speed < 0) {
                            state->speed_control.cruise_control.target_speed = 0;
                        }
                    }
                } else if (state->speed_control.pedal_control.enabled) {
                    state->speed_control.pedal_control.target_value -= 0.01;
                    if (state->speed_control.pedal_control.target_value < 0) {
                        state->speed_control.pedal_control.target_value = 0;
                    }
                } else {
                    next_option((int *) &state->display.subscreen.speed, SubScreenSpeed_MAX_VALUE);
                }
            }
            break;
        case BUTTON_CANCEL:
            printf("Button pressed: BUTTON_SOURCE\n");
            if (state->display.current_screen == Screen_Menu) {
                state->display.menu_option_selection = 0;
                break;
            }
            if (state->display.current_screen == Screen_Actions) {
                state->display.current_screen = Screen_Menu;
                state->display.actions_option_selection = 0;
                break;
            }
            if (state->display.current_screen == Screen_PidConfig) {
                if (state->display.subscreen.pid_config == SubScreenPidConfig_Select) {
                    state->display.current_screen = Screen_Actions;
                } else if (state->display.subscreen.pid_config == SubScreenPidConfig_Edit) {
                    if (state->display.pid_config.edit_selection == PidConfigScreenOptionsEditField_Factor) {
                        state->display.subscreen.pid_config = SubScreenPidConfig_Select;
                    } else if (state->display.pid_config.edit_selection == PidConfigScreenOptionsEditField_Apply) {
                        previous_option((int *) &state->display.pid_config.edit_selection, PidConfigScreenOptionsEditField_MAX_VALUE);
                    }
                }
                break;
            }
            if (state->display.current_screen == Screen_ActivateDiagnostics) {
                state->diagnostics.status = DiagnosticsStep_Off;
                break;
            }

            if (state->speed_control.cruise_control.enabled || state->speed_control.pedal_control.enabled) {
                if (state->speed_control.cruise_control.enabled) printf("Disconnecting cruise control because of user input\n");
                if (state->speed_control.pedal_control.enabled) printf("Disconnecting pedal control because of user input\n");
                state->speed_control.cruise_control.enabled = false;
                state->speed_control.pedal_control.enabled = false;
            } else if (state->display.current_screen == Screen_Speed
                       || state->display.current_screen == Screen_Sensors
                       || state->display.current_screen == Screen_Actions
                       || state->display.current_screen == Screen_About
            ) {
                state->display.current_screen = Screen_Menu;
            }
            break;
        case BUTTON_SOURCE_LONG_PRESS:
            printf("Button pressed: BUTTON_SOURCE_LONG_PRESS\n");
            break;
        case BUTTON_INFO:
            printf("Button pressed: BUTTON_INFO\n");
            break;
        case BUTTON_DOWN:
            printf("Button pressed: BUTTON_DOWN\n");
            break;
        case BUTTON_VOLUME_UP_LONG_PRESS:
            printf("Button pressed: BUTTON_VOLUME_UP_LONG_PRESS\n");

            if (state->speed_control.cruise_control.enabled) {
                if (state->display.subscreen.cruise_control == SubScreenCruiseControl_ETA) {
                    state->speed_control.cruise_control.eta_target_speed += 10 - 1;
                } else {
                    state->speed_control.cruise_control.target_speed += 10 - 1;
                }
            } else if (state->speed_control.pedal_control.enabled) {
                state->speed_control.pedal_control.target_value += 0.09;
                if (state->speed_control.pedal_control.target_value > 1) {
                    state->speed_control.pedal_control.target_value = 1;
                }
            }
            break;
        case BUTTON_VOLUME_DOWN_LONG_PRESS:
            printf("Button pressed: BUTTON_VOLUME_DOWN_LONG_PRESS\n");
            if (state->speed_control.cruise_control.enabled) {
                if (state->display.subscreen.cruise_control == SubScreenCruiseControl_ETA) {
                    state->speed_control.cruise_control.eta_target_speed -= 10 - 1;
                    if (state->speed_control.cruise_control.eta_target_speed < 0) {
                        state->speed_control.cruise_control.eta_target_speed = 0;
                    }
                } else {
                    state->speed_control.cruise_control.target_speed -= 10 - 1;
                    if (state->speed_control.cruise_control.target_speed < 0) {
                        state->speed_control.cruise_control.target_speed = 0;
                    }
                }
            } else if (state->speed_control.pedal_control.enabled) {
                state->speed_control.pedal_control.target_value -= 0.09;
                if (state->speed_control.pedal_control.target_value < 0) {
                    state->speed_control.pedal_control.target_value = 0;
                }
            }
            break;
        case BUTTON_INFO_LONG_PRESS:
            printf("Button pressed: BUTTON_INFO_LONG_PRESS\n");
            break;
        case BUTTON_UP_LONG_PRESS:
            printf("Button pressed: BUTTON_UP_LONG_PRESS\n");
            state->speed_control.cruise_control.target_speed = state->speed_control.cruise_control.previous_target_speed;
            break;
        case BUTTON_DOWN_LONG_PRESS:
            printf("Button pressed: BUTTON_DOWN_LONG_PRESS\n");
            break;
        default:
            break;
    }
}

typedef enum {
    PidProportional = 0,
    PidIntegral,
    PidDerivative,
} PidIncreaseTarget;
