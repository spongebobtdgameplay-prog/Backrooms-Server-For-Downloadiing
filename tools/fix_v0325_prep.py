from pathlib import Path

Root = Path('.')

WorldTypesPath = Root / 'src/World/WorldTypes.h'
WorldTypes = WorldTypesPath.read_text(encoding='utf-8')
WorldTypes = WorldTypes.replace('#include "../Physics/AABB.h"\n\n', '')

AabbDefinition = '''struct AABB\n{\n    glm::vec3 Min{0.0f};\n    glm::vec3 Max{0.0f};\n};\n\n'''

Marker = 'enum class Direction\n'
if AabbDefinition not in WorldTypes:
    if Marker not in WorldTypes:
        raise RuntimeError('WorldTypes.h direction marker not found')
    WorldTypes = WorldTypes.replace(Marker, AabbDefinition + Marker, 1)

WorldTypesPath.write_text(WorldTypes, encoding='utf-8', newline='\n')

CMakePath = Root / 'CMakeLists.txt'
CMake = CMakePath.read_text(encoding='utf-8')
if 'project(BackroomsOffical VERSION 0.3.24 LANGUAGES C CXX)' not in CMake:
    raise RuntimeError('CMake project version marker not found')
CMake = CMake.replace(
    'project(BackroomsOffical VERSION 0.3.24 LANGUAGES C CXX)',
    'project(BackroomsOffical VERSION 0.3.25 LANGUAGES C CXX)',
    1
)
CMakePath.write_text(CMake, encoding='utf-8', newline='\n')

if '#include "../Physics/AABB.h"' in WorldTypesPath.read_text(encoding='utf-8'):
    raise RuntimeError('stale nonexistent AABB include remains')

if 'struct AABB' not in WorldTypesPath.read_text(encoding='utf-8'):
    raise RuntimeError('AABB definition missing after prep fix')

if 'VERSION 0.3.25' not in CMakePath.read_text(encoding='utf-8'):
    raise RuntimeError('CMake version was not updated')

print('V0.3.25 prep corrections applied successfully')
