CC = g++
CPPFLAGS += -g -Wall -I../


all : Date.o Entry.o FileEntry.o VolumeEntry.o

Date.o : Date.cpp Date.h

Entry.o : Entry.cpp File.h

FileEntry.o : FileEntry.cpp File.h Date.h

VolumeEntry.o : VolumeEntry.cpp File.h Date.h

