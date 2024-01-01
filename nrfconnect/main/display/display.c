#include <lvgl.h>
#include <memory.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <zephyr/device.h>
#include <zephyr/drivers/display.h>
#include <zephyr/kernel.h>
#include <zephyr/pm/pm.h>
#include <zephyr/sys/printk.h>

#include "display.h"
#include "images.h"

#define SCREEN_WIDTH 800
#define SCREEN_HEIGTH 480
#define PADDING 20
#define WINDOW_WIDTH ((SCREEN_WIDTH - (3 * PADDING)) / 2)
#define WINDOW_HEIGHT (SCREEN_HEIGTH - (2 * PADDING))

#define TIME_TABLE_LENGTH 10
#define APPOINTMENT_SLOTS (18 - 8) * (60 / 15) // between 8am and 18pm one slot every 15 minutes

#define CONFIG_LVGL_DISPLAY_DEV_NAME "gdew075t7"

#define MAX_AMOUNT_DELIMITERS 7 + 10 * 5 // 7 delimiters without entries + 5 separators per entry with approximately max 10 entries

static const struct device * display_dev;

// Styles
const lv_style_const_prop_t style_window_props[] = { LV_STYLE_CONST_BORDER_WIDTH(0),
                                                     LV_STYLE_CONST_TEXT_FONT(&lv_font_montserrat_26),
                                                     LV_STYLE_CONST_BORDER_SIDE(LV_BORDER_SIDE_NONE),
                                                     PAD_TOP,
                                                     PAD_BOTTOM,
                                                     PAD_LEFT,
                                                     PAD_RIGHT,
                                                     OPA,
                                                     BG_COLOR,
                                                     BG_GRAD_COLOR,
                                                     RADIUS,
                                                     BORDER_COLOR,
                                                     BORDER_OPA,
                                                     BORDER_SIDE,
                                                     SHADOW_COLOR,
                                                     SHADOW_WIDTH,
                                                     TEXT_OPA,
                                                     TEXT_COLOR,
                                                     TEXT_LETTER_SPACE,
                                                     TEXT_LINE_SPACE,
                                                     IMG_OPA,
                                                     IMG_RECOLOR,
                                                     LINE_OPA,
                                                     LINE_COLOR,
                                                     LINE_WIDTH,
                                                     LINE_ROUNDED };
LV_STYLE_CONST_INIT(style_window, style_window_props); // style for the calendar

const lv_style_const_prop_t style_window_room_props[] = { LV_STYLE_CONST_BORDER_WIDTH(3),
                                                          LV_STYLE_CONST_TEXT_FONT(&lv_font_montserrat_30),
                                                          LV_STYLE_CONST_BORDER_SIDE(LV_BORDER_SIDE_FULL),

                                                          PAD_TOP,
                                                          PAD_BOTTOM,
                                                          PAD_LEFT,
                                                          PAD_RIGHT,
                                                          OPA,
                                                          BG_COLOR,
                                                          BG_GRAD_COLOR,
                                                          RADIUS,
                                                          BORDER_COLOR,
                                                          BORDER_OPA,
                                                          BORDER_SIDE,
                                                          SHADOW_COLOR,
                                                          SHADOW_WIDTH,
                                                          TEXT_OPA,
                                                          TEXT_COLOR,
                                                          TEXT_LETTER_SPACE,
                                                          TEXT_LINE_SPACE,
                                                          IMG_OPA,
                                                          IMG_RECOLOR,
                                                          LINE_OPA,
                                                          LINE_COLOR,
                                                          LINE_WIDTH,
                                                          LINE_ROUNDED };
LV_STYLE_CONST_INIT(style_window_room, style_window_room_props); // style for the room window

const lv_style_const_prop_t style_status_window_props[] = { LV_STYLE_CONST_BORDER_SIDE(LV_BORDER_SIDE_FULL),
                                                            LV_STYLE_CONST_BORDER_WIDTH(3),

                                                            PAD_TOP,
                                                            PAD_BOTTOM,
                                                            PAD_LEFT,
                                                            PAD_RIGHT,
                                                            OPA,
                                                            BG_COLOR,
                                                            BG_GRAD_COLOR,
                                                            RADIUS,
                                                            BORDER_COLOR,
                                                            BORDER_OPA,
                                                            BORDER_SIDE,
                                                            SHADOW_COLOR,
                                                            SHADOW_WIDTH,
                                                            TEXT_OPA,
                                                            TEXT_COLOR,
                                                            TEXT_LETTER_SPACE,
                                                            TEXT_LINE_SPACE,
                                                            IMG_OPA,
                                                            IMG_RECOLOR,
                                                            LINE_OPA,
                                                            LINE_COLOR,
                                                            LINE_WIDTH,
                                                            LINE_ROUNDED,
                                                            TEXT_FONT };
LV_STYLE_CONST_INIT(style_status_window, style_status_window_props); // style for the status window

const lv_style_const_prop_t style_time_slot_props[] = { LV_STYLE_CONST_BORDER_WIDTH(2),
                                                        LV_STYLE_CONST_BORDER_SIDE(LV_BORDER_SIDE_TOP),

                                                        PAD_TOP,
                                                        PAD_BOTTOM,
                                                        PAD_LEFT,
                                                        PAD_RIGHT,
                                                        OPA,
                                                        BG_COLOR,
                                                        BG_GRAD_COLOR,
                                                        RADIUS,
                                                        BORDER_COLOR,
                                                        BORDER_OPA,
                                                        BORDER_SIDE,
                                                        SHADOW_COLOR,
                                                        SHADOW_WIDTH,
                                                        TEXT_OPA,
                                                        TEXT_COLOR,
                                                        TEXT_LETTER_SPACE,
                                                        TEXT_LINE_SPACE,
                                                        IMG_OPA,
                                                        IMG_RECOLOR,
                                                        LINE_OPA,
                                                        LINE_COLOR,
                                                        LINE_WIDTH,
                                                        LINE_ROUNDED,
                                                        TEXT_FONT };
LV_STYLE_CONST_INIT(style_time_slot, style_time_slot_props); // style for the time slots behind the entries

const lv_style_const_prop_t style_line_props[] = { LV_STYLE_CONST_LINE_WIDTH(1),

                                                   PAD_TOP,
                                                   PAD_BOTTOM,
                                                   PAD_LEFT,
                                                   PAD_RIGHT,
                                                   OPA,
                                                   BG_COLOR,
                                                   BG_GRAD_COLOR,
                                                   RADIUS,
                                                   BORDER_COLOR,
                                                   BORDER_OPA,
                                                   BORDER_WIDTH,
                                                   BORDER_SIDE,
                                                   SHADOW_COLOR,
                                                   SHADOW_WIDTH,
                                                   TEXT_OPA,
                                                   TEXT_COLOR,
                                                   TEXT_LETTER_SPACE,
                                                   TEXT_LINE_SPACE,
                                                   IMG_OPA,
                                                   IMG_RECOLOR,
                                                   LINE_OPA,
                                                   LINE_COLOR,
                                                   LINE_ROUNDED,
                                                   TEXT_FONT };
LV_STYLE_CONST_INIT(style_line, style_line_props); // style for the line that indicates the 30 minute mark in the time slots

const lv_style_const_prop_t style_appointment_props[] = { LV_STYLE_CONST_BG_COLOR(LV_COLOR_BLACK),
                                                          LV_STYLE_CONST_BG_GRAD_COLOR(LV_COLOR_BLACK),
                                                          LV_STYLE_CONST_RADIUS(10),
                                                          LV_STYLE_CONST_TEXT_COLOR(LV_COLOR_WHITE),
                                                          LV_STYLE_CONST_BORDER_SIDE(LV_BORDER_SIDE_NONE),
                                                          LV_STYLE_CONST_LINE_WIDTH(0),
                                                          LV_STYLE_CONST_BORDER_WIDTH(0),

                                                          PAD_TOP,
                                                          PAD_BOTTOM,
                                                          PAD_LEFT,
                                                          PAD_RIGHT,
                                                          OPA,
                                                          BG_GRAD_COLOR,
                                                          BORDER_COLOR,
                                                          BORDER_OPA,
                                                          BORDER_WIDTH,
                                                          BORDER_SIDE,
                                                          SHADOW_COLOR,
                                                          SHADOW_WIDTH,
                                                          TEXT_OPA,
                                                          TEXT_LETTER_SPACE,
                                                          TEXT_LINE_SPACE,
                                                          IMG_OPA,
                                                          IMG_RECOLOR,
                                                          LINE_OPA,
                                                          LINE_COLOR,
                                                          LINE_WIDTH,
                                                          LINE_ROUNDED,
                                                          TEXT_FONT };
LV_STYLE_CONST_INIT(style_appointment, style_appointment_props); // style for the appointment / calendar entries

const lv_style_const_prop_t style_header_props[] = { LV_STYLE_CONST_BORDER_WIDTH(0),
                                                     LV_STYLE_CONST_LINE_ROUNDED(1),
                                                     LV_STYLE_CONST_BG_GRAD_COLOR(LV_COLOR_BLACK),
                                                     LV_STYLE_CONST_TEXT_FONT(&lv_font_montserrat_26),
                                                     LV_STYLE_CONST_BORDER_SIDE(LV_BORDER_SIDE_NONE),

                                                     PAD_TOP,
                                                     PAD_BOTTOM,
                                                     PAD_LEFT,
                                                     PAD_RIGHT,
                                                     OPA,
                                                     BG_COLOR,
                                                     RADIUS,
                                                     BORDER_COLOR,
                                                     BORDER_OPA,
                                                     BORDER_SIDE,
                                                     SHADOW_COLOR,
                                                     SHADOW_WIDTH,
                                                     TEXT_OPA,
                                                     TEXT_COLOR,
                                                     TEXT_LETTER_SPACE,
                                                     TEXT_LINE_SPACE,
                                                     IMG_OPA,
                                                     IMG_RECOLOR,
                                                     LINE_OPA,
                                                     LINE_COLOR,
                                                     LINE_WIDTH,
                                                     LINE_ROUNDED };
LV_STYLE_CONST_INIT(style_header, style_header_props); // style for the header line above the calendar

// Wallpaper
static lv_obj_t * wallpaper;

// Main Window
static lv_obj_t * main_window;

static lv_obj_t * main_room_label;
static lv_obj_t * main_logo;

static char main_room_message[25] = { 0 };

// Status Window
static lv_obj_t * status_window;

static lv_obj_t * status_message_label;
static lv_obj_t * status_heartbeat_label;

static char status_message_message[24]   = { 0 };
static char status_heartbeat_message[31] = { 0 };

// Calendar Windows
static lv_obj_t * calendar_window;

static lv_obj_t * calendar_header;
static lv_obj_t * calendar_header_label;

static char calender_header_message[24] = { 0 };

// Time Slots
static lv_obj_t * calendar_time_slots[TIME_TABLE_LENGTH];
static lv_obj_t * calendar_time_slot_labels[TIME_TABLE_LENGTH];

static lv_point_t line_points[] = { { 0, 0 }, { 305, 0 } };

// Appointments
static lv_obj_t * calendar_appointment_slot[APPOINTMENT_SLOTS];
static lv_obj_t * calendar_appointment_label_lecture[APPOINTMENT_SLOTS];
static lv_obj_t * calendar_appointment_label_subtitle[APPOINTMENT_SLOTS];
static uint32_t hashes[APPOINTMENT_SLOTS];

// basic display info
static char current_room[20];
static char current_weekday[11];
static char current_date[11];
static char current_time[6];
static int current_nodeid;

// adds the styles to the according objects
static void set_styles(void)
{
    lv_obj_add_style(calendar_window, (lv_style_t *) &style_window, 0);
    lv_obj_add_style(main_window, (lv_style_t *) &style_window_room, 0);
    lv_obj_add_style(status_window, (lv_style_t *) &style_status_window, 0);
}

// creates the wallpaper -> background
static void create_wallpaper(void)
{
    LV_IMG_DECLARE(chess);
    wallpaper = lv_img_create(lv_scr_act());
    lv_img_set_src(wallpaper, &chess);
    lv_obj_set_size(wallpaper, 800, 480);
}

// creates the main window on the right
static void create_main_window()
{
    main_window = lv_obj_create(wallpaper);
    lv_obj_set_size(main_window, WINDOW_WIDTH, 360);
    lv_obj_align(main_window, LV_ALIGN_TOP_RIGHT, -20, 20);

    main_room_label = lv_label_create(main_window);
    lv_label_set_text(main_room_label, "Room: not initialized");
    lv_obj_align(main_room_label, LV_ALIGN_TOP_LEFT, 20, 20);

    LV_IMG_DECLARE(Hochschule_Muenchen_Logo);
    main_logo = lv_img_create(main_window);
    lv_img_set_src(main_logo, &Hochschule_Muenchen_Logo);
    lv_obj_align(main_logo, LV_ALIGN_BOTTOM_RIGHT, -7, -3);
}

// creates the status window on the right bottom
static void create_status_window(void)
{
    status_window = lv_obj_create(wallpaper);
    lv_obj_set_size(status_window, WINDOW_WIDTH, 60);
    lv_obj_align(status_window, LV_ALIGN_BOTTOM_RIGHT, -20, -20);

    status_message_label = lv_label_create(status_window);
    lv_label_set_text(status_message_label, "Node-Id:");
    lv_obj_align(status_message_label, LV_ALIGN_TOP_LEFT, 10, 7);

    status_heartbeat_label = lv_label_create(status_window);
    lv_label_set_text(status_heartbeat_label, "Last update:");
    lv_obj_align(status_heartbeat_label, LV_ALIGN_BOTTOM_LEFT, 10, -7);
}

// creates the calendar on the left
static void create_calendar(void)
{
    char buffer[20];

    calendar_window = lv_obj_create(wallpaper);
    lv_obj_set_size(calendar_window, WINDOW_WIDTH, WINDOW_HEIGHT);
    lv_obj_align(calendar_window, LV_ALIGN_TOP_LEFT, PADDING, PADDING);

    calendar_header = lv_obj_create(calendar_window);
    lv_obj_add_style(calendar_header, (lv_style_t *) &style_header, 0);

    lv_obj_set_size(calendar_header, WINDOW_WIDTH - 4, 34);
    lv_obj_align(calendar_header, LV_ALIGN_TOP_LEFT, 0, 0);

    calendar_header_label = lv_label_create(calendar_header);
    lv_label_set_text(calendar_header_label, "Date: not initialized");
    lv_obj_align(calendar_header_label, LV_ALIGN_CENTER, 0, 0);

    for (int time_slot = 0; time_slot < TIME_TABLE_LENGTH; time_slot++)
    {
        calendar_time_slots[time_slot] = lv_obj_create(calendar_window);
        lv_obj_set_size(calendar_time_slots[time_slot], WINDOW_WIDTH - 4, 41);
        lv_obj_add_style(calendar_time_slots[time_slot], (lv_style_t *) &style_time_slot, 0);
        lv_obj_align(calendar_time_slots[time_slot], LV_ALIGN_TOP_LEFT, 2, 2 + 35 + (40 * time_slot));

        calendar_time_slot_labels[time_slot] = lv_label_create(calendar_time_slots[time_slot]);
        lv_obj_align(calendar_time_slot_labels[time_slot], LV_ALIGN_TOP_LEFT, 5, 7);

        lv_obj_t * line = lv_line_create(calendar_time_slots[time_slot]);
        lv_line_set_points(line, line_points, 2);
        lv_obj_add_style(line, (lv_style_t *) &style_line, 0);
        lv_obj_align(line, LV_ALIGN_TOP_LEFT, 60, 20);

        sprintf(buffer, "%02d:00", 8 + time_slot);
        lv_label_set_text(calendar_time_slot_labels[time_slot], buffer);
    }
}

// calls the 4 creation functions above
static void create_windows(void)
{
    create_wallpaper();
    create_calendar();
    create_main_window();
    create_status_window();
}

// creates a h31 hash value
uint32_t h31_hash(const char * s, size_t len)
{
    uint32_t h = 0;
    while (len)
    {
        h = 31 * h + *s++;
        --len;
    }
    return h;
}

/** - returns the entry position, if the given hash is already existing
 *  - if hash not present, INT_MAX is returned
 */
int active_entries_contains(uint32_t newhash)
{
    for (int i = 0; i < APPOINTMENT_SLOTS; i++)
    {
        if (hashes[i] == newhash)
        {
            return i;
        }
    }
    return __INT_MAX__;
}

/** - creates an appointment / calendar entry and displays it
 *  - takes nullterminated char arrays for title and professor subtitle as arguments
 *    aswell as the start / end hour and start / end minutes as integer
 *    plus the slot, in which the entry should be written
 */
void display_create_appointment(char * title, char * subtitle, uint32_t starth, uint32_t endh, uint32_t startmin, uint32_t endmin,
                                int slot)
{
    // some basic tests, if entry is valid / applicable / possible to display time wise
    if (endh < starth)
        return;
    if (endh == starth && endmin <= startmin)
        return;
    if (starth < 8)
    {
        starth   = 8;
        startmin = 0;
    }
    if (endh >= 18)
    {
        endh   = 18;
        endmin = 0;
    }
    if (startmin > 59 || endmin > 59)
    {
        return;
    }
    if (starth > 18 || endh < 8)
    {
        return;
    }

    int halign = 60;                        // standard distance from the left side of the calendar window
    int width  = WINDOW_WIDTH - halign - 4; // width of the calendar entry

    // height of the entry is calculated according to the start and end time
    int height;
    if (endmin < startmin)
    {
        height = (int) (40 * (float) ((endh - starth - 1) + ((float) (endmin - startmin + 60) / 60)));
    }
    else
    {
        height = (int) (40 * (float) ((endh - starth) + ((float) (endmin - startmin) / 60)));
    }

    printk("Adding Apointment %s - width: %i, height: %i\n", title, width, height);

    // size and text of the entry is applied
    lv_obj_set_size(calendar_appointment_slot[slot], width, height);

    // entry is big enough to show subtitle
    if (height >= 60)
    {
        lv_obj_align(calendar_appointment_label_lecture[slot], LV_ALIGN_TOP_LEFT, 5, height / 5);
        lv_obj_align(calendar_appointment_label_subtitle[slot], LV_ALIGN_TOP_LEFT, 5, height / 5 + 20);

        lv_label_set_text(calendar_appointment_label_lecture[slot], title);
        lv_label_set_text(calendar_appointment_label_subtitle[slot], subtitle);
    }
    else // entry is small, so only lecture name is shown
    {
        lv_obj_align(calendar_appointment_label_lecture[slot], LV_ALIGN_CENTER, 0, 0);
        lv_obj_align(calendar_appointment_label_subtitle[slot], LV_ALIGN_CENTER, 0, 0);
        lv_label_set_text(calendar_appointment_label_lecture[slot], title);
        lv_label_set_text(calendar_appointment_label_subtitle[slot], "");
    }

    // screen gets updated
    lv_task_handler();
}

// appointment gets "deleted" by setting its size to zero
static void clear_appointment(int slot)
{
    while (lv_obj_get_width(calendar_appointment_slot[slot]) != 0) // make sure set size to zero worked
    {
        lv_obj_set_size(calendar_appointment_slot[slot], 0, 0);
        lv_task_handler(); // update values and screen
    }
    hashes[slot] = 0; // hash value is deleted
}

// node id is written to the status screen
void display_set_nodeid(uint32_t id)
{
    snprintf(status_message_message, 24, "Node-Id: %u", id);
    lv_label_set_text(status_message_label, status_message_message);
}

// time of last update is written to the status screen
void display_set_lastupdated(char * date, char * time)
{
    snprintf(status_heartbeat_message, 31, "Last update: %.10s, %.5s", date, time);
    lv_label_set_text(status_heartbeat_label, status_heartbeat_message);
}

// todays date is written to the top of the calendar
void display_set_date(char * day, char * date)
{
    snprintf(calender_header_message, 23, "%.9s, %.10s", day, date);
    lv_label_set_text(calendar_header_label, calender_header_message);
}

// room number is written to the room window on the right
void display_set_room(const char * message)
{
    snprintf(main_room_message, 25, "Room: %s", message);
    lv_label_set_text(main_room_label, main_room_message);
}

// display is initialized, called once in the beginning
void display_init()
{
    // the e-ink display is detected
    display_dev = device_get_binding(CONFIG_LVGL_DISPLAY_DEV_NAME);
    if (display_dev == NULL)
    {
        printk("gdew075t7 display not found or not ready.");
        return;
    }

    create_windows(); // windows and wallpaper are created
    display_blanking_off(display_dev);
    set_styles(); // styles are applied

    /** - all slots for the calendar entries are created
     *    with size zero, so they arent visible
     *  - one for every 15 miuntes (start time)
     */
    for (int i = 0; i < APPOINTMENT_SLOTS; i++)
    {
        // slot creation w size 0
        calendar_appointment_slot[i] = lv_obj_create(calendar_window);
        lv_obj_add_style(calendar_appointment_slot[i], (lv_style_t *) &style_appointment, 0);
        lv_obj_set_size(calendar_appointment_slot[i], 0, 0);
        // y-axis position / margin from the top of the calendar window (15 min delta)
        int valign = (int) (4 + 35 + 40 * (((int) (i / 4)) + (float) ((i % 4) * 0.25)));
        lv_obj_align(calendar_appointment_slot[i], LV_ALIGN_TOP_LEFT, 60, valign);
        // label creation
        calendar_appointment_label_lecture[i]  = lv_label_create(calendar_appointment_slot[i]);
        calendar_appointment_label_subtitle[i] = lv_label_create(calendar_appointment_slot[i]);
    }

    lv_task_handler(); // screen gets initialized

    printf("Display initiated\n");
}

// changed memcpy to prevent buffer overflows
void s_memcpy(char * dest, int destsize, char * src, int srcsize)
{
    if (destsize >= srcsize)
    {
        memcpy(dest, src, srcsize);
    }
    else
    {
        memcpy(dest, src, destsize);
    }
}

// get null terminated char array
char * create0termArray(const char * src, int size)
{
    char * ret = k_calloc(size, 1);
    memcpy(ret, src, size - 1);
    ret[size - 1] = '\0';
    return ret;
}

// get integer from char array
uint32_t createIntfromMem(const char * src, int size)
{
    char temp[size + 1];
    temp[size] = '\0';
    memcpy(temp, src, size);
    uint32_t ret = strtoul(temp, NULL, 10);
    return ret;
}

// update the whole screen with nullterminated char array as argument
void update(char * info)
{
    // extracting all separator symbols indexes
    char * word = info;
    int separators[MAX_AMOUNT_DELIMITERS];
    separators[0] = 0;
    int index     = 1;

    while (separators[index - 1] != strlen(info) - 1)
    {
        char * place      = strchr(word + 1, '`');
        separators[index] = place - info;
        word              = info + separators[index];
        index++;
    }

    int entryquantity = index - 1;

    // extracting the mandatory information to char array \0
    uint32_t nodeid = createIntfromMem(info + 1, separators[1] - 1);

    char * roomnumber = create0termArray(info + separators[1] + 1, separators[2] - separators[1]);
    char * weekday    = create0termArray(info + separators[2] + 1, separators[3] - separators[2]);
    char * date       = create0termArray(info + separators[3] + 1, separators[4] - separators[3]);
    char * time       = create0termArray(info + separators[4] + 1, separators[5] - separators[4]);

    printf("Nodeid: %u, Roomnumber: %s, Wochentag: %s, Datum: %s, Zeit: %s\n", nodeid, roomnumber, weekday, date, time);

    // updating id if changed
    if (current_nodeid != nodeid)
    {
        printf("node id has changed to: %u\n", nodeid);
        display_set_nodeid(nodeid);
        current_nodeid = nodeid;

        lv_task_handler();
    }

    // updating room number if changed
    if (strcmp(current_room, roomnumber) != 0)
    {
        printf("room number has changed to: %s\n", roomnumber);
        display_set_room(roomnumber);
        memset(current_room, 0, 20);
        s_memcpy(current_room, 20, roomnumber, separators[2] - separators[1]);

        lv_task_handler();
    }

    // updating day and date if changed
    if (strcmp(weekday, current_weekday) != 0 || strcmp(date, current_date) != 0)
    {
        printf("weekday and/or date has changed to: %.9s, %s\n", weekday, date);
        display_set_date(weekday, date);
        memset(current_weekday, 0, 10);
        s_memcpy(current_weekday, 10, weekday, separators[3] - separators[2]);

        lv_task_handler();
    }

    // updating date and time if changed
    if (strcmp(date, current_date) != 0 || strcmp(time, current_time) != 0)
    {
        printf("time and/or date has changed to: %s, %s\n", time, date);
        display_set_lastupdated(date, time);
        memset(current_date, 0, 11);
        s_memcpy(current_date, 11, date, separators[4] - separators[3]);
        memset(current_time, 0, 6);
        s_memcpy(current_time, 6, time, separators[5] - separators[4]);

        lv_task_handler();
    }

    // free allocated memory
    k_free(roomnumber);
    k_free(weekday);
    k_free(date);
    k_free(time);

    // initializing array for still existing entries
    bool validated[APPOINTMENT_SLOTS] = { false };

    // extracting the appointment / occupancy entries if not already present and update the screen
    for (index = 5; index < entryquantity; index = index + 6)
    {
        // create hash from the whole entry
        char wholeentry[separators[index + 6] - separators[index] - 1];
        memcpy(wholeentry, info + separators[index] + 1, separators[index + 6] - separators[index] - 1);
        uint32_t newhash = h31_hash(wholeentry, separators[index + 6] - separators[index] - 1);

        int isnew = active_entries_contains(newhash); // check if hash is already existing
        if (isnew == __INT_MAX__)                     // if entry is new
        {

            // extracting information from entry
            char * title    = create0termArray(info + separators[index] + 1, separators[index + 1] - separators[index]);
            char * subtitle = create0termArray(info + separators[index + 1] + 1, separators[index + 2] - separators[index + 1]);
            uint32_t starth = createIntfromMem(info + separators[index + 2] + 1, separators[index + 3] - separators[index + 2] - 1);
            uint32_t endh   = createIntfromMem(info + separators[index + 4] + 1, separators[index + 5] - separators[index + 4] - 1);
            uint32_t startmin =
                createIntfromMem(info + separators[index + 3] + 1, separators[index + 4] - separators[index + 3] - 1);
            uint32_t endmin = createIntfromMem(info + separators[index + 5] + 1, separators[index + 6] - separators[index + 5] - 1);

            // calculate in which slot (dep. on start time) the entry should be written
            int slot = (starth - 8) * 4 + (int) (startmin / 15);
            clear_appointment(slot);   // delete current appointment
            hashes[slot]    = newhash; // save hash
            validated[slot] = true;    // validate entry because it is new
            display_create_appointment(title, subtitle, starth, endh, startmin, endmin, slot);

            // free allocated memory
            k_free(title);
            k_free(subtitle);
        }
        else
        {
            // validate existing entry because sent message contains it again
            validated[isnew] = true;
        }
    }

    // delete all entries, that arent valid / contained in the current message
    for (int i = 0; i < APPOINTMENT_SLOTS; i++)
    {
        if (!validated[i])
        {
            clear_appointment(i);
        }
    }
}

/** - function to initialize and update the screen
 *  - params: string -> message and signal -> semaphore */
void display_update(void * string, void * signal)
{
    display_init(); // display gets initialized -> base picture showing w.o. information

    while (1) // run forever
    {
        k_sem_take(signal, K_FOREVER);
        // when sem is given by the ChannelManager, update screen with current message
        update(string);
    }
}