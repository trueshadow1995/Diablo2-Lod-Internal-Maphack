/*
  AreaNames.h -- static area name table for Diablo II: LoD (all 136 areas).

  Indexed by levelNo (0 = unused, 1..136 = real game areas). Sourced from
  the game's Levels.txt. Used by the overlay to label exit markers without
  any game-function call.
*/
#pragma once
#include <cstdint>

static const char* kAreaNames[] =
{
    "",                          //   0  (unused)

    // Act I
    "Rogue Encampment",          //   1
    "Blood Moor",                //   2
    "Cold Plains",               //   3
    "Stony Field",               //   4
    "Dark Wood",                 //   5
    "Black Marsh",               //   6
    "Tamoe Highland",            //   7
    "Den of Evil",               //   8
    "Cave Level 1",              //   9
    "Underground Passage Lv 1",  //  10
    "Hole Level 1",              //  11
    "Pit Level 1",               //  12
    "Cave Level 2",              //  13
    "Underground Passage Lv 2",  //  14
    "Hole Level 2",              //  15
    "Pit Level 2",               //  16
    "Burial Grounds",            //  17
    "Crypt",                     //  18
    "Mausoleum",                 //  19
    "Forgotten Tower",           //  20
    "Tower Cellar Level 1",      //  21
    "Tower Cellar Level 2",      //  22
    "Tower Cellar Level 3",      //  23
    "Tower Cellar Level 4",      //  24
    "Tower Cellar Level 5",      //  25
    "Monastery Gate",            //  26
    "Outer Cloister",            //  27
    "Barracks",                  //  28
    "Jail Level 1",              //  29
    "Jail Level 2",              //  30
    "Jail Level 3",              //  31
    "Inner Cloister",            //  32
    "Cathedral",                 //  33
    "Catacombs Level 1",         //  34
    "Catacombs Level 2",         //  35
    "Catacombs Level 3",         //  36
    "Catacombs Level 4",         //  37
    "Tristram",                  //  38
    "Moo Moo Farm",              //  39

    // Act II
    "Lut Gholein",               //  40
    "Rocky Waste",               //  41
    "Dry Hills",                 //  42
    "Far Oasis",                 //  43
    "Lost City",                 //  44
    "Valley of Snakes",          //  45
    "Canyon of the Magi",        //  46
    "Sewers Level 1",            //  47
    "Sewers Level 2",            //  48
    "Sewers Level 3",            //  49
    "Harem Level 1",             //  50
    "Harem Level 2",             //  51
    "Palace Cellar Level 1",     //  52
    "Palace Cellar Level 2",     //  53
    "Palace Cellar Level 3",     //  54
    "Stony Tomb Level 1",        //  55
    "Halls of the Dead Level 1", //  56
    "Halls of the Dead Level 2", //  57
    "Claw Viper Temple Level 1", //  58
    "Stony Tomb Level 2",        //  59
    "Halls of the Dead Level 3", //  60
    "Claw Viper Temple Level 2", //  61
    "Maggot Lair Level 1",       //  62
    "Maggot Lair Level 2",       //  63
    "Maggot Lair Level 3",       //  64
    "Ancient Tunnels",           //  65
    "Tal Rasha's Tomb 1",        //  66
    "Tal Rasha's Tomb 2",        //  67
    "Tal Rasha's Tomb 3",        //  68
    "Tal Rasha's Tomb 4",        //  69
    "Tal Rasha's Tomb 5",        //  70
    "Tal Rasha's Tomb 6",        //  71
    "Tal Rasha's Tomb 7",        //  72
    "Duriel's Lair",             //  73
    "Arcane Sanctuary",          //  74

    // Act III
    "Kurast Docks",              //  75
    "Spider Forest",             //  76
    "Great Marsh",               //  77
    "Flayer Jungle",             //  78
    "Lower Kurast",              //  79
    "Kurast Bazaar",             //  80
    "Upper Kurast",              //  81
    "Kurast Causeway",           //  82
    "Travincal",                 //  83
    "Spider Cave",               //  84
    "Spider Cavern",             //  85
    "Swampy Pit Level 1",        //  86
    "Swampy Pit Level 2",        //  87
    "Flayer Dungeon Level 1",    //  88
    "Flayer Dungeon Level 2",    //  89
    "Swampy Pit Level 3",        //  90
    "Flayer Dungeon Level 3",    //  91
    "Sewer Level 1",             //  92
    "Sewer Level 2",             //  93
    "Ruined Temple",             //  94
    "Disused Fane",              //  95
    "Forgotten Reliquary",       //  96
    "Forgotten Temple",          //  97
    "Ruined Fane",               //  98
    "Disused Reliquary",         //  99
    "Durance of Hate Level 1",   // 100
    "Durance of Hate Level 2",   // 101
    "Durance of Hate Level 3",   // 102

    // Act IV
    "Pandemonium Fortress",      // 103
    "Outer Steppes",             // 104
    "Plains of Despair",         // 105
    "City of the Damned",        // 106
    "River of Flame",            // 107
    "Chaos Sanctuary",           // 108

    // Act V
    "Harrogath",                 // 109
    "Bloody Foothills",          // 110
    "Frigid Highlands",          // 111
    "Arreat Plateau",            // 112
    "Crystalline Passage",       // 113
    "Frozen River",              // 114
    "Glacial Trail",             // 115
    "Drifter Cavern",            // 116
    "Frozen Tundra",             // 117
    "Ancient's Way",             // 118
    "Icy Cellar",                // 119
    "Arreat Summit",             // 120
    "Nihlathak's Temple",        // 121
    "Halls of Anguish",          // 122
    "Halls of Pain",             // 123
    "Halls of Vaught",           // 124
    "Abaddon",                   // 125
    "Pit of Acheron",            // 126
    "Infernal Pit",              // 127
    "Worldstone Keep Level 1",   // 128
    "Worldstone Keep Level 2",   // 129
    "Worldstone Keep Level 3",   // 130
    "Throne of Destruction",     // 131
    "Worldstone Chamber",        // 132

    // Uber areas (LoD)
    "Matron's Den",              // 133
    "Forgotten Sands",           // 134
    "Furnace of Pain",           // 135
    "Tristram (Uber)",           // 136
};

static const int kAreaCount = sizeof(kAreaNames) / sizeof(kAreaNames[0]);

// Safe lookup: returns "" for out-of-range IDs.
inline const char* AreaName(uint32_t levelNo)
{
    if (levelNo < (uint32_t)kAreaCount)
        return kAreaNames[levelNo];
    return "";
}
