#pragma once

#include <cstdint>
#include <cstddef>

#pragma pack(push, 1)

struct RawPlayer
{
    uint32_t SlotID;               
    std::byte _pad1[36];

    uint32_t BipedHandle;          
    std::byte _pad2[12];

    float Position[3];
    float Rotation[3];
    float LookVector[3];

    uint32_t hPrimaryWeapon;       
    uint32_t hSecondaryWeapon;     
    uint32_t hObjective;          
    std::byte _pad3[72];

    wchar_t Name[28];
    std::byte _pad4[12];

    wchar_t Tag[8];
    std::byte _pad5[908];
};

struct RawBiped
{
    uint32_t ClassID;                 
    std::byte _pad1[28];

    float Position[3];
    std::byte _pad2[66];

    // ...
};

struct RawWeapon
{
    uint32_t ClassID;             
    std::byte _pad1[8];

    uint32_t UnknownHandle2;     
    std::byte _pad2[4];

    uint32_t PlayerHandle;          
    std::byte _pad3[8];

    float Position[3];
    std::byte _pad4[400];

    uint32_t WeaponHandle;          
    std::byte _pad5[266];

    short CurrentAmmo;
    std::byte _pad6[484];

    // ...
};

#pragma pack(pop)