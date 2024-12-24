//
// Created by samuel on 2024/12/06.
//

#include <stdio.h>
#include "buttons.h"

#include "../utils.h"
#include "../peripherals/canbus/canbus.h"

void control_buttons_handle(State* state, Button button) {
    switch (button) {
        case BUTTON_NONE:
            break;
        case BUTTON_UP:
            printf("Button pressed: BUTTON_UP\n");
            if (state->display.current_screen == Screen_Menu) {
                switch (state->display.menu_option_selection) {
                    case ScreenMenuOption_CruiseControl:
                        state->display.current_screen = Screen_CruiseControl;
                        break;
                    case ScreenMenuOption_Sensors:
                        state->display.current_screen = Screen_Sensors;
                        break;
                    case ScreenMenuOption_Actions:
                        state->display.current_screen = Screen_Actions;
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
                    case ScreenActionsOptions_Reboot:
                        utils_reboot(state);
                        break;
                    default:
                        break;
                }
                break;
            }

            if (state->display.current_screen == Screen_CruiseControl) {
                state->cruise_control.enabled = true;
            }
            break;
        case BUTTON_VOLUME_UP:
            printf("Button pressed: BUTTON_VOLUME_UP\n");
            if (state->display.current_screen == Screen_Menu) {
                state->display.menu_option_selection++;
                if (state->display.menu_option_selection >= ScreenMenuOption_MAX_VALUE) {
                    state->display.menu_option_selection = 0;
                }
                break;
            }
            if (state->display.current_screen == Screen_Actions) {
                state->display.actions_option_selection++;
                if (state->display.actions_option_selection >= ScreenActionsOptions_MAX_VALUE) {
                    state->display.actions_option_selection = 0;
                }
                break;
            }

            if (state->display.current_screen == Screen_CruiseControl) {
                state->cruise_control.target_speed++;
            }
            break;
        case BUTTON_VOLUME_DOWN:
            printf("Button pressed: BUTTON_VOLUME_DOWN\n");
            if (state->display.current_screen == Screen_Menu) {
                if (state->display.menu_option_selection <= 0) {
                    state->display.menu_option_selection = ScreenMenuOption_MAX_VALUE;
                }
                state->display.menu_option_selection--;
                break;
            }
            if (state->display.current_screen == Screen_Actions) {
                if (state->display.actions_option_selection <= 0) {
                    state->display.actions_option_selection = ScreenActionsOptions_MAX_VALUE;
                }
                state->display.actions_option_selection--;
                break;
            }

            if (state->display.current_screen == Screen_CruiseControl) {
                state->cruise_control.target_speed--;
                if (state->cruise_control.target_speed < 0) {
                    state->cruise_control.target_speed = 0;
                }
            }
            break;
        case BUTTON_SOURCE:
            printf("Button pressed: BUTTON_SOURCE\n");
            if (state->display.current_screen == Screen_Menu) {
                state->display.menu_option_selection = 0;
                break;
            }
            if (state->display.current_screen == Screen_Actions) {
                state->display.actions_option_selection = 0;
                break;
            }

            if ((!state->cruise_control.enabled && state->display.current_screen == Screen_CruiseControl) ||
                state->display.current_screen == Screen_Sensors ||
                state->display.current_screen == Screen_Actions) {
                state->display.current_screen = Screen_Menu;
            } else if (state->display.current_screen == Screen_CruiseControl) {
                if (state->cruise_control.enabled) printf("Disconnecting cruise control because of user input\n");
                state->cruise_control.enabled = false;
            }
            break;
        case BUTTON_SOURCE_LONG_PRESS: printf("Button pressed: BUTTON_SOURCE_LONG_PRESS\n");
            break;
        case BUTTON_INFO: printf("Button pressed: BUTTON_INFO\n");
            break;
        case BUTTON_DOWN: printf("Button pressed: BUTTON_DOWN\n");
            break;
        case BUTTON_VOLUME_UP_LONG_PRESS:
            printf("Button pressed: BUTTON_VOLUME_UP_LONG_PRESS\n");

            state->cruise_control.target_speed += 10 - 1;
            break;
        case BUTTON_VOLUME_DOWN_LONG_PRESS:
            printf("Button pressed: BUTTON_VOLUME_DOWN_LONG_PRESS\n");
            state->cruise_control.target_speed -= 10 - 1;
            if (state->cruise_control.target_speed < 0) {
                state->cruise_control.target_speed = 0;
            }
            break;
        case BUTTON_INFO_LONG_PRESS: printf("Button pressed: BUTTON_INFO_LONG_PRESS\n");
            break;
        case BUTTON_UP_LONG_PRESS:
            printf("Button pressed: BUTTON_UP_LONG_PRESS\n");
            state->cruise_control.target_speed = state->cruise_control.previous_target_speed;
            break;
        case BUTTON_DOWN_LONG_PRESS: printf("Button pressed: BUTTON_DOWN_LONG_PRESS\n");
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

void control_buttons_handle_pid_config(State* state, Button button) {
    static double pid_increase_step = 0.01;
    static PidIncreaseTarget pid_increase_target = PidProportional;

    // Temporary extra's for debugging PID
    switch (button) {
        case BUTTON_VOLUME_UP:
            if (!state->cruise_control.enabled) {
                if (pid_increase_target == PidProportional) {
                    state->cruise_control.pidKp += pid_increase_step;
                    printf("state->cruise_control.pidKp = %lf\n", state->cruise_control.pidKp);
                } else if (pid_increase_target == PidIntegral) {
                    state->cruise_control.pidKi += pid_increase_step;
                    printf("state->cruise_control.pidKi = %lf\n", state->cruise_control.pidKi);
                } else if (pid_increase_target == PidDerivative) {
                    state->cruise_control.pidKd += pid_increase_step;
                    printf("state->cruise_control.pidKd = %lf\n", state->cruise_control.pidKd);
                }
            }
            break;
        case BUTTON_VOLUME_DOWN:
            if (!state->cruise_control.enabled) {
                if (pid_increase_target == PidProportional) {
                    state->cruise_control.pidKp -= pid_increase_step;
                    printf("state->cruise_control.pidKp = %lf\n", state->cruise_control.pidKp);
                } else if (pid_increase_target == PidIntegral) {
                    state->cruise_control.pidKi -= pid_increase_step;
                    printf("state->cruise_control.pidKi = %lf\n", state->cruise_control.pidKi);
                } else if (pid_increase_target == PidDerivative) {
                    state->cruise_control.pidKd -= pid_increase_step;
                    printf("state->cruise_control.pidKd = %lf\n", state->cruise_control.pidKd);
                }
            }
            break;
        case BUTTON_SOURCE_LONG_PRESS:
            if (pid_increase_target == PidProportional) {
                pid_increase_target = PidIntegral;
                printf("pid_increase_target = PidIntegral\n");
            } else if (pid_increase_target == PidIntegral) {
                pid_increase_target = PidDerivative;
                printf("pid_increase_target = PidDerivative\n");
            } else {
                pid_increase_target = PidProportional;
                printf("pid_increase_target = PidProportional\n");
            }
            break;
        case BUTTON_VOLUME_UP_LONG_PRESS:
            if (!state->cruise_control.enabled) {
                pid_increase_step *= 10;
            }
            break;
        case BUTTON_VOLUME_DOWN_LONG_PRESS:
            if (!state->cruise_control.enabled) {
                pid_increase_step *= 0.1;
            }
            break;
        default:
            break;
    }
}
