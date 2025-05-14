#pragma once

#include "PlayerStrategy.hpp"
#include <memory>
#include <string>
#include <stdexcept>
#include <iostream>
#include <windows.h>

namespace sevens {

/**
 * Utility class for loading player strategies from shared libraries.
 */
class StrategyLoader {
public:
    static std::shared_ptr<PlayerStrategy> loadFromLibrary(const std::string& libraryPath) {
        // 尝试加载DLL
        HMODULE hDll = LoadLibraryA(libraryPath.c_str());
        if (!hDll) {
            DWORD error = GetLastError();
            std::string errorMsg = "Failed to load library: " + libraryPath + 
                                 "\nError code: " + std::to_string(error);
            
            // 获取更详细的错误信息
            char* messageBuffer = nullptr;
            size_t size = FormatMessageA(
                FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                NULL,
                error,
                MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                (LPSTR)&messageBuffer,
                0,
                NULL
            );
            
            if (messageBuffer) {
                errorMsg += "\nError message: ";
                errorMsg += std::string(messageBuffer, size);
                LocalFree(messageBuffer);
            }
            
            throw std::runtime_error(errorMsg);
        }

        try {
            // 获取创建策略的函数指针
            using CreateStrategyFunc = PlayerStrategy* (*)();
            CreateStrategyFunc createStrategy = 
                reinterpret_cast<CreateStrategyFunc>(
                    GetProcAddress(hDll, "createStrategy")
                );
            
            if (!createStrategy) {
                DWORD error = GetLastError();
                FreeLibrary(hDll);
                throw std::runtime_error(
                    "Failed to find createStrategy function in: " + libraryPath +
                    "\nError code: " + std::to_string(error)
                );
            }

            // 创建策略实例
            PlayerStrategy* strategy = createStrategy();
            if (!strategy) {
                FreeLibrary(hDll);
                throw std::runtime_error(
                    "Strategy creation returned nullptr from: " + libraryPath
                );
            }

            // 使用自定义删除器的shared_ptr，确保正确清理资源
            return std::shared_ptr<PlayerStrategy>(
                strategy,
                [hDll](PlayerStrategy* p) {
                    if (p) {
                        delete p;
                    }
                    FreeLibrary(hDll);
                }
            );
        }
        catch (const std::exception& e) {
            FreeLibrary(hDll);
            throw std::runtime_error(
                std::string("Error while loading strategy from ") + 
                libraryPath + ": " + e.what()
            );
        }
    }

    // 检查文件是否是有效的DLL
    static bool isValidLibrary(const std::string& libraryPath) {
        HMODULE hDll = LoadLibraryA(libraryPath.c_str());
        if (!hDll) {
            return false;
        }

        // 检查是否包含必要的函数
        auto createStrategy = GetProcAddress(hDll, "createStrategy");
        FreeLibrary(hDll);
        
        return createStrategy != nullptr;
    }

    // 获取库的错误信息
    static std::string getLastErrorMessage() {
        DWORD error = GetLastError();
        char* messageBuffer = nullptr;
        
        size_t size = FormatMessageA(
            FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
            NULL,
            error,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            (LPSTR)&messageBuffer,
            0,
            NULL
        );
        
        std::string message;
        if (messageBuffer) {
            message = std::string(messageBuffer, size);
            LocalFree(messageBuffer);
        }
        
        return message;
    }
};

} // namespace sevens
