# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "D:/Sotatek/espDev/Espressif/frameworks/esp-idf-v5.0.2/components/bootloader/subproject"
  "D:/Sotatek/sample_project/build/bootloader"
  "D:/Sotatek/sample_project/build/bootloader-prefix"
  "D:/Sotatek/sample_project/build/bootloader-prefix/tmp"
  "D:/Sotatek/sample_project/build/bootloader-prefix/src/bootloader-stamp"
  "D:/Sotatek/sample_project/build/bootloader-prefix/src"
  "D:/Sotatek/sample_project/build/bootloader-prefix/src/bootloader-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "D:/Sotatek/sample_project/build/bootloader-prefix/src/bootloader-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "D:/Sotatek/sample_project/build/bootloader-prefix/src/bootloader-stamp${cfgdir}") # cfgdir has leading slash
endif()
