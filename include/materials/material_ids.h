#ifndef PIXELSIM_MATERIAL_IDS_H
#define PIXELSIM_MATERIAL_IDS_H

typedef enum MaterialID {
    MAT_AIR = 0,
    MAT_SAND,
    MAT_WATER,
    MAT_STONE,
    MAT_FIRE,
    MAT_STEAM,
    MAT_ICE,
    MAT_GLASS,
    MAT_MUD,
    // Add new materials here
    MAT_COUNT
} MaterialID;

#define MAT_NONE MAT_AIR

#endif
