#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include "stb_image_resize2.h"

#include <vector>
#include <string>
#include <cstdint>
#include <algorithm> // Required for std::clamp
#include "Carbox.h"
#include "FileUtils.h"
#include "Logger.h"

namespace Randomizer {

void GenerateAndSaveCarboxAtlas(const std::string& outputPath, const std::vector<CarboxSource>& cars) {
    const int atlasSize = 1024;
    const int cellSizeCustom = 340; 
    const int gapCustom = 2;         
    
    const int cellSizeVanilla = 84;  
    const int gapVanilla = 1;         
    const int channels = 4;
    
    // Initialize a transparent black canvas
    std::vector<uint8_t> atlas(atlasSize * atlasSize * channels, 0);

    for (size_t i = 0; i < 9 && i < cars.size(); ++i) {
        const auto& car = cars[i];
        if (car.filepath.empty()) continue;

        int srcW, srcH, srcChannels;
        std::string absoluteImgPath = GetAbsoluteFilePath(car.filepath);

        uint8_t* img = stbi_load(absoluteImgPath.c_str(), &srcW, &srcH, &srcChannels, channels);
        
        if (!img) {
            Logger::TimestampLogf("[GenerateAndSaveCarboxAtlas] ERROR: stbi_load failed to find or parse carbox image at: %s", absoluteImgPath.c_str());
            continue;
        }

        // This will hold our final 340x340 image for this cell, regardless of the source
        std::vector<uint8_t> resizedImg(cellSizeCustom * cellSizeCustom * channels);

        if (car.isFromGrid) {
            // --- 1. Extract 84x84 Vanilla Cell ---
            std::vector<uint8_t> vanillaCell(cellSizeVanilla * cellSizeVanilla * channels);
            int srcStartX = (car.sourceGridX * (cellSizeVanilla + gapVanilla)) + gapVanilla;
            int srcStartY = (car.sourceGridY * (cellSizeVanilla + gapVanilla)) + gapVanilla;

            for (int y = 0; y < cellSizeVanilla; ++y) {
                for (int x = 0; x < cellSizeVanilla; ++x) {
                    int clampX = std::clamp(srcStartX + x, 0, srcW - 1);
                    int clampY = std::clamp(srcStartY + y, 0, srcH - 1);
                    
                    int destIdx = (y * cellSizeVanilla + x) * channels;
                    int srcIdx = (clampY * srcW + clampX) * channels;
                    
                    for (int c = 0; c < channels; ++c) {
                        vanillaCell[destIdx + c] = img[srcIdx + c];
                    }
                }
            }

            // --- 2. Upscale Vanilla Cell to 340x340 ---
            stbir_resize_uint8_srgb(
                vanillaCell.data(), cellSizeVanilla, cellSizeVanilla, 0,
                resizedImg.data(), cellSizeCustom, cellSizeCustom, 0,
                STBIR_RGBA
            );
        } 
        else {
            // --- 1. Scale Custom Carbox directly to 340x340 ---
            stbir_resize_uint8_srgb(
                img, srcW, srcH, 0,                             
                resizedImg.data(), cellSizeCustom, cellSizeCustom, 0,       
                STBIR_RGBA                                      
            );
        }

        // --- 3. Unified Write Logic (Write the 340x340 block to the atlas) ---
        int destGridX = i % 3;
        int destGridY = i / 3;
        int destStartX = destGridX * (cellSizeCustom + gapCustom);
        int destStartY = destGridY * (cellSizeCustom + gapCustom);

        // Loop from -1 to cellSizeCustom to write a 1px padding, clamping source coordinates
        for (int dy = -1; dy <= cellSizeCustom; ++dy) {
            for (int dx = -1; dx <= cellSizeCustom; ++dx) {
                int destX = destStartX + dx;
                int destY = destStartY + dy;

                if (destX < 0 || destX >= atlasSize || destY < 0 || destY >= atlasSize) continue;

                // Clamp to 0-339 so the edge pixels stretch outward
                int srcXClamp = std::clamp(dx, 0, cellSizeCustom - 1);
                int srcYClamp = std::clamp(dy, 0, cellSizeCustom - 1);

                int destIdx = (destY * atlasSize + destX) * channels;
                int srcIdx = (srcYClamp * cellSizeCustom + srcXClamp) * channels;

                for (int c = 0; c < channels; ++c) {
                    atlas[destIdx + c] = resizedImg[srcIdx + c];
                }
            }
        }

        stbi_image_free(img);
    }

    stbi_write_bmp(outputPath.c_str(), atlasSize, atlasSize, channels, atlas.data());
}

void GenerateAndSaveSingleCarbox(const std::string& outputPath, const CarboxSource& car) {
    if (car.filepath.empty()) return;

    const int targetSize = 256;
    const int channels = 4;
    
    int srcW, srcH, srcChannels;
    std::string absoluteImgPath = GetAbsoluteFilePath(car.filepath);

    uint8_t* img = stbi_load(absoluteImgPath.c_str(), &srcW, &srcH, &srcChannels, channels);
    
    if (!img) {
        Logger::TimestampLogf("[GenerateAndSaveSingleCarbox] ERROR: stbi_load failed to find or parse carbox image at: %s", absoluteImgPath.c_str());
        return;
    }

    std::vector<uint8_t> resizedImg(targetSize * targetSize * channels, 0);

    if (car.isFromGrid) {
        // --- Extract 84x84 Vanilla Cell ---
        const int cellSizeVanilla = 84;
        const int gapVanilla = 1;

        std::vector<uint8_t> vanillaCell(cellSizeVanilla * cellSizeVanilla * channels);
        int srcStartX = (car.sourceGridX * (cellSizeVanilla + gapVanilla)) + gapVanilla;
        int srcStartY = (car.sourceGridY * (cellSizeVanilla + gapVanilla)) + gapVanilla;

        for (int y = 0; y < cellSizeVanilla; ++y) {
            for (int x = 0; x < cellSizeVanilla; ++x) {
                int clampX = std::clamp(srcStartX + x, 0, srcW - 1);
                int clampY = std::clamp(srcStartY + y, 0, srcH - 1);
                
                int destIdx = (y * cellSizeVanilla + x) * channels;
                int srcIdx = (clampY * srcW + clampX) * channels;
                
                for (int c = 0; c < channels; ++c) {
                    vanillaCell[destIdx + c] = img[srcIdx + c];
                }
            }
        }

        // --- Upscale to 256x256 ---
        stbir_resize_uint8_srgb(
            vanillaCell.data(), cellSizeVanilla, cellSizeVanilla, 0,
            resizedImg.data(), targetSize, targetSize, 0,
            STBIR_RGBA
        );
    } else {
        // Source is already standalone, just ensure it's resized to 256x256 safely
        stbir_resize_uint8_srgb(
            img, srcW, srcH, 0,
            resizedImg.data(), targetSize, targetSize, 0,
            STBIR_RGBA
        );
    }

    stbi_write_bmp(outputPath.c_str(), targetSize, targetSize, channels, resizedImg.data());
    stbi_image_free(img);
}

}