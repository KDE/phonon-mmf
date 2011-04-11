project(phonon-mmf)

cmake_minimum_required(VERSION 2.6.2 FATAL_ERROR)

find_package(Phonon REQUIRED)

set(PHONON_MMF_LIB_MAJOR_VERSION "4")
set(PHONON_MMF_LIB_MINOR_VERSION "4")
set(PHONON_MMF_LIB_PATCH_VERSION "3")
set(PHONON_MMF_LIB_VERSION "${PHONON_MMF_LIB_MAJOR_VERSION}.${PHONON_MMF_LIB_MINOR_VERSION}.${PHONON_MMF_LIB_PATCH_VERSION}")

add_definitions(${QT_DEFINITIONS})
include_directories(${PHONON_INCLUDES} ${QT_INCLUDES})

# FIXME: mmf/ needs a CMakeLists.txt
# add_subdirectory(mmf)

macro_display_feature_log()
