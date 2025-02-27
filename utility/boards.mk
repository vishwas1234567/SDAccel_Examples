
# Default C++ Compiler Flags and xocc compiler flags
CXXFLAGS:=-O0 -g
CLFLAGS:=-g --xp "param:compiler.preserveHlsOutput=1" --xp "param:compiler.generateExtraRunData=true" -s

# Use the Xilinx OpenCL compiler
CLC:=$(XILINX_SDACCEL)/bin/xocc

# By default build for X86, this could also be set to POWER to build for power
ARCH:=X86

ifeq ($(ARCH),POWER)
DEVICES:= xilinx:adm-pcie-7v3:1ddr-ppc64le:2.1
CXX:=$(XILINX_SDACCEL)/gnu/ppc64le/4.9.3/lnx64/bin/powerpc64le-linux-gnu-g++
else
DEVICES:= xilinx:adm-pcie-ku3:2ddr:3.1
CXX:=$(XILINX_SDACCEL)/lnx64/tools/gcc/bin/g++
endif

# By default build for hardware can be set to
#   hw_emu for hardware emulation
#   sw_emu for software emulation
#   or a collection of all or none of these
TARGETS:=hw

