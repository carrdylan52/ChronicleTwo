#pragma once

// The memory-card save file and photo album in their PS2 byte layout, and their conversion to and
// from the host's CSaveData and CDC2AlbumData. On the host each photo's image pointer is 8 bytes
// and costume_bit is the size of long, so the game records are not the card images.

#include <cstddef>
#include <cstdint>
#include <cstring>

#include "inventmn.hpp"
#include "memcard.hpp"
#include "savedata.hpp"
#include "userdata.hpp"

namespace port::records {

inline constexpr std::size_t kSaveFileBytes = 0x659C0;  // SAVEDATA_FORMAT
inline constexpr std::size_t kSaveHeaderBytes = 0x80;   // SAVEDATA_FORMAT up to save_data
inline constexpr std::size_t kSaveDataBytes = 0x65930;  // CSaveData
inline constexpr std::size_t kAlbumBytes = 0x64CB0;     // CDC2AlbumData
inline constexpr std::size_t kPictureBytes = 0x18;      // USER_PICTURE_INFO
inline constexpr std::size_t kPictureImage = 0x14;      // USER_PICTURE_INFO::image
inline constexpr std::size_t kPhotoWorkBytes = 0x2000;  // one photo's pixels

// Offsets inside CSaveData on the card.
inline constexpr std::size_t kDiskPhotos = 0x1D2A0 + 0x7F30 + 0x408;      // user_data.invent_data.photo
inline constexpr std::size_t kDiskAfterPhotos = kDiskPhotos + 30 * kPictureBytes;
inline constexpr std::size_t kDiskPhotoWork = 0x1D2A0 + 0x7F30 + 0xD60;   // user_data.invent_data.photo_work
inline constexpr std::size_t kDiskCostumeBit = 0x1D2A0 + 0x45598;         // user_data.costume_bit
inline constexpr std::size_t kDiskAlbumPhotos = INVENT_ALBUM_PHOTO_NUM * kPhotoWorkBytes;

// The PS2 game saves the global SaveData, so its photo pointers on the card are addresses inside it.
inline constexpr uint32_t kPs2SaveData = 0x01E20860;

inline constexpr std::size_t kHostPhotos =
    offsetof(CSaveData, user_data) + offsetof(CUserDataManager, invent_data) + offsetof(CInventUserData, photo);
inline constexpr std::size_t kHostAfterPhotos =
    offsetof(CSaveData, user_data) + offsetof(CUserDataManager, invent_data) + offsetof(CInventUserData, created_item);
inline constexpr std::size_t kHostCostumeBit = offsetof(CSaveData, user_data) + offsetof(CUserDataManager, costume_bit);
inline constexpr std::size_t kHostAfterCostumeBit = kHostCostumeBit + sizeof(CUserDataManager::costume_bit);

// Everything outside the photos and costume_bit has its PS2 layout on the host; the splice
// below relies on that, and these catch a member that changes it.
static_assert(offsetof(SAVEDATA_FORMAT, save_data) == kSaveHeaderBytes);
static_assert(kHostPhotos == kDiskPhotos);
static_assert(kHostCostumeBit - kHostAfterPhotos == kDiskCostumeBit - kDiskAfterPhotos);
static_assert(sizeof(CSaveData) - kHostAfterCostumeBit == kSaveDataBytes - (kDiskCostumeBit + 8));
static_assert(offsetof(CSaveData, user_data) + offsetof(CUserDataManager, invent_data) +
                  offsetof(CInventUserData, photo_work) - kHostAfterPhotos ==
              kDiskPhotoWork - kDiskAfterPhotos);
static_assert(sizeof(SAVEDATA_FORMAT) - offsetof(SAVEDATA_FORMAT, unk_659B0) == kSaveFileBytes - 0x659B0);
static_assert(offsetof(USER_PICTURE_INFO, unk_c) + sizeof(USER_PICTURE_INFO::unk_c) == kPictureImage);
static_assert(offsetof(CDC2AlbumData, photo) == kDiskAlbumPhotos);
static_assert(sizeof(CDC2AlbumData) - offsetof(CDC2AlbumData, unk_644b0) ==
              kAlbumBytes - kDiskAlbumPhotos - INVENT_ALBUM_PHOTO_NUM * kPictureBytes);

namespace detail {

inline void PictureToDisk(const USER_PICTURE_INFO &photo, uint32_t image, unsigned char *disk) {
    std::memcpy(disk, &photo, kPictureImage);
    std::memcpy(disk + kPictureImage, &image, sizeof(image));
}

inline uint32_t PictureFromDisk(const unsigned char *disk, USER_PICTURE_INFO &photo) {
    uint32_t image;
    std::memcpy(&photo, disk, kPictureImage);
    std::memcpy(&image, disk + kPictureImage, sizeof(image));
    return image;
}

} // namespace detail

// Writes save as the card's CSaveData bytes (kSaveDataBytes). An image pointer into the save's
// own photo_work becomes the PS2 address of that photo in SaveData; any other becomes 0.
inline void SaveDataToDisk(const CSaveData &save, unsigned char *disk) {
    const unsigned char *host = reinterpret_cast<const unsigned char *>(&save);
    const CInventUserData &invent = save.user_data.invent_data;
    std::memcpy(disk, host, kDiskPhotos);
    for (int i = 0; i < 30; i++) {
        uint32_t image = 0;
        for (int j = 0; j < 30; j++) {
            if (invent.photo[i].image == reinterpret_cast<const char *>(invent.photo_work[j])) {
                image = kPs2SaveData + static_cast<uint32_t>(kDiskPhotoWork + j * kPhotoWorkBytes);
            }
        }
        detail::PictureToDisk(invent.photo[i], image, disk + kDiskPhotos + i * kPictureBytes);
    }
    std::memcpy(disk + kDiskAfterPhotos, host + kHostAfterPhotos, kDiskCostumeBit - kDiskAfterPhotos);
    uint64_t costume_bit = save.user_data.costume_bit;
    std::memcpy(disk + kDiskCostumeBit, &costume_bit, sizeof(costume_bit));
    std::memcpy(disk + kDiskCostumeBit + 8, host + kHostAfterCostumeBit, kSaveDataBytes - (kDiskCostumeBit + 8));
}

// Reads the card's CSaveData bytes into save. An image address inside SaveData's photo_work points
// at the same photo of save; any other becomes null, as the game relinks them (ResetAddress).
inline void SaveDataFromDisk(const unsigned char *disk, CSaveData &save) {
    unsigned char *host = reinterpret_cast<unsigned char *>(&save);
    CInventUserData &invent = save.user_data.invent_data;
    std::memcpy(host, disk, kDiskPhotos);
    for (int i = 0; i < 30; i++) {
        uint32_t image = detail::PictureFromDisk(disk + kDiskPhotos + i * kPictureBytes, invent.photo[i]);
        uint32_t offset = image - kPs2SaveData - static_cast<uint32_t>(kDiskPhotoWork);
        invent.photo[i].image =
            image != 0 && offset < 30 * kPhotoWorkBytes && offset % kPhotoWorkBytes == 0
                ? reinterpret_cast<char *>(invent.photo_work[offset / kPhotoWorkBytes])
                : nullptr;
    }
    std::memcpy(host + kHostAfterPhotos, disk + kDiskAfterPhotos, kDiskCostumeBit - kDiskAfterPhotos);
    uint64_t costume_bit;
    std::memcpy(&costume_bit, disk + kDiskCostumeBit, sizeof(costume_bit));
    save.user_data.costume_bit = static_cast<unsigned long>(costume_bit);
    std::memcpy(host + kHostAfterCostumeBit, disk + kDiskCostumeBit + 8, kSaveDataBytes - (kDiskCostumeBit + 8));
}

// The whole save file (kSaveFileBytes): header, CSaveData, trailer. SAVE_CONVERT_WORK has the
// same shape and converts the same way.
inline void SaveFileToDisk(const SAVEDATA_FORMAT &file, unsigned char *disk) {
    std::memcpy(disk, reinterpret_cast<const unsigned char *>(&file), kSaveHeaderBytes);
    SaveDataToDisk(file.save_data, disk + kSaveHeaderBytes);
    std::memcpy(disk + kSaveHeaderBytes + kSaveDataBytes, file.unk_659B0, sizeof(file.unk_659B0));
}

inline void SaveFileFromDisk(const unsigned char *disk, SAVEDATA_FORMAT &file) {
    std::memcpy(reinterpret_cast<unsigned char *>(&file), disk, kSaveHeaderBytes);
    SaveDataFromDisk(disk + kSaveHeaderBytes, file.save_data);
    std::memcpy(file.unk_659B0, disk + kSaveHeaderBytes + kSaveDataBytes, sizeof(file.unk_659B0));
}

// The album lives on the menu heap, whose PS2 address is not fixed, so its image pointers are
// written as 0 and relinked to the album's own photos on load.
inline void AlbumToDisk(const CDC2AlbumData &album, unsigned char *disk) {
    std::memcpy(disk, album.photo_work, kDiskAlbumPhotos);
    for (int i = 0; i < INVENT_ALBUM_PHOTO_NUM; i++) {
        detail::PictureToDisk(album.photo[i], 0, disk + kDiskAlbumPhotos + i * kPictureBytes);
    }
    std::memcpy(disk + kDiskAlbumPhotos + INVENT_ALBUM_PHOTO_NUM * kPictureBytes, album.unk_644b0,
                sizeof(album.unk_644b0));
}

inline void AlbumFromDisk(const unsigned char *disk, CDC2AlbumData &album) {
    std::memcpy(album.photo_work, disk, kDiskAlbumPhotos);
    for (int i = 0; i < INVENT_ALBUM_PHOTO_NUM; i++) {
        detail::PictureFromDisk(disk + kDiskAlbumPhotos + i * kPictureBytes, album.photo[i]);
        album.photo[i].image = album.photo_work[i];
    }
    std::memcpy(album.unk_644b0, disk + kDiskAlbumPhotos + INVENT_ALBUM_PHOTO_NUM * kPictureBytes,
                sizeof(album.unk_644b0));
}

} // namespace port::records
