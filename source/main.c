#include <malloc.h>
#include <stdarg.h>
#include <stdio.h>
#ifdef _3DS
#include <3ds.h>
#endif
#include "console.h"
#include "ftp.h"

#ifdef _3DS
#include <sf2d.h>
#include <sfil.h>
#include "app_bottom_png.h"
#endif

#ifndef _3DS
#include "getopt.h"
#include <signal.h>
#endif

#include "configread.h"

#ifndef _3DS
int sigint_force_exit = 0; // DO NOT WRITE FROM MAIN THREAD.
#endif

/*! looping mechanism
 *
 *  @param[in] callback function to call during each iteration
 *
 *  @returns loop status
 */
loop_status_t loop(loop_status_t (*callback)(void)) {
    loop_status_t status = LOOP_CONTINUE;

    // This is all fine on the 3DS, but eating 100% CPU is unacceptable under linux.

#ifdef _3DS
    /**PNG Graphics (sf2d)**/
    sf2d_set_clear_color(RGBA8(0x40, 0x40, 0x40, 0xFF));

    sf2d_texture *app_bottom = NULL;
    if (sett_app_bottom_path == NULL) {
        // Image was not specified on SD, so use the builtin default.
        app_bottom = sfil_load_PNG_buffer(app_bottom_png, SF2D_PLACE_RAM);
    } else {
        // Load image from SD.
        app_bottom = sfil_load_PNG_file(sett_app_bottom_path, SF2D_PLACE_RAM);
        if (app_bottom == NULL) {
            // Load failed, so load default and log it.
            console_print(RED "Loading PNG from SD failed. Using default.\n" RESET);
            app_bottom = sfil_load_PNG_buffer(app_bottom_png, SF2D_PLACE_RAM);
        } else {
            console_print(GREEN "Loaded user image '%s'.\n" RESET, sett_app_bottom_path);
        }
    }

    while (aptMainLoop()) {
        hidScanInput();

        /**app_bottom graphic**/
        sf2d_start_frame(GFX_BOTTOM, GFX_LEFT);
        sf2d_draw_texture(app_bottom, 0, 0);
        sf2d_end_frame();

        sf2d_swapbuffers();

        status = callback();

        if (status != LOOP_CONTINUE)
            return status;
    }
    /**free graphics **/
    sf2d_free_texture(app_bottom);

    return LOOP_EXIT;
#else
    while (status == LOOP_CONTINUE) {
        status = callback();

        if(sigint_force_exit == 1) {
            status = LOOP_EXIT; // Break out.
        }
    }
    return status;
#endif
}

#ifdef _3DS
/*! wait until the B button is pressed (on 3DS)
 *
 *  @returns loop status
 */
static loop_status_t wait_for_b(void) {
    /* update button state */
    hidScanInput();

    /* check if B was pressed */
    if (hidKeysDown() & KEY_B)
        return LOOP_EXIT;

    /* B was not pressed */
    return LOOP_CONTINUE;
}
#endif

#ifndef _3DS
// Sockets leak without proper cleanup. We need to do this on Linux.
void sigint_handler(int sig) {
	console_print(YELLOW "SIGINT recieved. Cleaning up and exiting.\n" RESET);
    sigint_force_exit = 1;
}
#endif

/*! entry point
 *
 *  @param[in] argc unused
 *  @param[in] argv unused
 *
 *  returns exit status
 */
int main(int argc, char *argv[]) {
    loop_status_t status = LOOP_RESTART;

#ifdef _3DS
    /* initialize needed 3DS services */

    // Perform CPU limit init.
    aptInit();
    APT_SetAppCpuTimeLimit(0);

    sf2d_init();
    acInit();
    gfxInitDefault();
    gfxSet3D(false);
    sdmcWriteSafe(false);

    cfguInit();
#endif

#ifndef _3DS
    signal(SIGINT, sigint_handler); // Register cleanup-on-ctrl-c
#endif

    /* initialize console subsystem */
    console_init();
#ifdef _3DS
    console_set_status("\n" GREEN STATUS_STRING RESET);
#endif

    char* root_dir = NULL; // Directory to jail to.
    int con_port = -1;
    int con_read_only = 0;
    int con_disable_color = 0;
    int con_config_file = 0;

#ifndef _3DS
    // On a linux system. Extract argv and argc if there.
    int c;
    char unres_config[PATH_MAX];
    ssize_t len_unres_config = 0;
    while ( (c = getopt(argc, argv, "p:R:rC:hn")) != -1) {
        switch(c) {
            case 'p':
                sscanf(optarg, "%d", &con_port);
                break;
            case 'R':
                root_dir = optarg;
                break;
            case 'n':
                con_disable_color = 1;
                break;
			case 'r':
				con_read_only = 1;
				break;
            case 'C':
                memset(unres_config, 0, PATH_MAX);
                if (realpath(optarg, unres_config) == NULL) {
                    console_print(RED "Specified config file '%s' not found.\n" RESET, optarg);
                    goto exit_fail;
                }
                con_config_file = 1;
                break;
			case 'h':
				console_print(GREEN STATUS_STRING RESET "\n");
				console_print(WHITE "Options:\n" RESET);
				console_print(YELLOW "  -p " BLUE "PORT" CYAN "      Run server on port 'PORT'.\n" RESET);
				console_print(YELLOW "  -R " BLUE "ROOT" CYAN "      Root directory for access. Default is `pwd`.\n" RESET);
				console_print(YELLOW "  -r     " CYAN "      Read-only. Disable all uploads.\n" RESET);
				console_print(YELLOW "  -C " BLUE "CFG" CYAN "       Load config file 'CFG'.\n" RESET);
                console_print(YELLOW "  -n     " CYAN "      Disable color output.\n" RESET);
				console_print(YELLOW "  -h     " CYAN "      Print this help.\n" RESET);
				return 0;
				break;
            case '?':
            default:
                break;
        }
    }

    if (con_config_file)
        config_file = unres_config; // Full path to config.

#endif

    if (con_disable_color)
        sett_disable_color = 1; // We enable this early.

    int cfg = load_config_file();
    if (cfg && con_config_file) {
        console_print(RED "Specified config file '%s' not found.\n" RESET, config_file);
        goto exit_fail;
    } else if (cfg) {
        console_print(YELLOW "Default config file '%s' not found. Loading defaults.\n" RESET, default_config_file);
    } else {
        console_print(GREEN "Config file '%s' loaded.\n" RESET, config_file);
    }

    if (con_port != -1)
        sett_port = con_port;
    if (con_read_only)
        sett_read_only = 1;
    if (con_disable_color)
        sett_disable_color = 1;

#ifdef _3DS
    int high_clock_enabled = 0;

    // If user has an n3DS and clock rate option is set, jack up the clock.
    if (sett_high_clock_rate == 1) {
        uint8_t sysType = 0;
        CFGU_GetSystemModel(&sysType);
        if (sysType == 2 || sysType == 4) { // Only set clock rate on N3DS units.
            high_clock_enabled = 1;
            osSetSpeedupEnable(1);
            console_print(GREEN "New3DS clock rate enabled.\n" RESET);
        } else {
            console_print(YELLOW "System type '%hhu' wrong, not setting clock rate.\n" RESET, sysType);
        }
    }
#endif

    if (con_read_only) {
        console_print(RED "Running in read-only mode.\n" RESET);
    }

    while (status == LOOP_RESTART) {
        /* initialize ftp subsystem */
        if (ftp_init(sett_port, root_dir, con_read_only) == 0) {
            /* ftp loop */
            status = loop(ftp_loop);

            /* done with ftp */
            ftp_exit();
        } else
            status = LOOP_EXIT;
    }

exit_fail:

#ifdef _3DS
    console_print("Press B to exit\n");
    loop(wait_for_b);

    // reset clock rate
    if (high_clock_enabled)
        osSetSpeedupEnable(0);

    /* deinitialize 3DS services */
    cfguExit();

    sf2d_fini();
    gfxExit();
    acExit();
    aptExit();
#endif

    return 0;
}

