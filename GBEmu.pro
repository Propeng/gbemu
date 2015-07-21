######################################################################
# Automatically generated by qmake (2.01a) Mon Jul 20 22:26:26 2015
######################################################################

TEMPLATE = app
TARGET = 
DEPENDPATH += . \
              emu \
              qt \
              sdl \
              emu\cpu \
              emu\mbc \
              emu\peripheral \
              emu\rom \
              emu\sound \
              emu\video
INCLUDEPATH += .
QT += core gui widgets

# Input
HEADERS += emu/gb.h \
           emu/mem.h \
           qt/AudioBuffer.h \
           qt/EmulatorWidget.h \
           qt/GBWindow.h \
           qt/PrinterPreview.h \
           sdl/audio.h \
           sdl/input.h \
           emu/cpu/cpu.h \
           emu/cpu/instruction.h \
           emu/mbc/mbc.h \
           emu/peripheral/peripheral.h \
           emu/peripheral/printer.h \
           emu/rom/rom.h \
           emu/sound/sound.h \
           emu/video/video.h
SOURCES += emu/gb.c \
           emu/mem.c \
           qt/AudioBuffer.cpp \
           qt/EmulatorWidget.cpp \
           qt/GBWindow.cpp \
           qt/PrinterPreview.cpp \
           qt/qt.cpp \
           sdl/audio.c \
           sdl/input.c \
           sdl/standalone.c \
           emu/cpu/cpu.c \
           emu/cpu/instruction.c \
           emu/mbc/mbc.c \
           emu/mbc/mbc1.c \
           emu/mbc/mbc2.c \
           emu/mbc/mbc3.c \
           emu/mbc/mbc5.c \
           emu/peripheral/peripheral.c \
           emu/peripheral/printer.c \
           emu/rom/rom.c \
           emu/sound/sound.c \
           emu/video/dma.c \
           emu/video/video.c
