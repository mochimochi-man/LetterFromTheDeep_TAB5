#pragma once
#include "math3d.h"
namespace abyss {
enum class MachineKind : uint8_t { ArmoredFish, WingRay, TankFrog };
struct MachinePose { Vec3 position; float yaw,length,phase; MachineKind kind; };
constexpr int CityMachineCount=6;
// They go in the life catalogue like anything else that moves, on save ids of their own
// after the last real animal. Nothing else may take 50 to 52.
constexpr uint8_t MachineFirstId=50;
constexpr int MachineKindCount=3;
const char* machineName(MachineKind kind);
MachinePose cityMachinePose(int index,float time);
}
