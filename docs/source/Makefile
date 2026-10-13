# ------------------------------------------------
# @author Adrian Szczepanski
# @date 13-05-2022
# ------------------------------------------------

target ?= manual
main_file ?= main

project_dir := ..
source_dir := .
build_dir := $(project_dir)/build

roff_sources := $(wildcard $(source_dir)/*.roff)

#target list
$(project_dir)/$(target).pdf: $(roff_sources) | $(build_dir)
	@echo Compiling document $@
	@soelim $(source_dir)/$(main_file).roff | preconv -Dutf8 | refer | groff -Kutf8 -p -e -t -mec -ms -Tdvi > $(build_dir)/$(target).dvi
	@dvipdfm -cz 9 $(build_dir)/$(target).dvi
	@mv $(target).pdf $(project_dir)

$(project_dir)/$(target).txt: $(roff_sources) | $(build_dir)
	@echo Compiling document $@
	@soelim  $(source_dir)/$(main_file).roff | preconv -Dutf8 | refer | groff -Kutf8 -Tutf8 -p -t -e -ms > $@
	@mv $(target).txt $(project_dir)

# make directories
$(build_dir) :
	mkdir $@
