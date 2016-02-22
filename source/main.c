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
#endif

#include "configread.h"

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
    sf2d_texture *app_bottom =
        sfil_load_PNG_buffer(app_bottom_png, SF2D_PLACE_RAM);

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
    while (status == LOOP_CONTINUE)
        status = callback();
    return status;
#endif
}

/*! wait until the B button is pressed (on 3DS)
 *
 *  @returns loop status
 */
static loop_status_t wait_for_b(void) {
#ifdef _3DS
    /* update button state */
    hidScanInput();

    /* check if B was pressed */
    if (hidKeysDown() & KEY_B)
        return LOOP_EXIT;

    /* B was not pressed */
    return LOOP_CONTINUE;
#else
    return LOOP_EXIT;
#endif
}

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
    sf2d_init();
    acInit();
    gfxInitDefault();
    gfxSet3D(false);
    sdmcWriteSafe(false);
#endif

    /* initialize console subsystem */
    console_init();
#ifdef _3DS
    console_set_status("\n" GREEN STATUS_STRING RESET);
#endif

    // Load config.
    int cfg = load_config_file();
    if (cfg) {
        // Error.
        printf(RED "Default config file '%s' not found. Using builtin defaults.\n" RESET, config_file);
    }

    char* root_dir = NULL; // Directory to jail to.
	int ro = 0; // Read-only. No sends are allowed.

#ifndef _3DS
    // On a linux system. Extract argv and argc if there.
    int c;
    while ( (c = getopt(argc, argv, "p:R:rC:h")) != -1) {
        switch(c) {
            case 'p':
                sscanf(optarg, "%d", &sett_port);
                break;
            case 'R':
                root_dir = optarg;
                break;
			case 'r':
				ro = 1;
				break;
            case 'C':
                config_file = optarg;
                cfg = load_config_file();
                if (cfg) {
                    printf(RED "Specified config file '%s' not found.\n" RESET, optarg);
                    goto exit_fail;
                }
                break;
			case 'h':
				printf(GREEN STATUS_STRING RESET "\n");
				printf(WHITE "Options:\n" RESET);
				printf(YELLOW "  -p " BLUE "PORT" CYAN "      Run server on port 'PORT'.\n" RESET);
				printf(YELLOW "  -R " BLUE "ROOT" CYAN "      Root directory for access. Default is `pwd`.\n" RESET);
				printf(YELLOW "  -r     " CYAN "      Read-only. Disable all uploads.\n" RESET);
				printf(YELLOW "  -C " BLUE "CFG" CYAN "       Load config file 'CFG'.\n" RESET);
				printf(YELLOW "  -h     " CYAN "      Print this help.\n" RESET);
				return 0;
				break;
            case '?':
            default:
                break;
        }
    }
#endif

    if (ro == 1) {
        printf(RED "Running in read-only mode.\n" RESET);
    }

    while (status == LOOP_RESTART) {
        /* initialize ftp subsystem */
        if (ftp_init(sett_port, root_dir, ro) == 0) {
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
#endif

    loop(wait_for_b);

#ifdef _3DS
    /* deinitialize 3DS services */
    sf2d_fini();
    gfxExit();
    acExit();
#endif

    return 0;
}
