# ------------------------------------------------
# @author Adrian Szczepanski
# @date 06-03-2023
# ------------------------------------------------

# Target
tested_library_name := npos-cli

# Directories
project_dir ?= ../..
make_dir := $(project_dir)/make
external_dir := $(project_dir)/external

# Sources 
cxx_sources :=  \
$(wildcard *.cpp) \
$(wildcard */*.cpp) \

# Includes
cxx_includes := \
-I. \
-I$(project_dir)/npos-nps \
-I$(project_dir)/npos-ipc \
-I$(project_dir)/npos-dev \
-I$(project_dir)/npos-fs \

# Defines
cxx_defs := \

# Append rest of Makefile from template
include $(make_dir)/SoftwareTestLib.mk