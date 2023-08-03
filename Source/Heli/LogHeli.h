#pragma once

DECLARE_LOG_CATEGORY_EXTERN(LogHeli, Log, Warning);

#define HELI_LOG(x, ...) UE_LOG(LogHeli, Log, TEXT("%s: " x), UE_SOURCE_LOCATION, __VA_ARGS__)
#define HELI_WRN(x, ...) UE_LOG(LogHeli, Warning, TEXT("%s: " x), UE_SOURCE_LOCATION, __VA_ARGS__)
#define HELI_ERR(x, ...) UE_LOG(LogHeli, Error, TEXT("%s: " x), UE_SOURCE_LOCATION, __VA_ARGS__)
#define HELI_FTL(x, ...) UE_LOG(LogHeli, Fatal, TEXT("%s: " x), UE_SOURCE_LOCATION, __VA_ARGS__)