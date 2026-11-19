# ------------------------------------------------
# @author Adrian Szczepanski
# @date 06-03-2023
# ------------------------------------------------

# Target
tested_library_name := npos-mdlwr

# Directories
project_dir ?= ../..
make_dir := $(project_dir)/make
external_dir := $(project_dir)/external

# Sources 
cxx_sources :=  \
$(wildcard *.cpp) \
$(wildcard cli/program/*.cpp) \
$(wildcard dev/service/*.cpp) \
$(wildcard fs/service/*.cpp) \
$(wildcard fs/service/dir/*.cpp) \
$(wildcard fs/service/file/*.cpp) \

# Includes
cxx_includes := \
-I. \
-I$(project_dir)/npos \
-I$(project_dir)/npos/tests \

# Defines
cxx_defs := \

# Append rest of Makefile from template
include $(make_dir)/SoftwareTestLib.mk