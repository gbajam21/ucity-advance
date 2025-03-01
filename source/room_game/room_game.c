// SPDX-License-Identifier: GPL-3.0-only
//
// Copyright (c) 2021 Antonio Niño Díaz

#include <string.h>

#include <ugba/ugba.h>

#include "audio.h"
#include "cursor.h"
#include "date.h"
#include "input_utils.h"
#include "jukebox.h"
#include "main.h"
#include "money.h"
#include "map_utils.h"
#include "random.h"
#include "save.h"
#include "sfx.h"
#include "text_utils.h"
#include "room_bank/room_bank.h"
#include "room_game/building_info.h"
#include "room_game/build_menu.h"
#include "room_game/draw_building.h"
#include "room_game/draw_common.h"
#include "room_game/notification_box.h"
#include "room_game/pause_menu.h"
#include "room_game/room_game.h"
#include "room_game/text_messages.h"
#include "room_game/tileset_info.h"
#include "room_game/status_bar.h"
#include "room_graphs/graphs_handler.h"
#include "room_save_slots/room_save_slots.h"
#include "simulation/budget.h"
#include "simulation/building_count.h"
#include "simulation/calculate_stats.h"
#include "simulation/common.h"
#include "simulation/fire.h"
#include "simulation/technology.h"
#include "simulation/traffic.h"
#include "simulation/transport_anims.h"
#include "simulation/water.h"

// Assets

#include "maps/city/city_map_palette_bin.h"
#include "maps/city/city_map_palette_gbc_bin.h"
#include "maps/city/city_map_tiles_bin.h"
#include "maps/city/city_map_tiles_gbc_bin.h"
#include "sprites/busy_icon_gbc_palette_bin.h"
#include "sprites/busy_icon_gbc_tiles_bin.h"
#include "sprites/busy_icon_palette_bin.h"
#include "sprites/busy_icon_tiles_bin.h"

// ----------------------------------------------------------------------------

static int graphics_are_new = 1;
static int simulation_enabled = 1;
static int animations_enabled = 1;

// Frames left for a simulation step
static int frames_left_to_step = 0;
#define MIN_FRAMES_PER_DATE_STEP    60

// This is set to 1 when in disaster mode
static int simulation_disaster_mode = 0;

// This is set to 1 when the main loop isn't done with a simulation step
static volatile int main_loop_is_busy;

static char city_name[CITY_MAX_NAME_LENGTH + 1];

// ----------------------------------------------------------------------------

// Must be a power of 2
#define ANIMATION_TRANSPORT_COUNT_FRAMES    4

// Must be a multiple of ANIMATION_TRANSPORT_COUNT_FRAMES
#define ANIMATION_COUNT_FRAMES_NORMAL       60

// Doesn't need to be a multiple of ANIMATION_TRANSPORT_COUNT_FRAMES
#define ANIMATION_COUNT_FRAMES_DISASTER     15

int animation_has_to_update_transport;
int animation_has_to_update_map;
int animation_countdown; // This goes from 0 to ANIMATION_COUNT_FRAMES_xxxxx

int Room_Game_IsInDisasterMode(void)
{
    return simulation_disaster_mode;
}

void Room_Game_SetDisasterMode(int enabled)
{
    simulation_disaster_mode = enabled;
}

void Room_Game_SetAnimationsEnabled(int value)
{
    animations_enabled = value;
}

int Room_Game_AreAnimationsEnabled(void)
{
    if (animations_enabled)
        return 1;
    return 0;
}

static void GameAnimateMapVBLFastHandle(void)
{
    if (animations_enabled == 0)
        return;

    if (simulation_disaster_mode)
    {
        // Disaster mode

        if (animation_countdown < ANIMATION_COUNT_FRAMES_DISASTER)
        {
            animation_countdown++;
            return;
        }

        animation_countdown = 0;

        animation_has_to_update_map = 1;

        return;
    }
    else
    {
        // Normal mode

        // Update every ANIMATION_TRANSPORT_COUNT_FRAMES frames
        if ((animation_countdown & (ANIMATION_TRANSPORT_COUNT_FRAMES - 1))
                == (ANIMATION_TRANSPORT_COUNT_FRAMES -1))
        {
            // This function does a fast update of the position of all sprites,
            // it doesn't handle creation or destruction of objects.
            Simulation_TransportAnimsVBLHandle();

            animation_has_to_update_transport = 1;
        }

        if (animation_countdown < ANIMATION_COUNT_FRAMES_NORMAL)
        {
            animation_countdown++;
            return;
        }

        animation_countdown = 0;

        animation_has_to_update_map = 1;

        return;
    }
}

static void GameAnimateMap(void)
{
    if (animations_enabled == 0)
        return;

    if (simulation_disaster_mode)
    {
        // Disaster mode

        if (animation_has_to_update_map)
        {
            Simulation_FireAnimate();
            Simulation_WaterAnimate();

            animation_has_to_update_map = 0;
        }

        return;
    }
    else
    {
        // Normal mode

        if (animation_has_to_update_transport)
        {
            Simulation_TransportAnimsHandle();
            animation_has_to_update_transport = 0;
        }

        if (animation_has_to_update_map)
        {
            Simulation_TrafficAnimate();
            Simulation_WaterAnimate();

            animation_has_to_update_map = 0;
        }

        return;
    }
}

// ----------------------------------------------------------------------------

typedef enum {
    MODE_RUNNING,
    MODE_WATCH,
    MODE_SHOW_NOTIFICATION,
    MODE_SELECT_BUILDING,
    MODE_MODIFY_MAP,
    MODE_PAUSE_MENU,
} room_game_mode;

static room_game_mode current_mode;

static int mapx = 0;
static int mapy = 0;
static int scrollx = 0;
static int scrolly = 0;

static int last_build_x = -1;
static int last_build_y = -1;

void Room_Game_GetCurrentScroll(int *x, int *y)
{
    *x = mapx;
    *y = mapy;
}

int Room_Game_IsSimulationEnabled(void)
{
    return simulation_enabled;
}

static void Load_City_Data(const void *map, int scx, int scy)
{
    if (map)
    {
        // Load the map
        copy_map_to_sbb(map, (void *)CITY_MAP_BASE,
                        CITY_MAP_HEIGHT, CITY_MAP_WIDTH);
    }

    mapx = scx * 8;
    mapy = scy * 8;

    BG_RegularScrollSet(2, mapx, mapy);
}

void Room_Game_Request_Scroll(int scx, int scy)
{
    mapx = scx * 8;
    mapy = scy * 8;

    BG_RegularScrollSet(2, mapx, mapy);
}

void Room_Game_Graphics_New_Set(int new_graphics)
{
    graphics_are_new = new_graphics;
}

int Room_Game_Graphics_New_Get(void)
{
    return graphics_are_new;
}

void Room_Game_Load_City_Graphics(void)
{
    if (Room_Game_Graphics_New_Get())
    {
        // Load the palettes
        VRAM_BGPalette16Copy(city_map_palette_bin, city_map_palette_bin_size,
                             CITY_MAP_PALETTE);

        // Load the tiles
        SWI_CpuSet_Copy16(city_map_tiles_bin, (void *)CITY_TILES_BASE,
                          city_map_tiles_bin_size);
    }
    else
    {
        // Load the palettes
        VRAM_BGPalette16Copy(city_map_palette_gbc_bin,
                             city_map_palette_gbc_bin_size,
                             CITY_MAP_PALETTE);

        // Load the tiles
        SWI_CpuSet_Copy16(city_map_tiles_gbc_bin, (void *)CITY_TILES_BASE,
                          city_map_tiles_gbc_bin_size);
    }

    // Setup background
    BG_RegularInit(2, BG_REGULAR_512x512, BG_256_COLORS,
                   CITY_TILES_BASE, CITY_MAP_BASE);
}

static void Room_Game_Load_Cursor_Graphics(void)
{
#define CURSOR_PALETTE      (15)
#define CURSOR_TILES_BASE   MEM_BG_TILES_BLOCK_ADDR(5)
#define CURSOR_TILES_INDEX  (512)

    Load_Cursor_Graphics((void *)CURSOR_TILES_BASE, CURSOR_TILES_INDEX);

    Cursor_Set_Position(48, 48);

    Load_Cursor_Palette(CURSOR_PALETTE);
}

static void Room_Game_Load_Busy_Icon_Graphics(void)
{
#define BUSY_ICON_PALETTE      (11)
#define BUSY_ICON_TILES_BASE   (MEM_BG_TILES_BLOCK_ADDR(5) + (8 * 8 * 4 / 8))
#define BUSY_ICON_TILES_INDEX  (513)

    if (Room_Game_Graphics_New_Get())
    {
        SWI_CpuSet_Copy16(busy_icon_tiles_bin, (void *)BUSY_ICON_TILES_BASE,
                          busy_icon_tiles_bin_size);

        VRAM_OBJPalette16Copy(busy_icon_palette_bin, busy_icon_palette_bin_size,
                              BUSY_ICON_PALETTE);
    }
    else
    {
        SWI_CpuSet_Copy16(busy_icon_gbc_tiles_bin, (void *)BUSY_ICON_TILES_BASE,
                          busy_icon_gbc_tiles_bin_size);

        VRAM_OBJPalette16Copy(busy_icon_gbc_palette_bin,
                              busy_icon_gbc_palette_bin_size,
                              BUSY_ICON_PALETTE);
    }

    OBJ_RegularInit(116, GBA_SCREEN_W - 8, 0,
                    OBJ_SIZE_8x8, OBJ_16_COLORS,
                    BUSY_ICON_PALETTE, BUSY_ICON_TILES_INDEX);
    OBJ_PrioritySet(116, 1);
    OBJ_RegularEnableSet(116, 0);
}

static void Room_Game_Load_Busy_Icon_Set_Position(int status_bar_is_up)
{
    if (status_bar_is_up)
        OBJ_PositionSet(116, GBA_SCREEN_W - 8, 16);
    else
        OBJ_PositionSet(116, GBA_SCREEN_W - 8, 0);
}

static void Room_Game_Load_Busy_Icon_Show(int show)
{
    OBJ_RegularEnableSet(116, show);
}

static void Room_Game_Draw_RCI_Bars(void)
{
    int r, c, i;
    Simulation_GetDemandRCI(&r, &c, &i);

    // TODO: Remove magic numbers

    char RCI[4] = { 0 };

    RCI[0] = 144 + r;
    RCI[1] = 160 + c;
    RCI[2] = 176 + i;
    StatusBarPrint(27, 0, RCI);

    RCI[0] = 128;
    RCI[1] = 129;
    RCI[2] = 130;
    StatusBarPrint(27, 1, RCI);
}

static void Room_Game_Handle_Scroll(void)
{
    const int maxscrollx = (CITY_MAP_WIDTH * 8) - GBA_SCREEN_W;
    const int maxscrolly = (CITY_MAP_HEIGHT * 8) - GBA_SCREEN_H;

    const int cur_win_x_min = 8 * 4;
    const int cur_win_x_max = GBA_SCREEN_W - 8 * 4;
    const int cur_win_y_min = 8 * 4;
    const int cur_win_y_max = GBA_SCREEN_H - 8 * 4;

    int curx, cury, curw, curh;
    Cursor_Get_Position(&curx, &cury);
    Cursor_Get_Size(&curw, &curh);

    if (scrolly == 0)
    {
        if (Key_Autorepeat_Pressed_Up())
        {
            if (cury > cur_win_y_min)
            {
                Cursor_Set_Position(curx, cury - 8);
            }
            else
            {
                if (mapy > 0)
                {
                    scrolly = -2;
                }
                else
                {
                    if (cury > 0)
                        Cursor_Set_Position(curx, cury - 8);
                }
            }
        }
        else if (Key_Autorepeat_Pressed_Down())
        {
            if ((cury + curh) < cur_win_y_max)
            {
                Cursor_Set_Position(curx, cury + 8);
            }
            else
            {
                if (mapy < maxscrolly)
                {
                    scrolly = 2;
                }
                else
                {
                    if ((cury + curh) < GBA_SCREEN_H)
                        Cursor_Set_Position(curx, cury + 8);
                }
            }
        }
    }

    if (scrollx == 0)
    {
        if (Key_Autorepeat_Pressed_Left())
        {
            if (curx > cur_win_x_min)
            {
                Cursor_Set_Position(curx - 8, cury);
            }
            else
            {
                if (mapx > 0)
                {
                    scrollx = -2;
                }
                else
                {
                    if (curx > 0)
                        Cursor_Set_Position(curx - 8, cury);
                }
            }
        }
        else if (Key_Autorepeat_Pressed_Right())
        {
            if ((curx + curw) < cur_win_x_max)
            {
                Cursor_Set_Position(curx + 8, cury);
            }
            else
            {
                if (mapx < maxscrollx)
                {
                    scrollx = 2;
                }
                else
                {
                    if ((curx + curw) < GBA_SCREEN_W)
                        Cursor_Set_Position(curx + 8, cury);
                }
            }
        }
    }

    // Refresh cursor

    Cursor_Update();
    Cursor_Refresh();

    // Update position of status bar

    if (cury <= cur_win_y_min)
    {
        Room_Game_Load_Busy_Icon_Set_Position(0);
        StatusBarPositionSet(STATUS_BAR_DOWN);
        if (current_mode == MODE_MODIFY_MAP)
        {
            int building = BuildMenuSelection();
            BuildIconPlace(building, GBA_SCREEN_W - 16, GBA_SCREEN_H - 16);
        }
    }
    else if ((cury + curh) >= cur_win_y_max)
    {
        Room_Game_Load_Busy_Icon_Set_Position(1);
        StatusBarPositionSet(STATUS_BAR_UP);
        if (current_mode == MODE_MODIFY_MAP)
        {
            int building = BuildMenuSelection();
            BuildIconPlace(building, GBA_SCREEN_W - 16, 0);
        }
    }
}

static void Room_Game_Handle_Scroll_Fast(void)
{
    const int maxscrollx = (CITY_MAP_WIDTH * 8) - GBA_SCREEN_W;
    const int maxscrolly = (CITY_MAP_HEIGHT * 8) - GBA_SCREEN_H;

    uint16_t keys = KEYS_Held();

    if (scrolly == 0)
    {
        if (keys & KEY_UP)
        {
            if (mapy > 0)
                scrolly = -2;
        }
        else if (keys & KEY_DOWN)
        {
            if (mapy < maxscrolly)
                scrolly = 2;
        }
    }

    if (scrollx == 0)
    {
        if (keys & KEY_LEFT)
        {
            if (mapx > 0)
                scrollx = -2;
        }
        else if (keys & KEY_RIGHT)
        {
            if (mapx < maxscrollx)
                scrollx = 2;
        }
    }
}

static void Room_Game_Handle_Drift(void)
{
    const int maxscrollx = (CITY_MAP_WIDTH * 8) - GBA_SCREEN_W;
    const int maxscrolly = (CITY_MAP_HEIGHT * 8) - GBA_SCREEN_H;

    mapx += scrollx;
    mapy += scrolly;

    if (mapx < 0)
        mapx = 0;
    if (mapx > maxscrollx)
        mapx = maxscrollx;
    if (mapy < 0)
        mapy = 0;
    if (mapy > maxscrolly)
        mapy = maxscrolly;

    BG_RegularScrollSet(2, mapx, mapy);

    // End map scroll if reached a grid spot

    if ((mapx % 8) == 0)
        scrollx = 0;
    if ((mapy % 8) == 0)
        scrolly = 0;
}

void BuildModeUpdateStatusBar(void)
{
    int building = BuildMenuSelection();
    const building_info *bi = Get_Building_Info(building);

    char str[31];
    str[0] = 0;

    strcpy(str, "Cost:      ");
    Print_Integer_Decimal_Right(&str[strlen(str)], 11, bi->price);
    StatusBarPrint(0, 0, str);

    strcpy(str, "Funds:     ");
    Print_Integer_Decimal_Right(&str[strlen(str)], 11, MoneyGet());
    StatusBarPrint(0, 1, str);

    Room_Game_Draw_RCI_Bars();
}

void ModifyModeUpdateStatusBar(void)
{
    int building = BuildMenuSelection();
    const building_info *bi = Get_Building_Info(building);

    char str[31];
    str[0] = 0;

    strcpy(str, "Cost:      ");
    Print_Integer_Decimal_Right(&str[strlen(str)], 11, bi->price);
    StatusBarPrint(0, 0, str);

    strcpy(str, "Funds:     ");
    Print_Integer_Decimal_Right(&str[strlen(str)], 11, MoneyGet());
    StatusBarPrint(0, 1, str);

    int y = (StatusBarPositionGet() == STATUS_BAR_UP) ? 0 : (GBA_SCREEN_H - 16);
    BuildIconPlace(building, GBA_SCREEN_W - 16, y);
}

void Room_Game_Set_Mode(int mode)
{
    current_mode = mode;

    switch (mode)
    {
        case MODE_RUNNING:
        {
            Room_Game_Load_Busy_Icon_Show(0);
            BuildSelectMenuHide();
            BuildIconPlace(0, 240, 160);
            Cursor_Set_Size(8, 8);
            Cursor_Refresh();
            StatusBarClear();
            StatusBarShow();
            if (simulation_disaster_mode == 0)
            {
                Simulation_TransportAnimsShow();
            }
            break;
        }
        case MODE_WATCH:
        {
            Room_Game_Load_Busy_Icon_Show(0);
            Cursor_Hide();
            StatusBarHide();
            break;
        }
        case MODE_SHOW_NOTIFICATION:
        {
            Room_Game_Load_Busy_Icon_Show(0);
            StatusBarHide();
            Notification_Box_Show();
            break;
        }
        case MODE_SELECT_BUILDING:
        {
            Cursor_Hide();
            Simulation_TransportAnimsHide();
            StatusBarClear();
            BuildModeUpdateStatusBar();
            BuildMenuReset();
            BuildSelectMenuShow();
            StatusBarShow();
            break;
        }
        case MODE_MODIFY_MAP:
        {
            BuildSelectMenuHide();

            const building_info *info = Get_Building_Info(BuildMenuSelection());
            Cursor_Set_Size(info->width * 8, info->height * 8);
            Cursor_Refresh();
            StatusBarClear();
            ModifyModeUpdateStatusBar();
            StatusBarShow();

            last_build_x = -1;
            last_build_y = -1;
            break;
        }
        case MODE_PAUSE_MENU:
        {
            Cursor_Hide();
            PauseMenuLoad();
            PauseMenuDraw();
            break;
        }
        default:
        {
            UGBA_Assert(0);
            break;
        }
    }
}

void ViewModeUpdateStatusBar(void)
{
    char str[31];
    str[0] = 0;

    strcpy(str, "Funds:     ");
    Print_Integer_Decimal_Right(&str[strlen(str)], 11, MoneyGet());
    StatusBarPrint(0, 0, str);

    strcpy(str, "Date:  ");
    strcat(str, DateString());
    StatusBarPrint(0, 1, str);

    Room_Game_Draw_RCI_Bars();
}

// ----------------------------------------------------------------------------

void Room_Game_Load(void)
{
    Game_Clear_Screen();

    // Load tiles
    // ----------

    BuildSelectMenuLoadGfx();
    StatusBarLoad();
    StatusBarShow();
    Room_Game_Load_Cursor_Graphics();
    Room_Game_Load_Busy_Icon_Graphics();

    // Load notification box
    // ---------------------

    Notification_Box_Load();

    // Load city background
    // ---------------

    Room_Game_Load_City_Graphics();

    BG_RegularScrollSet(2, mapx, mapy);

    /// Setup display
    //---------------

    DISP_ModeSet(0);
    DISP_Object1DMappingEnable(1);
    DISP_LayersEnable(1, 1, 1, 0, 1);

    // Prepare cursor
    // --------------

    Cursor_Set_Size(8, 8);
    Cursor_Reset_Position();
    Cursor_Refresh();

    // Load transportation sprites
    // -------------------------

    Simulation_TransportLoadGraphics();

    // Initialize room state
    // ---------------------

    // Refresh some simulation data
    Simulation_CountBuildings();

    frames_left_to_step = 0;
    main_loop_is_busy = 1;

    Simulation_RequestDisaster(REQUESTED_DISASTER_NONE);

    Room_Game_Set_Mode(MODE_RUNNING);
}

void Room_Game_Set_Initial_Load_State(void)
{
    // Whenever a new city is loaded, enable simulation. Don't re-enable it when
    // returning to this room from a menu, or from checking minimaps.
    simulation_enabled = 1;

    // Clear messages that may be in the queue from the previously loaded city
    MessageQueueInit();

    Simulation_TransportAnimsInit();
}

void Room_Game_Unload(void)
{
    Cursor_Hide();

    Game_Clear_Screen();
}

void Room_Game_Handle(void)
{
    switch (current_mode)
    {
        case MODE_RUNNING:
        {
            // If there are pending messages, don't start simulation step
            if (MessageQueueIsEmpty() == 0)
            {
                Notification_Box_Clear();
                Notification_Box_Set_Text(MessageQueueGet());
                Room_Game_Set_Mode(MODE_SHOW_NOTIFICATION);
                break;
            }

            if (simulation_enabled)
            {
                if (frames_left_to_step == 0)
                {
                    frames_left_to_step = MIN_FRAMES_PER_DATE_STEP;

                    main_loop_is_busy = 1;

                    if (Simulation_NegativeBudgetCountGet() >=
                        NEGATIVE_BUDGET_COUNT_GAME_OVER)
                    {
                        Game_Room_Prepare_Switch(ROOM_MAIN_MENU);
                    }

                    Simulation_SimulateAll();
                }
            }

            main_loop_is_busy = 0;

            break;
        }
        case MODE_WATCH:
        {
            if (simulation_enabled)
            {
                if (frames_left_to_step == 0)
                {
                    frames_left_to_step = MIN_FRAMES_PER_DATE_STEP;

                    main_loop_is_busy = 1;
                    Simulation_SimulateAll();
                }
            }

            main_loop_is_busy = 0;

            break;
        }
        case MODE_SHOW_NOTIFICATION:
        {
            break;
        }
        case MODE_SELECT_BUILDING:
        {
            break;
        }
        case MODE_MODIFY_MAP:
        {
            break;
        }
        case MODE_PAUSE_MENU:
        {
            break;
        }
        default:
        {
            UGBA_Assert(0);
            break;
        }
    }
}

// ----------------------------------------------------------------------------

void Room_Game_FastVBLHandler(void)
{
    GameAnimateMapVBLFastHandle();

    switch (current_mode)
    {
        case MODE_RUNNING:
        {
            Room_Game_Handle_Scroll();

            if (frames_left_to_step > 0)
                frames_left_to_step--;

            break;
        }
        case MODE_WATCH:
        {
            Room_Game_Handle_Scroll_Fast();

            if (frames_left_to_step > 0)
                frames_left_to_step--;

            break;
        }
        case MODE_SHOW_NOTIFICATION:
        {
            break;
        }
        case MODE_SELECT_BUILDING:
        {
            break;
        }
        case MODE_MODIFY_MAP:
        {
            Room_Game_Handle_Scroll();
            break;
        }
        case MODE_PAUSE_MENU:
        {
            if (frames_left_to_step > 0)
                frames_left_to_step--;

            break;
        }
        default:
        {
            UGBA_Assert(0);
            break;
        }
    }

    Room_Game_Handle_Drift();

    // If the map scrolls this frame it is needed to update the position of the
    // sprites for the next frame. At this point we know the scroll of the
    // background for the next frame, so this function can calculate the
    // position of the sprites for the next frame. They both will be updated in
    // the next VBL handler execution.
    //
    // This function also handles the non-critical parts of the animation
    // handling.
    Simulation_TransportAnimsScroll();
}

void Room_Game_SlowVBLHandler(void)
{
    uint16_t keys_pressed = KEYS_Pressed();
    uint16_t keys_released = KEYS_Released();
    uint16_t keys = KEYS_Held();

    switch (current_mode)
    {
        case MODE_RUNNING:
        {
            ViewModeUpdateStatusBar();

            if (keys_pressed & (KEY_SELECT | KEY_L))
                Room_Game_Set_Mode(MODE_SELECT_BUILDING);
            else if (keys_pressed & KEY_B)
                Room_Game_Set_Mode(MODE_WATCH);

            if (keys_released & (KEY_START | KEY_R))
            {
                Room_Game_Set_Mode(MODE_PAUSE_MENU);
                return;
            }

            if (keys_pressed & KEY_A)
            {
                int curx, cury;
                Cursor_Get_Position(&curx, &cury);

                int x = (mapx + curx) / 8;
                int y = (mapy + cury) / 8;

                uint16_t tile = CityMapGetTile(x, y);
                const city_tile_info *info = City_Tileset_Entry_Info(tile);

                 CustomMessageStringSet(info->name);
                 MessageQueueAdd(ID_MSG_CUSTOM);
            }

            if (MessageQueueIsEmpty() == 0)
            {
                Notification_Box_Clear();
                Notification_Box_Set_Text(MessageQueueGet());
                Room_Game_Set_Mode(MODE_SHOW_NOTIFICATION);
                break;
            }

            GameAnimateMap();

            break;
        }
        case MODE_WATCH:
        {
            if (MessageQueueIsEmpty() == 0)
            {
                Notification_Box_Clear();
                Notification_Box_Set_Text(MessageQueueGet());
                Room_Game_Set_Mode(MODE_SHOW_NOTIFICATION);
                break;
            }

            if (keys_released & KEY_B)
                Room_Game_Set_Mode(MODE_RUNNING);

            GameAnimateMap();

            break;
        }
        case MODE_SHOW_NOTIFICATION:
        {
            Notification_Box_Update();

            if (keys_pressed & KEY_A)
            {
                if (Notification_Box_Message_Is_Completed())
                {
                    if (MessageQueueIsEmpty() == 0)
                    {
                        Notification_Box_Clear();
                        Notification_Box_Set_Text(MessageQueueGet());
                    }
                    else
                    {
                        Notification_Box_Hide();
                        Room_Game_Set_Mode(MODE_RUNNING);
                    }
                }
            }

            break;
        }
        case MODE_SELECT_BUILDING:
        {
            Room_Game_Load_Busy_Icon_Show(main_loop_is_busy != 0);

            BuildMenuHandleInput();

            if (keys_released & KEY_B)
            {
                Room_Game_Set_Mode(MODE_RUNNING);
            }
            else if (keys_released & KEY_A)
            {
                int building = BuildMenuSelection();

                if (CityStats_IsBuildingAvailable(building) &&
                    Technology_IsBuildingAvailable(building))
                {
                    Room_Game_Set_Mode(MODE_MODIFY_MAP);
                }
                else
                {
                    SFX_WrongSelection();
                }
            }

            break;
        }
        case MODE_MODIFY_MAP:
        {
            Room_Game_Load_Busy_Icon_Show(main_loop_is_busy != 0);

            if (keys_released & KEY_B)
            {
                Room_Game_Set_Mode(MODE_RUNNING);
                Simulation_CountBuildings();
            }
            else if (keys_pressed & (KEY_SELECT | KEY_L))
            {
                Room_Game_Set_Mode(MODE_SELECT_BUILDING);
            }

            int curx, cury;
            Cursor_Get_Position(&curx, &cury);

            int x = (mapx + curx) / 8;
            int y = (mapy + cury) / 8;

            if (main_loop_is_busy == 0)
            {
                if (keys & KEY_A)
                {
                    if ((last_build_x != x) || (last_build_y != y))
                        Building_Build(0, BuildMenuSelection(), x, y);

                    last_build_x = x;
                    last_build_y = y;

                    ModifyModeUpdateStatusBar();
                }
            }

            if ((keys & KEY_A) == 0)
            {
                // Let player build again on the same place after releasing the
                // button. This is useful to demolish a building and then clear
                // the tile to turn it to field, or to remove RCI tiles after
                // demolishing RCI buildings.
                last_build_x = -1;
                last_build_y = -1;
            }

            break;
        }
        case MODE_PAUSE_MENU:
        {
            pause_menu_options option = PauseMenuHandleInput();
            switch (option)
            {
                case PAUSE_MENU_BUDGET:
                    Game_Room_Prepare_Switch(ROOM_BUDGET);
                    break;
                case PAUSE_MENU_BANK:
                    Game_Room_Prepare_Switch(ROOM_BANK);
                    return;
                case PAUSE_MENU_MINIMAPS:
                    Game_Room_Prepare_Switch(ROOM_MINIMAP);
                    return;
                case PAUSE_MENU_GRAPHS:
                    Game_Room_Prepare_Switch(ROOM_GRAPHS);
                    return;
                case PAUSE_MENU_CITY_STATS:
                    Game_Room_Prepare_Switch(ROOM_CITY_STATS);
                    return;
                // The following two are handled in PauseMenuHandleInput()
                //case PAUSE_MENU_DISASTERS:
                //case PAUSE_MENU_OPTIONS:
                case PAUSE_MENU_PAUSE:
                    simulation_enabled ^= 1;
                    PauseMenuDraw();
                    break;
                case PAUSE_MENU_SAVE_GAME:
                    // If a disaster is active, don't let the player save
                    if (simulation_disaster_mode == 0)
                    {
                        Room_Save_Slots_Set_Mode(ROOM_SAVE_SLOTS_SAVE);
                        Game_Room_Prepare_Switch(ROOM_SAVE_SLOTS);
                    }
                    else
                    {
                        SFX_WrongSelection();
                    }
                    return;
                case PAUSE_MENU_MAIN_MENU:
                    Room_Game_Settings_Save();
                    Game_Room_Prepare_Switch(ROOM_MAIN_MENU);
                    return;

                case PAUSE_MENU_EXIT:
                    Room_Game_Settings_Save();
                    Room_Game_Set_Mode(MODE_RUNNING);
                    break;

                case DISASTERS_ENABLE:
                {
                    int new_value = Simulation_AreDisastersEnabled() ^ 1;
                    Simulation_DisastersSetEnabled(new_value);
                    PauseMenuDraw();
                    break;
                }
                case DISASTERS_START_FIRE:
                    Simulation_RequestDisaster(REQUESTED_DISASTER_FIRE);
                    break;
                case DISASTERS_MELTDOWN:
                    Simulation_RequestDisaster(REQUESTED_DISASTER_MELTDOWN);
                    break;

                case OPTIONS_ANIMATIONS_ENABLE:
                    if (animations_enabled)
                        Simulation_TransportAnimsHide();
                    else
                        Simulation_TransportAnimsInit();

                    animations_enabled ^= 1;
                    PauseMenuDraw();
                    break;
                case OPTIONS_MUSIC_ENABLE:
                    Audio_Enable_Set(Audio_Enable_Get() ^ 1);
                    PauseMenuDraw();
                    break;
                case OPTIONS_GRAPHICS_NEW:
                    if (Room_Game_Graphics_New_Get())
                        Room_Game_Graphics_New_Set(0);
                    else
                        Room_Game_Graphics_New_Set(1);
                    Room_Game_Load_City_Graphics();
                    Simulation_TransportLoadGraphics();
                    BuildSelectMenuLoadGfx();
                    Room_Game_Load_Busy_Icon_Graphics();
                    PauseMenuDraw();
                    Jukebox_RoomSet(JUKEBOX_ROOM_GAME);
                    break;

                case PAUSE_MENU_INVALID_OPTION:
                    // Nothing to do, this is returned when no action needs to
                    // be taken.
                    break;
                default:
                    UGBA_Assert(0);
                    break;
            }

            GameAnimateMap();

            break;
        }
        default:
        {
            UGBA_Assert(0);
            break;
        }
    }
}

const char *Room_Game_Get_City_Name(void)
{
    return city_name;
}

void Room_Game_Set_City_Name(const char *name)
{
    // Save name with padding spaces before the actual name
    memset(city_name, ' ', sizeof(city_name));
    size_t l = strnlen(name, CITY_MAX_NAME_LENGTH);
    if (l == 0)
    {
        name = "No Name";
        l = strlen(name);
    }
    int src = 0;
    int dst = CITY_MAX_NAME_LENGTH - l - 1;
    for ( ; dst < (CITY_MAX_NAME_LENGTH - 1); dst++, src++)
        city_name[dst] = name[src];
    city_name[dst + 1] = '\0';
}

void Room_Game_Load_City(const void *map, const char *name,
                         int scroll_x, int scroll_y)
{
    Load_City_Data(map, scroll_x, scroll_y);
    Simulation_SetFirstStep();
    Room_Game_Set_City_Name(name);
}

void Room_Game_Set_City_Date(uint32_t month, uint32_t year)
{
    DateSet(month, year);
}

void Room_Game_Set_City_Economy(int money_amount, int tax_percentage,
                                int loan_payments, int payment_amount)
{
    MoneySet(money_amount);
    Simulation_TaxPercentageSet(tax_percentage);
    Room_Bank_Set_Loan(loan_payments, payment_amount);
}

static EWRAM_BSS uint16_t decompressed_map[CITY_MAP_WIDTH * CITY_MAP_HEIGHT];

// Returns 1 if the city has been loaded correctly
int Room_Game_City_Load(int slot_index)
{
    volatile city_save_data *sav_city = Save_Data_Get_City(slot_index);
    city_save_data *city = Save_Data_Get_City_Temporary();
    Save_Data_Safe_Copy(city, sav_city);

    if (city->name[0] == '\0')
        return 0;

    memcpy(city_name, city->name, CITY_MAX_NAME_LENGTH);

    DateSet(city->month, city->year);

    Simulation_SetCityClass(city->city_type);
    rand_slow_set_seed(city->rand_slow_seed);

    Room_Game_Set_City_Economy(city->funds, city->tax_percent,
                               city->loan_remaining_payments,
                               city->loan_payment_amount);

    Technology_SetLevel(city->technology_level);

    Simulation_NegativeBudgetCountSet(city->negative_budget_count);

    Graph_Data_Set(&(city->graph_population), GRAPH_INFO_POPULATION);
    Graph_Data_Set(&(city->graph_residential), GRAPH_INFO_RESIDENTIAL);
    Graph_Data_Set(&(city->graph_commercial), GRAPH_INFO_COMMERCIAL);
    Graph_Data_Set(&(city->graph_industrial), GRAPH_INFO_INDUSTRIAL);
    Graph_Data_Set(&(city->graph_funds), GRAPH_INFO_FUNDS);

    for (int j = 0; j < CITY_MAP_HEIGHT; j++)
    {
        for (int i = 0; i < CITY_MAP_WIDTH; i++)
        {
            uint16_t lsb = city->map_lsb[j * CITY_MAP_WIDTH + i];
            uint16_t msb = city->map_msb[(j * CITY_MAP_WIDTH + i) / 8];
            uint16_t bit_mask = 1 << (i % 8);
            if (msb & bit_mask)
                msb = 1 << 8;
            else
                msb = 0 << 8;
            uint16_t tile = lsb | msb;
            uint16_t vram_info = City_Tileset_VRAM_Info(tile);
            decompressed_map[j * CITY_MAP_WIDTH + i] = vram_info;
        }
    }

    Load_City_Data(decompressed_map, city->last_scroll_x, city->last_scroll_y);

    PersistentMessageFlagsGet(city->persistent_msg_flags);

    Simulation_SetFirstStep();

    return 1;
}

void Room_Game_City_Save(int slot_index)
{
    volatile city_save_data *sav_city = Save_Data_Get_City(slot_index);
    city_save_data *city = Save_Data_Get_City_Temporary();

    memcpy(city->name, city_name, CITY_MAX_NAME_LENGTH);

    city->month = DateGetMonth();
    city->year = DateGetYear();

    city->rand_slow_seed = rand_slow_get_seed();

    city->funds = MoneyGet();

    city->city_type = Simulation_GetCityClass();

    city->last_scroll_x = mapx / 8;
    city->last_scroll_y = mapy / 8;

    city->tax_percent = Simulation_TaxPercentageGet();

    city->technology_level = Technology_GetLevel();

    city->negative_budget_count = Simulation_NegativeBudgetCountGet();

    int payments, amount;
    Room_Bank_Get_Loan(&payments, &amount);
    city->loan_remaining_payments = payments;
    city->loan_payment_amount = amount;

    Graph_Data_Get(&(city->graph_population), GRAPH_INFO_POPULATION);
    Graph_Data_Get(&(city->graph_residential), GRAPH_INFO_RESIDENTIAL);
    Graph_Data_Get(&(city->graph_commercial), GRAPH_INFO_COMMERCIAL);
    Graph_Data_Get(&(city->graph_industrial), GRAPH_INFO_INDUSTRIAL);
    Graph_Data_Get(&(city->graph_funds), GRAPH_INFO_FUNDS);

    memset(city->map_msb, 0, sizeof(city->map_msb));
    for (int j = 0; j < CITY_MAP_HEIGHT; j++)
    {
        for (int i = 0; i < CITY_MAP_WIDTH; i++)
        {
            void *map = (void *)CITY_MAP_BASE;
            uint16_t tile = read_tile_sbb(map, i, j);
            city->map_lsb[j * CITY_MAP_WIDTH + i] = tile & 0xFF;

            uint16_t msb;
            uint16_t bit_mask = 1 << (i % 8);
            if (tile & (1 << 8))
                msb = bit_mask;
            else
                msb = 0;
            city->map_msb[(j * CITY_MAP_WIDTH + i) / 8] |= msb;
        }
    }

    PersistentMessageFlagsSet(city->persistent_msg_flags);

    Save_Data_Safe_Copy(sav_city, city);
}

void Room_Game_Settings_Load(void)
{
    volatile save_data *sav = Save_Data_Get();

    Simulation_DisastersSetEnabled(sav->disasters_enabled);
    Room_Game_SetAnimationsEnabled(sav->animations_enabled);
    Audio_Enable_Set(sav->music_enabled);
    Room_Game_Graphics_New_Set(sav->new_graphics);

    uint32_t seed = ((uint32_t)sav->rand_fast_seed[3] << 24) |
                    ((uint32_t)sav->rand_fast_seed[2] << 16) |
                    ((uint32_t)sav->rand_fast_seed[1] << 8) |
                    ((uint32_t)sav->rand_fast_seed[0] << 0);
    rand_fast_set_seed(seed);
}

void Room_Game_Settings_Save(void)
{
    volatile save_data *sav = Save_Data_Get();

    sav->disasters_enabled = Simulation_AreDisastersEnabled();
    sav->animations_enabled = Room_Game_AreAnimationsEnabled();
    sav->music_enabled = Audio_Enable_Get();
    sav->new_graphics = Room_Game_Graphics_New_Get();

    uint32_t seed = rand_fast_get_seed();
    sav->rand_fast_seed[3] = seed >> 24;
    sav->rand_fast_seed[2] = seed >> 16;
    sav->rand_fast_seed[1] = seed >> 8;
    sav->rand_fast_seed[0] = seed >> 0;

    Save_Reset_Checksum();
}
