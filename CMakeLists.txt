cmake_minimum_required(VERSION 3.16)
project(ubdmr C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

find_package(Qt5 COMPONENTS Core Gui Qml Quick Network Multimedia REQUIRED)

# Translations: i18n/ubdmr_<lang>.ts -> .qm, embedded as :/i18n/*.qm.
# LanguageManager offers every embedded language and loads the chosen one.
find_package(Qt5LinguistTools REQUIRED)
file(GLOB UBDMR_TS_FILES ${CMAKE_CURRENT_SOURCE_DIR}/i18n/ubdmr_*.ts)
qt5_add_translation(UBDMR_QM_FILES ${UBDMR_TS_FILES})
set(UBDMR_I18N_QRC ${CMAKE_CURRENT_BINARY_DIR}/i18n.qrc)
set(_i18n_entries "")
foreach(_qm ${UBDMR_QM_FILES})
    get_filename_component(_qm_name ${_qm} NAME)
    string(APPEND _i18n_entries "    <file alias=\"i18n/${_qm_name}\">${_qm}</file>\n")
endforeach()
file(WRITE ${UBDMR_I18N_QRC}
     "<RCC>\n  <qresource prefix=\"/\">\n${_i18n_entries}  </qresource>\n</RCC>\n")
# The .qrc references generated .qm files, so they must exist before rcc runs.
add_custom_target(ubdmr_translations DEPENDS ${UBDMR_QM_FILES})

# Built-in AMBE+2 codec, kept identical to upstream (see its README), so its
# warnings are not ours to fix and are silenced.
file(GLOB OPENDMR_SOURCES
    third_party/opendmr/decoder/*.c
    third_party/opendmr/encoder/*.cc
    third_party/opendmr/encoder/*.cpp)
add_library(opendmr STATIC ${OPENDMR_SOURCES})
target_include_directories(opendmr PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/opendmr/decoder
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/opendmr/encoder)
target_compile_options(opendmr PRIVATE -w -O2)
target_link_libraries(opendmr PUBLIC m)

add_executable(ubdmr
    src/main.cpp
    src/ambeframe.cpp
    src/ambeframe.h
    src/ambeservervocoder.cpp
    src/ambeservervocoder.h
    src/audioplayer.cpp
    src/audioplayer.h
    src/audiorecorder.cpp
    src/audiorecorder.h
    src/fulllc.cpp
    src/fulllc.h
    src/idlookup.cpp
    src/idlookup.h
    src/languagemanager.cpp
    src/languagemanager.h
    src/lastheardmodel.cpp
    src/lastheardmodel.h
    src/radiocontroller.cpp
    src/radiocontroller_tx.cpp
    src/radiocontroller.h
    src/rewindclient.cpp
    src/rewindclient_text.cpp
    src/rewindclient.h
    src/softwarevocoder.cpp
    src/softwarevocoder.h
    src/systemtheme.cpp
    src/systemtheme.h
    src/vocoder.h
    src/voiceformat.h
    qml.qrc
    ${UBDMR_I18N_QRC})
add_dependencies(ubdmr ubdmr_translations)

target_include_directories(ubdmr PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_compile_options(ubdmr PRIVATE -Wall -Wextra)

target_link_libraries(ubdmr
    Qt5::Core Qt5::Gui Qt5::Qml Qt5::Quick Qt5::Network Qt5::Multimedia opendmr)

# QML is compiled into the binary via qml.qrc and not installed separately.
install(TARGETS ubdmr RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX})
install(FILES manifest.json ubdmr.desktop ubdmr.apparmor ubdmr.png
        DESTINATION ${CMAKE_INSTALL_PREFIX})
