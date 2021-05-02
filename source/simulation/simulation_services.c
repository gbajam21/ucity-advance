// SPDX-License-Identifier: GPL-3.0-only
//
// Copyright (c) 2021, Antonio Niño Díaz

#include <stdint.h>
#include <string.h>

#include <ugba/ugba.h>

#include "room_game/draw_common.h"
#include "room_game/room_game.h"

EWRAM_BSS static uint8_t services_matrix[CITY_MAP_WIDTH * CITY_MAP_HEIGHT];

uint8_t *Simulation_ServicesGetMap(void)
{
    return &services_matrix[0];
}

#define SERVICES_MASK_WIDTH     32
#define SERVICES_MASK_HEIGHT    32

#define SERVICES_MASK_CENTER_X  16
#define SERVICES_MASK_CENTER_Y  16

const uint8_t SERVICES_INFLUENCE_MASK[SERVICES_MASK_WIDTH * SERVICES_MASK_HEIGHT] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x0B, 0x10, 0x14, 0x17,
    0x18, 0x17, 0x14, 0x10, 0x0B, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x08, 0x12, 0x1B, 0x22, 0x28, 0x2C, 0x2F, 0x30, 0x2F, 0x2C, 0x28,
    0x22, 0x1B, 0x12, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x11, 0x1D, 0x28, 0x31,
    0x39, 0x3F, 0x44, 0x47, 0x48, 0x47, 0x44, 0x3F, 0x39, 0x31, 0x28, 0x1D,
    0x11, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x09, 0x18, 0x25, 0x32, 0x3E, 0x48, 0x50, 0x57, 0x5C, 0x5F,
    0x60, 0x5F, 0x5C, 0x57, 0x50, 0x48, 0x3E, 0x32, 0x25, 0x18, 0x09, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0A, 0x1B, 0x2A,
    0x39, 0x47, 0x53, 0x5E, 0x67, 0x6E, 0x73, 0x76, 0x78, 0x76, 0x73, 0x6E,
    0x67, 0x5E, 0x53, 0x47, 0x39, 0x2A, 0x1B, 0x0A, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x09, 0x1B, 0x2C, 0x3D, 0x4C, 0x5B, 0x68, 0x73,
    0x7D, 0x85, 0x8B, 0x8E, 0x90, 0x8E, 0x8B, 0x85, 0x7D, 0x73, 0x68, 0x5B,
    0x4C, 0x3D, 0x2C, 0x1B, 0x09, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04,
    0x18, 0x2A, 0x3D, 0x4E, 0x5F, 0x6E, 0x7C, 0x88, 0x93, 0x9C, 0xA2, 0xA6,
    0xA8, 0xA6, 0xA2, 0x9C, 0x93, 0x88, 0x7C, 0x6E, 0x5F, 0x4E, 0x3D, 0x2A,
    0x18, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x11, 0x25, 0x39, 0x4C, 0x5F,
    0x70, 0x80, 0x90, 0x9D, 0xA9, 0xB2, 0xBA, 0xBE, 0xC0, 0xBE, 0xBA, 0xB2,
    0xA9, 0x9D, 0x90, 0x80, 0x70, 0x5F, 0x4C, 0x39, 0x25, 0x11, 0x00, 0x00,
    0x00, 0x00, 0x08, 0x1D, 0x32, 0x47, 0x5B, 0x6E, 0x80, 0x92, 0xA2, 0xB1,
    0xBE, 0xC9, 0xD1, 0xD6, 0xD8, 0xD6, 0xD1, 0xC9, 0xBE, 0xB1, 0xA2, 0x92,
    0x80, 0x6E, 0x5B, 0x47, 0x32, 0x1D, 0x08, 0x00, 0x00, 0x00, 0x12, 0x28,
    0x3E, 0x53, 0x68, 0x7C, 0x90, 0xA2, 0xB4, 0xC4, 0xD2, 0xDF, 0xE8, 0xEE,
    0xF0, 0xEE, 0xE8, 0xDF, 0xD2, 0xC4, 0xB4, 0xA2, 0x90, 0x7C, 0x68, 0x53,
    0x3E, 0x28, 0x12, 0x00, 0x00, 0x04, 0x1B, 0x31, 0x48, 0x5E, 0x73, 0x88,
    0x9D, 0xB1, 0xC4, 0xD6, 0xE6, 0xF4, 0xFE, 0xFF, 0xFF, 0xFF, 0xFE, 0xF4,
    0xE6, 0xD6, 0xC4, 0xB1, 0x9D, 0x88, 0x73, 0x5E, 0x48, 0x31, 0x1B, 0x04,
    0x00, 0x0B, 0x22, 0x39, 0x50, 0x67, 0x7D, 0x93, 0xA9, 0xBE, 0xD2, 0xE6,
    0xF8, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xF8, 0xE6, 0xD2, 0xBE,
    0xA9, 0x93, 0x7D, 0x67, 0x50, 0x39, 0x22, 0x0B, 0x00, 0x10, 0x28, 0x3F,
    0x57, 0x6E, 0x85, 0x9C, 0xB2, 0xC9, 0xDF, 0xF4, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xF4, 0xDF, 0xC9, 0xB2, 0x9C, 0x85, 0x6E,
    0x57, 0x3F, 0x28, 0x10, 0x00, 0x14, 0x2C, 0x44, 0x5C, 0x73, 0x8B, 0xA2,
    0xBA, 0xD1, 0xE8, 0xFE, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFE, 0xE8, 0xD1, 0xBA, 0xA2, 0x8B, 0x73, 0x5C, 0x44, 0x2C, 0x14,
    0x00, 0x17, 0x2F, 0x47, 0x5F, 0x76, 0x8E, 0xA6, 0xBE, 0xD6, 0xEE, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xEE, 0xD6,
    0xBE, 0xA6, 0x8E, 0x76, 0x5F, 0x47, 0x2F, 0x17, 0x00, 0x18, 0x30, 0x48,
    0x60, 0x78, 0x90, 0xA8, 0xC0, 0xD8, 0xF0, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xF0, 0xD8, 0xC0, 0xA8, 0x90, 0x78,
    0x60, 0x48, 0x30, 0x18, 0x00, 0x17, 0x2F, 0x47, 0x5F, 0x76, 0x8E, 0xA6,
    0xBE, 0xD6, 0xEE, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xEE, 0xD6, 0xBE, 0xA6, 0x8E, 0x76, 0x5F, 0x47, 0x2F, 0x17,
    0x00, 0x14, 0x2C, 0x44, 0x5C, 0x73, 0x8B, 0xA2, 0xBA, 0xD1, 0xE8, 0xFE,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFE, 0xE8, 0xD1,
    0xBA, 0xA2, 0x8B, 0x73, 0x5C, 0x44, 0x2C, 0x14, 0x00, 0x10, 0x28, 0x3F,
    0x57, 0x6E, 0x85, 0x9C, 0xB2, 0xC9, 0xDF, 0xF4, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xF4, 0xDF, 0xC9, 0xB2, 0x9C, 0x85, 0x6E,
    0x57, 0x3F, 0x28, 0x10, 0x00, 0x0B, 0x22, 0x39, 0x50, 0x67, 0x7D, 0x93,
    0xA9, 0xBE, 0xD2, 0xE6, 0xF8, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xF8, 0xE6, 0xD2, 0xBE, 0xA9, 0x93, 0x7D, 0x67, 0x50, 0x39, 0x22, 0x0B,
    0x00, 0x04, 0x1B, 0x31, 0x48, 0x5E, 0x73, 0x88, 0x9D, 0xB1, 0xC4, 0xD6,
    0xE6, 0xF4, 0xFE, 0xFF, 0xFF, 0xFF, 0xFE, 0xF4, 0xE6, 0xD6, 0xC4, 0xB1,
    0x9D, 0x88, 0x73, 0x5E, 0x48, 0x31, 0x1B, 0x04, 0x00, 0x00, 0x12, 0x28,
    0x3E, 0x53, 0x68, 0x7C, 0x90, 0xA2, 0xB4, 0xC4, 0xD2, 0xDF, 0xE8, 0xEE,
    0xF0, 0xEE, 0xE8, 0xDF, 0xD2, 0xC4, 0xB4, 0xA2, 0x90, 0x7C, 0x68, 0x53,
    0x3E, 0x28, 0x12, 0x00, 0x00, 0x00, 0x08, 0x1D, 0x32, 0x47, 0x5B, 0x6E,
    0x80, 0x92, 0xA2, 0xB1, 0xBE, 0xC9, 0xD1, 0xD6, 0xD8, 0xD6, 0xD1, 0xC9,
    0xBE, 0xB1, 0xA2, 0x92, 0x80, 0x6E, 0x5B, 0x47, 0x32, 0x1D, 0x08, 0x00,
    0x00, 0x00, 0x00, 0x11, 0x25, 0x39, 0x4C, 0x5F, 0x70, 0x80, 0x90, 0x9D,
    0xA9, 0xB2, 0xBA, 0xBE, 0xC0, 0xBE, 0xBA, 0xB2, 0xA9, 0x9D, 0x90, 0x80,
    0x70, 0x5F, 0x4C, 0x39, 0x25, 0x11, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04,
    0x18, 0x2A, 0x3D, 0x4E, 0x5F, 0x6E, 0x7C, 0x88, 0x93, 0x9C, 0xA2, 0xA6,
    0xA8, 0xA6, 0xA2, 0x9C, 0x93, 0x88, 0x7C, 0x6E, 0x5F, 0x4E, 0x3D, 0x2A,
    0x18, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x09, 0x1B, 0x2C, 0x3D,
    0x4C, 0x5B, 0x68, 0x73, 0x7D, 0x85, 0x8B, 0x8E, 0x90, 0x8E, 0x8B, 0x85,
    0x7D, 0x73, 0x68, 0x5B, 0x4C, 0x3D, 0x2C, 0x1B, 0x09, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x0A, 0x1B, 0x2A, 0x39, 0x47, 0x53, 0x5E,
    0x67, 0x6E, 0x73, 0x76, 0x78, 0x76, 0x73, 0x6E, 0x67, 0x5E, 0x53, 0x47,
    0x39, 0x2A, 0x1B, 0x0A, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x09, 0x18, 0x25, 0x32, 0x3E, 0x48, 0x50, 0x57, 0x5C, 0x5F,
    0x60, 0x5F, 0x5C, 0x57, 0x50, 0x48, 0x3E, 0x32, 0x25, 0x18, 0x09, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04,
    0x11, 0x1D, 0x28, 0x31, 0x39, 0x3F, 0x44, 0x47, 0x48, 0x47, 0x44, 0x3F,
    0x39, 0x31, 0x28, 0x1D, 0x11, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x12, 0x1B,
    0x22, 0x28, 0x2C, 0x2F, 0x30, 0x2F, 0x2C, 0x28, 0x22, 0x1B, 0x12, 0x08,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x0B, 0x10, 0x14, 0x17,
    0x18, 0x17, 0x14, 0x10, 0x0B, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
};

// Coordinates are the center of the mask
static void Simulation_ServicesApplyMask(int x, int y)
{
    int sx = x - SERVICES_MASK_CENTER_X;
    int sy = y - SERVICES_MASK_CENTER_Y;

    for (int j = 0; j < SERVICES_MASK_HEIGHT; j++)
    {
        int mapy = j + sy;

        if (mapy < 0)
            continue;

        if (mapy >= CITY_MAP_HEIGHT)
            break;

        for (int i = 0; i < SERVICES_MASK_WIDTH; i++)
        {
            int mapx = i + sx;

            if (mapx < 0)
                continue;

            if (mapx >= CITY_MAP_WIDTH)
                break;

            int val = SERVICES_INFLUENCE_MASK[j * SERVICES_MASK_WIDTH + i];
            int old = services_matrix[mapy * CITY_MAP_WIDTH + mapx];

            val += old;

            if (val > 255)
                services_matrix[mapy * CITY_MAP_WIDTH + mapx] = 255;
            else
                services_matrix[mapy * CITY_MAP_WIDTH + mapx] = val;
        }
    }
}

// Central tile of the building (tileset_info.h)
void Simulation_Services(uint16_t source_tile)
{
    memset(services_matrix, 0, sizeof(services_matrix));

    for (int j = 0; j < CITY_MAP_HEIGHT; j++)
    {
        for (int i = 0; i < CITY_MAP_WIDTH; i++)
        {
            if (CityMapGetTile(i, j) == source_tile)
            {
                // TODO: If no power, ignore this building
                // TILE_OK_POWER_BIT
                Simulation_ServicesApplyMask(i, j);
            }
        }
    }
}

#define SERVICES_MASK_BIG_WIDTH     64
#define SERVICES_MASK_BIG_HEIGHT    64

#define SERVICES_MASK_BIG_CENTER_X  32
#define SERVICES_MASK_BIG_CENTER_Y  32

static uint8_t SERVICES_INFLUENCE_MASK_BIG[
                        SERVICES_MASK_BIG_WIDTH * SERVICES_MASK_BIG_HEIGHT] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x05, 0x07, 0x08, 0x0A, 0x0B, 0x0B,
    0x0C, 0x0B, 0x0B, 0x0A, 0x08, 0x07, 0x05, 0x02, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x08, 0x0B, 0x0E, 0x10, 0x13,
    0x14, 0x16, 0x17, 0x17, 0x18, 0x17, 0x17, 0x16, 0x14, 0x13, 0x10, 0x0E,
    0x0B, 0x08, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x07, 0x0B, 0x0F, 0x13,
    0x17, 0x1A, 0x1C, 0x1E, 0x20, 0x22, 0x23, 0x23, 0x24, 0x23, 0x23, 0x22,
    0x20, 0x1E, 0x1C, 0x1A, 0x17, 0x13, 0x0F, 0x0B, 0x07, 0x02, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x08, 0x0D,
    0x12, 0x17, 0x1B, 0x1F, 0x22, 0x25, 0x28, 0x2A, 0x2C, 0x2E, 0x2F, 0x2F,
    0x30, 0x2F, 0x2F, 0x2E, 0x2C, 0x2A, 0x28, 0x25, 0x22, 0x1F, 0x1B, 0x17,
    0x12, 0x0D, 0x08, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
    0x07, 0x0D, 0x13, 0x18, 0x1D, 0x22, 0x26, 0x2A, 0x2E, 0x31, 0x34, 0x36,
    0x38, 0x3A, 0x3B, 0x3B, 0x3C, 0x3B, 0x3B, 0x3A, 0x38, 0x36, 0x34, 0x31,
    0x2E, 0x2A, 0x26, 0x22, 0x1D, 0x18, 0x13, 0x0D, 0x07, 0x01, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x04, 0x0B, 0x11, 0x17, 0x1D, 0x23, 0x28, 0x2D, 0x31, 0x35,
    0x39, 0x3C, 0x3F, 0x42, 0x44, 0x45, 0x47, 0x47, 0x48, 0x47, 0x47, 0x45,
    0x44, 0x42, 0x3F, 0x3C, 0x39, 0x35, 0x31, 0x2D, 0x28, 0x23, 0x1D, 0x17,
    0x11, 0x0B, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0x0E, 0x15, 0x1B, 0x22, 0x28, 0x2D,
    0x33, 0x38, 0x3C, 0x41, 0x45, 0x48, 0x4B, 0x4E, 0x50, 0x51, 0x53, 0x53,
    0x54, 0x53, 0x53, 0x51, 0x50, 0x4E, 0x4B, 0x48, 0x45, 0x41, 0x3C, 0x38,
    0x33, 0x2D, 0x28, 0x22, 0x1B, 0x15, 0x0E, 0x07, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x09, 0x10, 0x18, 0x1F,
    0x25, 0x2C, 0x32, 0x38, 0x3E, 0x43, 0x48, 0x4C, 0x50, 0x54, 0x57, 0x59,
    0x5C, 0x5D, 0x5F, 0x5F, 0x60, 0x5F, 0x5F, 0x5D, 0x5C, 0x59, 0x57, 0x54,
    0x50, 0x4C, 0x48, 0x43, 0x3E, 0x38, 0x32, 0x2C, 0x25, 0x1F, 0x18, 0x10,
    0x09, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x0A,
    0x12, 0x1A, 0x21, 0x28, 0x2F, 0x36, 0x3C, 0x42, 0x48, 0x4E, 0x53, 0x57,
    0x5B, 0x5F, 0x62, 0x65, 0x67, 0x69, 0x6A, 0x6B, 0x6C, 0x6B, 0x6A, 0x69,
    0x67, 0x65, 0x62, 0x5F, 0x5B, 0x57, 0x53, 0x4E, 0x48, 0x42, 0x3C, 0x36,
    0x2F, 0x28, 0x21, 0x1A, 0x12, 0x0A, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x02, 0x0A, 0x13, 0x1B, 0x23, 0x2A, 0x32, 0x39, 0x40, 0x47, 0x4D,
    0x53, 0x58, 0x5E, 0x62, 0x67, 0x6A, 0x6E, 0x71, 0x73, 0x75, 0x76, 0x77,
    0x78, 0x77, 0x76, 0x75, 0x73, 0x71, 0x6E, 0x6A, 0x67, 0x62, 0x5E, 0x58,
    0x53, 0x4D, 0x47, 0x40, 0x39, 0x32, 0x2A, 0x23, 0x1B, 0x13, 0x0A, 0x02,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x01, 0x0A, 0x13, 0x1B, 0x24, 0x2C, 0x34, 0x3B,
    0x43, 0x4A, 0x51, 0x57, 0x5D, 0x63, 0x68, 0x6D, 0x72, 0x76, 0x79, 0x7C,
    0x7F, 0x81, 0x82, 0x83, 0x84, 0x83, 0x82, 0x81, 0x7F, 0x7C, 0x79, 0x76,
    0x72, 0x6D, 0x68, 0x63, 0x5D, 0x57, 0x51, 0x4A, 0x43, 0x3B, 0x34, 0x2C,
    0x24, 0x1B, 0x13, 0x0A, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x09, 0x12, 0x1B, 0x24,
    0x2C, 0x34, 0x3D, 0x45, 0x4C, 0x54, 0x5B, 0x61, 0x68, 0x6E, 0x73, 0x78,
    0x7D, 0x81, 0x85, 0x88, 0x8B, 0x8D, 0x8E, 0x8F, 0x90, 0x8F, 0x8E, 0x8D,
    0x8B, 0x88, 0x85, 0x81, 0x7D, 0x78, 0x73, 0x6E, 0x68, 0x61, 0x5B, 0x54,
    0x4C, 0x45, 0x3D, 0x34, 0x2C, 0x24, 0x1B, 0x12, 0x09, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x07,
    0x10, 0x1A, 0x23, 0x2C, 0x34, 0x3D, 0x45, 0x4E, 0x55, 0x5D, 0x64, 0x6B,
    0x72, 0x78, 0x7E, 0x83, 0x88, 0x8D, 0x90, 0x94, 0x97, 0x99, 0x9A, 0x9B,
    0x9C, 0x9B, 0x9A, 0x99, 0x97, 0x94, 0x90, 0x8D, 0x88, 0x83, 0x7E, 0x78,
    0x72, 0x6B, 0x64, 0x5D, 0x55, 0x4E, 0x45, 0x3D, 0x34, 0x2C, 0x23, 0x1A,
    0x10, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x04, 0x0E, 0x18, 0x21, 0x2A, 0x34, 0x3D, 0x45, 0x4E, 0x56,
    0x5F, 0x66, 0x6E, 0x75, 0x7C, 0x82, 0x88, 0x8E, 0x93, 0x98, 0x9C, 0x9F,
    0xA2, 0xA5, 0xA6, 0xA7, 0xA8, 0xA7, 0xA6, 0xA5, 0xA2, 0x9F, 0x9C, 0x98,
    0x93, 0x8E, 0x88, 0x82, 0x7C, 0x75, 0x6E, 0x66, 0x5F, 0x56, 0x4E, 0x45,
    0x3D, 0x34, 0x2A, 0x21, 0x18, 0x0E, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x0B, 0x15, 0x1F, 0x28, 0x32, 0x3B,
    0x45, 0x4E, 0x56, 0x5F, 0x67, 0x6F, 0x77, 0x7F, 0x86, 0x8D, 0x93, 0x99,
    0x9E, 0xA3, 0xA7, 0xAB, 0xAE, 0xB0, 0xB2, 0xB3, 0xB4, 0xB3, 0xB2, 0xB0,
    0xAE, 0xAB, 0xA7, 0xA3, 0x9E, 0x99, 0x93, 0x8D, 0x86, 0x7F, 0x77, 0x6F,
    0x67, 0x5F, 0x56, 0x4E, 0x45, 0x3B, 0x32, 0x28, 0x1F, 0x15, 0x0B, 0x01,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0x11, 0x1B,
    0x25, 0x2F, 0x39, 0x43, 0x4C, 0x55, 0x5F, 0x67, 0x70, 0x78, 0x80, 0x88,
    0x90, 0x97, 0x9D, 0xA3, 0xA9, 0xAE, 0xB2, 0xB6, 0xBA, 0xBC, 0xBE, 0xBF,
    0xC0, 0xBF, 0xBE, 0xBC, 0xBA, 0xB6, 0xB2, 0xAE, 0xA9, 0xA3, 0x9D, 0x97,
    0x90, 0x88, 0x80, 0x78, 0x70, 0x67, 0x5F, 0x55, 0x4C, 0x43, 0x39, 0x2F,
    0x25, 0x1B, 0x11, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x02, 0x0D, 0x17, 0x22, 0x2C, 0x36, 0x40, 0x4A, 0x54, 0x5D, 0x66, 0x6F,
    0x78, 0x81, 0x89, 0x91, 0x99, 0xA0, 0xA7, 0xAE, 0xB4, 0xB9, 0xBE, 0xC2,
    0xC5, 0xC8, 0xCA, 0xCB, 0xCC, 0xCB, 0xCA, 0xC8, 0xC5, 0xC2, 0xBE, 0xB9,
    0xB4, 0xAE, 0xA7, 0xA0, 0x99, 0x91, 0x89, 0x81, 0x78, 0x6F, 0x66, 0x5D,
    0x54, 0x4A, 0x40, 0x36, 0x2C, 0x22, 0x17, 0x0D, 0x02, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x08, 0x13, 0x1D, 0x28, 0x32, 0x3C, 0x47, 0x51,
    0x5B, 0x64, 0x6E, 0x77, 0x80, 0x89, 0x92, 0x9A, 0xA2, 0xAA, 0xB1, 0xB8,
    0xBE, 0xC4, 0xC9, 0xCD, 0xD1, 0xD4, 0xD6, 0xD7, 0xD8, 0xD7, 0xD6, 0xD4,
    0xD1, 0xCD, 0xC9, 0xC4, 0xBE, 0xB8, 0xB1, 0xAA, 0xA2, 0x9A, 0x92, 0x89,
    0x80, 0x77, 0x6E, 0x64, 0x5B, 0x51, 0x47, 0x3C, 0x32, 0x28, 0x1D, 0x13,
    0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x0D, 0x18, 0x23, 0x2D,
    0x38, 0x42, 0x4D, 0x57, 0x61, 0x6B, 0x75, 0x7F, 0x88, 0x91, 0x9A, 0xA3,
    0xAB, 0xB3, 0xBB, 0xC2, 0xC8, 0xCE, 0xD4, 0xD8, 0xDC, 0xDF, 0xE2, 0xE3,
    0xE4, 0xE3, 0xE2, 0xDF, 0xDC, 0xD8, 0xD4, 0xCE, 0xC8, 0xC2, 0xBB, 0xB3,
    0xAB, 0xA3, 0x9A, 0x91, 0x88, 0x7F, 0x75, 0x6B, 0x61, 0x57, 0x4D, 0x42,
    0x38, 0x2D, 0x23, 0x18, 0x0D, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x07,
    0x12, 0x1D, 0x28, 0x33, 0x3E, 0x48, 0x53, 0x5D, 0x68, 0x72, 0x7C, 0x86,
    0x90, 0x99, 0xA2, 0xAB, 0xB4, 0xBC, 0xC4, 0xCC, 0xD2, 0xD9, 0xDF, 0xE4,
    0xE8, 0xEB, 0xEE, 0xEF, 0xF0, 0xEF, 0xEE, 0xEB, 0xE8, 0xE4, 0xDF, 0xD9,
    0xD2, 0xCC, 0xC4, 0xBC, 0xB4, 0xAB, 0xA2, 0x99, 0x90, 0x86, 0x7C, 0x72,
    0x68, 0x5D, 0x53, 0x48, 0x3E, 0x33, 0x28, 0x1D, 0x12, 0x07, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x0B, 0x17, 0x22, 0x2D, 0x38, 0x43, 0x4E, 0x58, 0x63,
    0x6E, 0x78, 0x82, 0x8D, 0x97, 0xA0, 0xAA, 0xB3, 0xBC, 0xC5, 0xCD, 0xD5,
    0xDC, 0xE3, 0xE9, 0xEF, 0xF3, 0xF7, 0xF9, 0xFB, 0xFC, 0xFB, 0xF9, 0xF7,
    0xF3, 0xEF, 0xE9, 0xE3, 0xDC, 0xD5, 0xCD, 0xC5, 0xBC, 0xB3, 0xAA, 0xA0,
    0x97, 0x8D, 0x82, 0x78, 0x6E, 0x63, 0x58, 0x4E, 0x43, 0x38, 0x2D, 0x22,
    0x17, 0x0B, 0x00, 0x00, 0x00, 0x00, 0x04, 0x0F, 0x1B, 0x26, 0x31, 0x3C,
    0x48, 0x53, 0x5E, 0x68, 0x73, 0x7E, 0x88, 0x93, 0x9D, 0xA7, 0xB1, 0xBB,
    0xC4, 0xCD, 0xD6, 0xDE, 0xE6, 0xED, 0xF4, 0xF9, 0xFE, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFE, 0xF9, 0xF4, 0xED, 0xE6, 0xDE, 0xD6, 0xCD,
    0xC4, 0xBB, 0xB1, 0xA7, 0x9D, 0x93, 0x88, 0x7E, 0x73, 0x68, 0x5E, 0x53,
    0x48, 0x3C, 0x31, 0x26, 0x1B, 0x0F, 0x04, 0x00, 0x00, 0x00, 0x08, 0x13,
    0x1F, 0x2A, 0x35, 0x41, 0x4C, 0x57, 0x62, 0x6D, 0x78, 0x83, 0x8E, 0x99,
    0xA3, 0xAE, 0xB8, 0xC2, 0xCC, 0xD5, 0xDE, 0xE7, 0xEF, 0xF7, 0xFE, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFE, 0xF7,
    0xEF, 0xE7, 0xDE, 0xD5, 0xCC, 0xC2, 0xB8, 0xAE, 0xA3, 0x99, 0x8E, 0x83,
    0x78, 0x6D, 0x62, 0x57, 0x4C, 0x41, 0x35, 0x2A, 0x1F, 0x13, 0x08, 0x00,
    0x00, 0x00, 0x0B, 0x17, 0x22, 0x2E, 0x39, 0x45, 0x50, 0x5B, 0x67, 0x72,
    0x7D, 0x88, 0x93, 0x9E, 0xA9, 0xB4, 0xBE, 0xC8, 0xD2, 0xDC, 0xE6, 0xEF,
    0xF8, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xF8, 0xEF, 0xE6, 0xDC, 0xD2, 0xC8, 0xBE, 0xB4,
    0xA9, 0x9E, 0x93, 0x88, 0x7D, 0x72, 0x67, 0x5B, 0x50, 0x45, 0x39, 0x2E,
    0x22, 0x17, 0x0B, 0x00, 0x00, 0x02, 0x0E, 0x1A, 0x25, 0x31, 0x3C, 0x48,
    0x54, 0x5F, 0x6A, 0x76, 0x81, 0x8D, 0x98, 0xA3, 0xAE, 0xB9, 0xC4, 0xCE,
    0xD9, 0xE3, 0xED, 0xF7, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xF7, 0xED, 0xE3,
    0xD9, 0xCE, 0xC4, 0xB9, 0xAE, 0xA3, 0x98, 0x8D, 0x81, 0x76, 0x6A, 0x5F,
    0x54, 0x48, 0x3C, 0x31, 0x25, 0x1A, 0x0E, 0x02, 0x00, 0x05, 0x10, 0x1C,
    0x28, 0x34, 0x3F, 0x4B, 0x57, 0x62, 0x6E, 0x79, 0x85, 0x90, 0x9C, 0xA7,
    0xB2, 0xBE, 0xC9, 0xD4, 0xDF, 0xE9, 0xF4, 0xFE, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFE, 0xF4, 0xE9, 0xDF, 0xD4, 0xC9, 0xBE, 0xB2, 0xA7, 0x9C, 0x90,
    0x85, 0x79, 0x6E, 0x62, 0x57, 0x4B, 0x3F, 0x34, 0x28, 0x1C, 0x10, 0x05,
    0x00, 0x07, 0x13, 0x1E, 0x2A, 0x36, 0x42, 0x4E, 0x59, 0x65, 0x71, 0x7C,
    0x88, 0x94, 0x9F, 0xAB, 0xB6, 0xC2, 0xCD, 0xD8, 0xE4, 0xEF, 0xF9, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xF9, 0xEF, 0xE4, 0xD8, 0xCD, 0xC2,
    0xB6, 0xAB, 0x9F, 0x94, 0x88, 0x7C, 0x71, 0x65, 0x59, 0x4E, 0x42, 0x36,
    0x2A, 0x1E, 0x13, 0x07, 0x00, 0x08, 0x14, 0x20, 0x2C, 0x38, 0x44, 0x50,
    0x5C, 0x67, 0x73, 0x7F, 0x8B, 0x97, 0xA2, 0xAE, 0xBA, 0xC5, 0xD1, 0xDC,
    0xE8, 0xF3, 0xFE, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFE, 0xF3,
    0xE8, 0xDC, 0xD1, 0xC5, 0xBA, 0xAE, 0xA2, 0x97, 0x8B, 0x7F, 0x73, 0x67,
    0x5C, 0x50, 0x44, 0x38, 0x2C, 0x20, 0x14, 0x08, 0x00, 0x0A, 0x16, 0x22,
    0x2E, 0x3A, 0x45, 0x51, 0x5D, 0x69, 0x75, 0x81, 0x8D, 0x99, 0xA5, 0xB0,
    0xBC, 0xC8, 0xD4, 0xDF, 0xEB, 0xF7, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xF7, 0xEB, 0xDF, 0xD4, 0xC8, 0xBC, 0xB0, 0xA5, 0x99,
    0x8D, 0x81, 0x75, 0x69, 0x5D, 0x51, 0x45, 0x3A, 0x2E, 0x22, 0x16, 0x0A,
    0x00, 0x0B, 0x17, 0x23, 0x2F, 0x3B, 0x47, 0x53, 0x5F, 0x6A, 0x76, 0x82,
    0x8E, 0x9A, 0xA6, 0xB2, 0xBE, 0xCA, 0xD6, 0xE2, 0xEE, 0xF9, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xF9, 0xEE, 0xE2, 0xD6, 0xCA,
    0xBE, 0xB2, 0xA6, 0x9A, 0x8E, 0x82, 0x76, 0x6A, 0x5F, 0x53, 0x47, 0x3B,
    0x2F, 0x23, 0x17, 0x0B, 0x00, 0x0B, 0x17, 0x23, 0x2F, 0x3B, 0x47, 0x53,
    0x5F, 0x6B, 0x77, 0x83, 0x8F, 0x9B, 0xA7, 0xB3, 0xBF, 0xCB, 0xD7, 0xE3,
    0xEF, 0xFB, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFB,
    0xEF, 0xE3, 0xD7, 0xCB, 0xBF, 0xB3, 0xA7, 0x9B, 0x8F, 0x83, 0x77, 0x6B,
    0x5F, 0x53, 0x47, 0x3B, 0x2F, 0x23, 0x17, 0x0B, 0x00, 0x0C, 0x18, 0x24,
    0x30, 0x3C, 0x48, 0x54, 0x60, 0x6C, 0x78, 0x84, 0x90, 0x9C, 0xA8, 0xB4,
    0xC0, 0xCC, 0xD8, 0xE4, 0xF0, 0xFC, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFC, 0xF0, 0xE4, 0xD8, 0xCC, 0xC0, 0xB4, 0xA8, 0x9C,
    0x90, 0x84, 0x78, 0x6C, 0x60, 0x54, 0x48, 0x3C, 0x30, 0x24, 0x18, 0x0C,
    0x00, 0x0B, 0x17, 0x23, 0x2F, 0x3B, 0x47, 0x53, 0x5F, 0x6B, 0x77, 0x83,
    0x8F, 0x9B, 0xA7, 0xB3, 0xBF, 0xCB, 0xD7, 0xE3, 0xEF, 0xFB, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFB, 0xEF, 0xE3, 0xD7, 0xCB,
    0xBF, 0xB3, 0xA7, 0x9B, 0x8F, 0x83, 0x77, 0x6B, 0x5F, 0x53, 0x47, 0x3B,
    0x2F, 0x23, 0x17, 0x0B, 0x00, 0x0B, 0x17, 0x23, 0x2F, 0x3B, 0x47, 0x53,
    0x5F, 0x6A, 0x76, 0x82, 0x8E, 0x9A, 0xA6, 0xB2, 0xBE, 0xCA, 0xD6, 0xE2,
    0xEE, 0xF9, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xF9,
    0xEE, 0xE2, 0xD6, 0xCA, 0xBE, 0xB2, 0xA6, 0x9A, 0x8E, 0x82, 0x76, 0x6A,
    0x5F, 0x53, 0x47, 0x3B, 0x2F, 0x23, 0x17, 0x0B, 0x00, 0x0A, 0x16, 0x22,
    0x2E, 0x3A, 0x45, 0x51, 0x5D, 0x69, 0x75, 0x81, 0x8D, 0x99, 0xA5, 0xB0,
    0xBC, 0xC8, 0xD4, 0xDF, 0xEB, 0xF7, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xF7, 0xEB, 0xDF, 0xD4, 0xC8, 0xBC, 0xB0, 0xA5, 0x99,
    0x8D, 0x81, 0x75, 0x69, 0x5D, 0x51, 0x45, 0x3A, 0x2E, 0x22, 0x16, 0x0A,
    0x00, 0x08, 0x14, 0x20, 0x2C, 0x38, 0x44, 0x50, 0x5C, 0x67, 0x73, 0x7F,
    0x8B, 0x97, 0xA2, 0xAE, 0xBA, 0xC5, 0xD1, 0xDC, 0xE8, 0xF3, 0xFE, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFE, 0xF3, 0xE8, 0xDC, 0xD1, 0xC5,
    0xBA, 0xAE, 0xA2, 0x97, 0x8B, 0x7F, 0x73, 0x67, 0x5C, 0x50, 0x44, 0x38,
    0x2C, 0x20, 0x14, 0x08, 0x00, 0x07, 0x13, 0x1E, 0x2A, 0x36, 0x42, 0x4E,
    0x59, 0x65, 0x71, 0x7C, 0x88, 0x94, 0x9F, 0xAB, 0xB6, 0xC2, 0xCD, 0xD8,
    0xE4, 0xEF, 0xF9, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xF9, 0xEF,
    0xE4, 0xD8, 0xCD, 0xC2, 0xB6, 0xAB, 0x9F, 0x94, 0x88, 0x7C, 0x71, 0x65,
    0x59, 0x4E, 0x42, 0x36, 0x2A, 0x1E, 0x13, 0x07, 0x00, 0x05, 0x10, 0x1C,
    0x28, 0x34, 0x3F, 0x4B, 0x57, 0x62, 0x6E, 0x79, 0x85, 0x90, 0x9C, 0xA7,
    0xB2, 0xBE, 0xC9, 0xD4, 0xDF, 0xE9, 0xF4, 0xFE, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFE, 0xF4, 0xE9, 0xDF, 0xD4, 0xC9, 0xBE, 0xB2, 0xA7, 0x9C, 0x90,
    0x85, 0x79, 0x6E, 0x62, 0x57, 0x4B, 0x3F, 0x34, 0x28, 0x1C, 0x10, 0x05,
    0x00, 0x02, 0x0E, 0x1A, 0x25, 0x31, 0x3C, 0x48, 0x54, 0x5F, 0x6A, 0x76,
    0x81, 0x8D, 0x98, 0xA3, 0xAE, 0xB9, 0xC4, 0xCE, 0xD9, 0xE3, 0xED, 0xF7,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xF7, 0xED, 0xE3, 0xD9, 0xCE, 0xC4, 0xB9,
    0xAE, 0xA3, 0x98, 0x8D, 0x81, 0x76, 0x6A, 0x5F, 0x54, 0x48, 0x3C, 0x31,
    0x25, 0x1A, 0x0E, 0x02, 0x00, 0x00, 0x0B, 0x17, 0x22, 0x2E, 0x39, 0x45,
    0x50, 0x5B, 0x67, 0x72, 0x7D, 0x88, 0x93, 0x9E, 0xA9, 0xB4, 0xBE, 0xC8,
    0xD2, 0xDC, 0xE6, 0xEF, 0xF8, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xF8, 0xEF, 0xE6, 0xDC,
    0xD2, 0xC8, 0xBE, 0xB4, 0xA9, 0x9E, 0x93, 0x88, 0x7D, 0x72, 0x67, 0x5B,
    0x50, 0x45, 0x39, 0x2E, 0x22, 0x17, 0x0B, 0x00, 0x00, 0x00, 0x08, 0x13,
    0x1F, 0x2A, 0x35, 0x41, 0x4C, 0x57, 0x62, 0x6D, 0x78, 0x83, 0x8E, 0x99,
    0xA3, 0xAE, 0xB8, 0xC2, 0xCC, 0xD5, 0xDE, 0xE7, 0xEF, 0xF7, 0xFE, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFE, 0xF7,
    0xEF, 0xE7, 0xDE, 0xD5, 0xCC, 0xC2, 0xB8, 0xAE, 0xA3, 0x99, 0x8E, 0x83,
    0x78, 0x6D, 0x62, 0x57, 0x4C, 0x41, 0x35, 0x2A, 0x1F, 0x13, 0x08, 0x00,
    0x00, 0x00, 0x04, 0x0F, 0x1B, 0x26, 0x31, 0x3C, 0x48, 0x53, 0x5E, 0x68,
    0x73, 0x7E, 0x88, 0x93, 0x9D, 0xA7, 0xB1, 0xBB, 0xC4, 0xCD, 0xD6, 0xDE,
    0xE6, 0xED, 0xF4, 0xF9, 0xFE, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFE, 0xF9, 0xF4, 0xED, 0xE6, 0xDE, 0xD6, 0xCD, 0xC4, 0xBB, 0xB1, 0xA7,
    0x9D, 0x93, 0x88, 0x7E, 0x73, 0x68, 0x5E, 0x53, 0x48, 0x3C, 0x31, 0x26,
    0x1B, 0x0F, 0x04, 0x00, 0x00, 0x00, 0x00, 0x0B, 0x17, 0x22, 0x2D, 0x38,
    0x43, 0x4E, 0x58, 0x63, 0x6E, 0x78, 0x82, 0x8D, 0x97, 0xA0, 0xAA, 0xB3,
    0xBC, 0xC5, 0xCD, 0xD5, 0xDC, 0xE3, 0xE9, 0xEF, 0xF3, 0xF7, 0xF9, 0xFB,
    0xFC, 0xFB, 0xF9, 0xF7, 0xF3, 0xEF, 0xE9, 0xE3, 0xDC, 0xD5, 0xCD, 0xC5,
    0xBC, 0xB3, 0xAA, 0xA0, 0x97, 0x8D, 0x82, 0x78, 0x6E, 0x63, 0x58, 0x4E,
    0x43, 0x38, 0x2D, 0x22, 0x17, 0x0B, 0x00, 0x00, 0x00, 0x00, 0x00, 0x07,
    0x12, 0x1D, 0x28, 0x33, 0x3E, 0x48, 0x53, 0x5D, 0x68, 0x72, 0x7C, 0x86,
    0x90, 0x99, 0xA2, 0xAB, 0xB4, 0xBC, 0xC4, 0xCC, 0xD2, 0xD9, 0xDF, 0xE4,
    0xE8, 0xEB, 0xEE, 0xEF, 0xF0, 0xEF, 0xEE, 0xEB, 0xE8, 0xE4, 0xDF, 0xD9,
    0xD2, 0xCC, 0xC4, 0xBC, 0xB4, 0xAB, 0xA2, 0x99, 0x90, 0x86, 0x7C, 0x72,
    0x68, 0x5D, 0x53, 0x48, 0x3E, 0x33, 0x28, 0x1D, 0x12, 0x07, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x02, 0x0D, 0x18, 0x23, 0x2D, 0x38, 0x42, 0x4D, 0x57,
    0x61, 0x6B, 0x75, 0x7F, 0x88, 0x91, 0x9A, 0xA3, 0xAB, 0xB3, 0xBB, 0xC2,
    0xC8, 0xCE, 0xD4, 0xD8, 0xDC, 0xDF, 0xE2, 0xE3, 0xE4, 0xE3, 0xE2, 0xDF,
    0xDC, 0xD8, 0xD4, 0xCE, 0xC8, 0xC2, 0xBB, 0xB3, 0xAB, 0xA3, 0x9A, 0x91,
    0x88, 0x7F, 0x75, 0x6B, 0x61, 0x57, 0x4D, 0x42, 0x38, 0x2D, 0x23, 0x18,
    0x0D, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x13, 0x1D, 0x28,
    0x32, 0x3C, 0x47, 0x51, 0x5B, 0x64, 0x6E, 0x77, 0x80, 0x89, 0x92, 0x9A,
    0xA2, 0xAA, 0xB1, 0xB8, 0xBE, 0xC4, 0xC9, 0xCD, 0xD1, 0xD4, 0xD6, 0xD7,
    0xD8, 0xD7, 0xD6, 0xD4, 0xD1, 0xCD, 0xC9, 0xC4, 0xBE, 0xB8, 0xB1, 0xAA,
    0xA2, 0x9A, 0x92, 0x89, 0x80, 0x77, 0x6E, 0x64, 0x5B, 0x51, 0x47, 0x3C,
    0x32, 0x28, 0x1D, 0x13, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x02, 0x0D, 0x17, 0x22, 0x2C, 0x36, 0x40, 0x4A, 0x54, 0x5D, 0x66, 0x6F,
    0x78, 0x81, 0x89, 0x91, 0x99, 0xA0, 0xA7, 0xAE, 0xB4, 0xB9, 0xBE, 0xC2,
    0xC5, 0xC8, 0xCA, 0xCB, 0xCC, 0xCB, 0xCA, 0xC8, 0xC5, 0xC2, 0xBE, 0xB9,
    0xB4, 0xAE, 0xA7, 0xA0, 0x99, 0x91, 0x89, 0x81, 0x78, 0x6F, 0x66, 0x5D,
    0x54, 0x4A, 0x40, 0x36, 0x2C, 0x22, 0x17, 0x0D, 0x02, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0x11, 0x1B, 0x25, 0x2F, 0x39, 0x43,
    0x4C, 0x55, 0x5F, 0x67, 0x70, 0x78, 0x80, 0x88, 0x90, 0x97, 0x9D, 0xA3,
    0xA9, 0xAE, 0xB2, 0xB6, 0xBA, 0xBC, 0xBE, 0xBF, 0xC0, 0xBF, 0xBE, 0xBC,
    0xBA, 0xB6, 0xB2, 0xAE, 0xA9, 0xA3, 0x9D, 0x97, 0x90, 0x88, 0x80, 0x78,
    0x70, 0x67, 0x5F, 0x55, 0x4C, 0x43, 0x39, 0x2F, 0x25, 0x1B, 0x11, 0x07,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x0B, 0x15,
    0x1F, 0x28, 0x32, 0x3B, 0x45, 0x4E, 0x56, 0x5F, 0x67, 0x6F, 0x77, 0x7F,
    0x86, 0x8D, 0x93, 0x99, 0x9E, 0xA3, 0xA7, 0xAB, 0xAE, 0xB0, 0xB2, 0xB3,
    0xB4, 0xB3, 0xB2, 0xB0, 0xAE, 0xAB, 0xA7, 0xA3, 0x9E, 0x99, 0x93, 0x8D,
    0x86, 0x7F, 0x77, 0x6F, 0x67, 0x5F, 0x56, 0x4E, 0x45, 0x3B, 0x32, 0x28,
    0x1F, 0x15, 0x0B, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x04, 0x0E, 0x18, 0x21, 0x2A, 0x34, 0x3D, 0x45, 0x4E, 0x56,
    0x5F, 0x66, 0x6E, 0x75, 0x7C, 0x82, 0x88, 0x8E, 0x93, 0x98, 0x9C, 0x9F,
    0xA2, 0xA5, 0xA6, 0xA7, 0xA8, 0xA7, 0xA6, 0xA5, 0xA2, 0x9F, 0x9C, 0x98,
    0x93, 0x8E, 0x88, 0x82, 0x7C, 0x75, 0x6E, 0x66, 0x5F, 0x56, 0x4E, 0x45,
    0x3D, 0x34, 0x2A, 0x21, 0x18, 0x0E, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0x10, 0x1A, 0x23, 0x2C,
    0x34, 0x3D, 0x45, 0x4E, 0x55, 0x5D, 0x64, 0x6B, 0x72, 0x78, 0x7E, 0x83,
    0x88, 0x8D, 0x90, 0x94, 0x97, 0x99, 0x9A, 0x9B, 0x9C, 0x9B, 0x9A, 0x99,
    0x97, 0x94, 0x90, 0x8D, 0x88, 0x83, 0x7E, 0x78, 0x72, 0x6B, 0x64, 0x5D,
    0x55, 0x4E, 0x45, 0x3D, 0x34, 0x2C, 0x23, 0x1A, 0x10, 0x07, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x09, 0x12, 0x1B, 0x24, 0x2C, 0x34, 0x3D, 0x45, 0x4C, 0x54, 0x5B, 0x61,
    0x68, 0x6E, 0x73, 0x78, 0x7D, 0x81, 0x85, 0x88, 0x8B, 0x8D, 0x8E, 0x8F,
    0x90, 0x8F, 0x8E, 0x8D, 0x8B, 0x88, 0x85, 0x81, 0x7D, 0x78, 0x73, 0x6E,
    0x68, 0x61, 0x5B, 0x54, 0x4C, 0x45, 0x3D, 0x34, 0x2C, 0x24, 0x1B, 0x12,
    0x09, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x01, 0x0A, 0x13, 0x1B, 0x24, 0x2C, 0x34, 0x3B,
    0x43, 0x4A, 0x51, 0x57, 0x5D, 0x63, 0x68, 0x6D, 0x72, 0x76, 0x79, 0x7C,
    0x7F, 0x81, 0x82, 0x83, 0x84, 0x83, 0x82, 0x81, 0x7F, 0x7C, 0x79, 0x76,
    0x72, 0x6D, 0x68, 0x63, 0x5D, 0x57, 0x51, 0x4A, 0x43, 0x3B, 0x34, 0x2C,
    0x24, 0x1B, 0x13, 0x0A, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x0A, 0x13,
    0x1B, 0x23, 0x2A, 0x32, 0x39, 0x40, 0x47, 0x4D, 0x53, 0x58, 0x5E, 0x62,
    0x67, 0x6A, 0x6E, 0x71, 0x73, 0x75, 0x76, 0x77, 0x78, 0x77, 0x76, 0x75,
    0x73, 0x71, 0x6E, 0x6A, 0x67, 0x62, 0x5E, 0x58, 0x53, 0x4D, 0x47, 0x40,
    0x39, 0x32, 0x2A, 0x23, 0x1B, 0x13, 0x0A, 0x02, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x02, 0x0A, 0x12, 0x1A, 0x21, 0x28, 0x2F, 0x36, 0x3C, 0x42,
    0x48, 0x4E, 0x53, 0x57, 0x5B, 0x5F, 0x62, 0x65, 0x67, 0x69, 0x6A, 0x6B,
    0x6C, 0x6B, 0x6A, 0x69, 0x67, 0x65, 0x62, 0x5F, 0x5B, 0x57, 0x53, 0x4E,
    0x48, 0x42, 0x3C, 0x36, 0x2F, 0x28, 0x21, 0x1A, 0x12, 0x0A, 0x02, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x09, 0x10, 0x18, 0x1F,
    0x25, 0x2C, 0x32, 0x38, 0x3E, 0x43, 0x48, 0x4C, 0x50, 0x54, 0x57, 0x59,
    0x5C, 0x5D, 0x5F, 0x5F, 0x60, 0x5F, 0x5F, 0x5D, 0x5C, 0x59, 0x57, 0x54,
    0x50, 0x4C, 0x48, 0x43, 0x3E, 0x38, 0x32, 0x2C, 0x25, 0x1F, 0x18, 0x10,
    0x09, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x07, 0x0E, 0x15, 0x1B, 0x22, 0x28, 0x2D, 0x33, 0x38, 0x3C, 0x41,
    0x45, 0x48, 0x4B, 0x4E, 0x50, 0x51, 0x53, 0x53, 0x54, 0x53, 0x53, 0x51,
    0x50, 0x4E, 0x4B, 0x48, 0x45, 0x41, 0x3C, 0x38, 0x33, 0x2D, 0x28, 0x22,
    0x1B, 0x15, 0x0E, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x0B, 0x11, 0x17, 0x1D, 0x23,
    0x28, 0x2D, 0x31, 0x35, 0x39, 0x3C, 0x3F, 0x42, 0x44, 0x45, 0x47, 0x47,
    0x48, 0x47, 0x47, 0x45, 0x44, 0x42, 0x3F, 0x3C, 0x39, 0x35, 0x31, 0x2D,
    0x28, 0x23, 0x1D, 0x17, 0x11, 0x0B, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
    0x07, 0x0D, 0x13, 0x18, 0x1D, 0x22, 0x26, 0x2A, 0x2E, 0x31, 0x34, 0x36,
    0x38, 0x3A, 0x3B, 0x3B, 0x3C, 0x3B, 0x3B, 0x3A, 0x38, 0x36, 0x34, 0x31,
    0x2E, 0x2A, 0x26, 0x22, 0x1D, 0x18, 0x13, 0x0D, 0x07, 0x01, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x08, 0x0D, 0x12, 0x17, 0x1B, 0x1F,
    0x22, 0x25, 0x28, 0x2A, 0x2C, 0x2E, 0x2F, 0x2F, 0x30, 0x2F, 0x2F, 0x2E,
    0x2C, 0x2A, 0x28, 0x25, 0x22, 0x1F, 0x1B, 0x17, 0x12, 0x0D, 0x08, 0x02,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02,
    0x07, 0x0B, 0x0F, 0x13, 0x17, 0x1A, 0x1C, 0x1E, 0x20, 0x22, 0x23, 0x23,
    0x24, 0x23, 0x23, 0x22, 0x20, 0x1E, 0x1C, 0x1A, 0x17, 0x13, 0x0F, 0x0B,
    0x07, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x08, 0x0B, 0x0E, 0x10, 0x13,
    0x14, 0x16, 0x17, 0x17, 0x18, 0x17, 0x17, 0x16, 0x14, 0x13, 0x10, 0x0E,
    0x0B, 0x08, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x02, 0x05, 0x07, 0x08, 0x0A, 0x0B, 0x0B, 0x0C, 0x0B, 0x0B, 0x0A,
    0x08, 0x07, 0x05, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
};

// Coordinates are the center of the mask
static void Simulation_ServicesApplyMaskBig(int x, int y)
{
    int sx = x - SERVICES_MASK_BIG_CENTER_X;
    int sy = y - SERVICES_MASK_BIG_CENTER_Y;

    for (int j = 0; j < SERVICES_MASK_BIG_HEIGHT; j++)
    {
        int mapy = j - sy;

        if (mapy < 0)
            continue;

        if (mapy >= CITY_MAP_HEIGHT)
            break;

        for (int i = 0; i < SERVICES_MASK_BIG_WIDTH; i++)
        {
            int mapx = i - sx;

            if (mapx < 0)
                continue;

            if (mapx >= CITY_MAP_WIDTH)
                break;

            int val = SERVICES_INFLUENCE_MASK_BIG[j * SERVICES_MASK_BIG_WIDTH + i];
            int old = services_matrix[mapy * CITY_MAP_HEIGHT + mapx];

            val += old;

            if (val > 255)
                services_matrix[mapy * CITY_MAP_HEIGHT + mapx] = 255;
            else
                services_matrix[mapy * CITY_MAP_HEIGHT + mapx] = val;
        }
    }
}

// Central tile of the building (tileset_info.h)
void Simulation_ServicesBig(uint16_t source_tile)
{
    memset(services_matrix, 0, sizeof(services_matrix));

    for (int j = 0; j < CITY_MAP_HEIGHT; j++)
    {
        for (int i = 0; i < CITY_MAP_WIDTH; i++)
        {
            if (CityMapGetTile(i, j) == source_tile)
            {
                // TODO: If no power, ignore this building
                // TILE_OK_POWER_BIT
                Simulation_ServicesApplyMaskBig(i, j);
            }
        }
    }
}
