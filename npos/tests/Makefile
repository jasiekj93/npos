# ------------------------------------------------
# @author Adrian Szczepanski
# @date 06-03-2023
# ------------------------------------------------

# Target
tested_library_name := npos

# Directories
project_dir ?= ../..
make_dir := $(project_dir)/make
external_dir := $(project_dir)/external

# Sources 
cxx_sources :=  \
$(wildcard *.cpp) \
$(wildcard nps/*.cpp) \
$(wildcard ipc/*.cpp) \
$(wildcard dev/*.cpp) \
$(wildcard dev/msg/*.cpp) \
$(wildcard dev/service/*.cpp) \
$(wildcard dev/task/*.cpp) \
$(wildcard fs/*.cpp) \
$(wildcard fs/msg/dir/*.cpp) \
$(wildcard fs/msg/file/*.cpp) \
$(wildcard fs/task/*.cpp) \
$(wildcard cli/*.cpp) \
$(wildcard cli/program/*.cpp) \
$(wildcard cli/utils/*.cpp) \
$(wildcard cli/task/*.cpp) \

# Includes
cxx_includes := \
-I. \

# Defines
cxx_defs := \

# Append rest of Makefile from template
include $(make_dir)/SoftwareTestLib.mk