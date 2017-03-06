# SlickEdit generated file.  Do not edit this file except in designated areas.

# Make command to use for dependencies
MAKE=make
RM=rm
MKDIR=mkdir

# -----Begin user-editable area-----

# -----End user-editable area-----

# If no configuration is specified, "Debug" will be used
ifndef CFG
CFG=Debug
endif

#
# Configuration: Debug
#
ifeq "$(CFG)" "Debug"
OUTDIR=Debug
OUTFILE=$(OUTDIR)/zwave-automation-engine
CFG_INC=-I../z-way-devel/include -IlibZaeUtil -IlibZaeService 
CFG_LIB=-l:libreadline.a -l:libncurses.a -lrt 
CFG_OBJ=
COMMON_OBJ=$(OUTDIR)/Conditional.o $(OUTDIR)/Expression.o \
	$(OUTDIR)/List.o $(OUTDIR)/builtin_service.o \
	$(OUTDIR)/builtin_service_manager.o $(OUTDIR)/cli_auth.o \
	$(OUTDIR)/cli_commands.o $(OUTDIR)/cli_logger.o \
	$(OUTDIR)/cli_resolver.o $(OUTDIR)/cli_rest.o $(OUTDIR)/cli_scene.o \
	$(OUTDIR)/cli_sensor.o $(OUTDIR)/cli_service.o $(OUTDIR)/cli_vdev.o \
	$(OUTDIR)/command_class.o $(OUTDIR)/command_parser.o \
	$(OUTDIR)/config.o $(OUTDIR)/data_callbacks.o \
	$(OUTDIR)/device_callbacks.o $(OUTDIR)/http_server.o \
	$(OUTDIR)/logging_modules.o $(OUTDIR)/operator.o \
	$(OUTDIR)/parser_dfa.o $(OUTDIR)/resolver.o $(OUTDIR)/scene.o \
	$(OUTDIR)/scene_action.o $(OUTDIR)/scene_manager.o \
	$(OUTDIR)/script_action_handler.o $(OUTDIR)/service_manager.o \
	$(OUTDIR)/socket_io.o $(OUTDIR)/user_manager.o \
	$(OUTDIR)/vdev_manager.o $(OUTDIR)/vty_io.o \
	$(OUTDIR)/zwave-automation-engine.o 
OBJ=$(COMMON_OBJ) $(CFG_OBJ)
ALL_OBJ=$(OUTDIR)/Conditional.o $(OUTDIR)/Expression.o \
	$(OUTDIR)/List.o $(OUTDIR)/builtin_service.o \
	$(OUTDIR)/builtin_service_manager.o $(OUTDIR)/cli_auth.o \
	$(OUTDIR)/cli_commands.o $(OUTDIR)/cli_logger.o \
	$(OUTDIR)/cli_resolver.o $(OUTDIR)/cli_rest.o $(OUTDIR)/cli_scene.o \
	$(OUTDIR)/cli_sensor.o $(OUTDIR)/cli_service.o $(OUTDIR)/cli_vdev.o \
	$(OUTDIR)/command_class.o $(OUTDIR)/command_parser.o \
	$(OUTDIR)/config.o $(OUTDIR)/data_callbacks.o \
	$(OUTDIR)/device_callbacks.o $(OUTDIR)/http_server.o \
	$(OUTDIR)/logging_modules.o $(OUTDIR)/operator.o \
	$(OUTDIR)/parser_dfa.o $(OUTDIR)/resolver.o $(OUTDIR)/scene.o \
	$(OUTDIR)/scene_action.o $(OUTDIR)/scene_manager.o \
	$(OUTDIR)/script_action_handler.o $(OUTDIR)/service_manager.o \
	$(OUTDIR)/socket_io.o $(OUTDIR)/user_manager.o \
	$(OUTDIR)/vdev_manager.o $(OUTDIR)/vty_io.o \
	$(OUTDIR)/zwave-automation-engine.o -l:libreadline.a -l:libncurses.a \
	-lrt 

COMPILE=/home/alex/pidev/pitools/arm-bcm2708/gcc-linaro-arm-linux-gnueabihf-raspbian-x64/bin/arm-linux-gnueabihf-gcc -c  -x c "-D_GNU_SOURCE" -g -std=c99 -o "$(OUTDIR)/$(*F).o" $(CFG_INC) $<
LINK=/home/alex/pidev/pitools/arm-bcm2708/gcc-linaro-arm-linux-gnueabihf-raspbian-x64/bin/arm-linux-gnueabihf-gcc  -g -LlibZaeUtil/Debug -lZaeUtil -L/home/alex/pidev/pitools/arm-bcm2708/gcc-linaro-arm-linux-gnueabihf-raspbian/arm-linux-gnueabihf/libc/lib/arm-linux-gnueabihf -L../z-way-devel/lib -lzway -lzcommons -lpthread -lxml2 -lz -lm -lcrypto -larchive -llzma -lnettle -lacl -lattr -llzo2 -lbz2 -lcurl -ljson-c -ldl -o "$(OUTFILE)" $(ALL_OBJ)

# Pattern rules
$(OUTDIR)/%.o : %.c
	$(COMPILE)

$(OUTDIR)/%.o : BuiltinServices/%.c
	$(COMPILE)

# Build rules
all: $(OUTFILE)

$(OUTFILE): $(OUTDIR) deps $(OBJ)
	$(LINK)
	-scp $(OUTDIR)/zwave-automation-engine osmc@osmc:/home/osmc/Projects/zwave-automation-engine/

$(OUTDIR):
	$(MKDIR) -p "$(OUTDIR)"

# Build dependencies
deps:
	@(cd libZaeUtil/;$(MAKE) -f Makefile CFG=$(CFG))
	@(cd services/Cron/;$(MAKE) -f Makefile CFG=$(CFG))
	@(cd services/DateTime/;$(MAKE) -f Makefile CFG=$(CFG))
	@(cd services/IFTTT/;$(MAKE) -f Makefile CFG=$(CFG))
	@(cd services/Mail/;$(MAKE) -f Makefile CFG=$(CFG))
	@(cd services/SMS/;$(MAKE) -f Makefile CFG=$(CFG))
	@(cd services/Timer/;$(MAKE) -f Makefile CFG=$(CFG))
	@(cd services/Weather/;$(MAKE) -f Makefile CFG=$(CFG))
	@(cd vdev/SurveillanceStation/;$(MAKE) -f Makefile CFG=$(CFG))

# Rebuild this project
rebuild: cleanall all

# Clean this project
clean:
	$(RM) -f $(OUTFILE)
	$(RM) -f $(OBJ)

# Clean this project and all dependencies
cleanall: clean
	@(cd libZaeUtil/;$(MAKE) -f Makefile cleanall CFG=$(CFG))
	@(cd services/Cron/;$(MAKE) -f Makefile cleanall CFG=$(CFG))
	@(cd services/DateTime/;$(MAKE) -f Makefile cleanall CFG=$(CFG))
	@(cd services/IFTTT/;$(MAKE) -f Makefile cleanall CFG=$(CFG))
	@(cd services/Mail/;$(MAKE) -f Makefile cleanall CFG=$(CFG))
	@(cd services/SMS/;$(MAKE) -f Makefile cleanall CFG=$(CFG))
	@(cd services/Timer/;$(MAKE) -f Makefile cleanall CFG=$(CFG))
	@(cd services/Weather/;$(MAKE) -f Makefile cleanall CFG=$(CFG))
	@(cd vdev/SurveillanceStation/;$(MAKE) -f Makefile cleanall CFG=$(CFG))
endif

#
# Configuration: Release
#
ifeq "$(CFG)" "Release"
OUTDIR=Release
OUTFILE=$(OUTDIR)/zwave-automation-engine
CFG_INC=-I../z-way-devel/include -IlibZaeUtil -IlibZaeService 
CFG_LIB=-l:libreadline.a -l:libncurses.a -lrt 
CFG_OBJ=
COMMON_OBJ=$(OUTDIR)/Conditional.o $(OUTDIR)/Expression.o \
	$(OUTDIR)/List.o $(OUTDIR)/builtin_service.o \
	$(OUTDIR)/builtin_service_manager.o $(OUTDIR)/cli_auth.o \
	$(OUTDIR)/cli_commands.o $(OUTDIR)/cli_logger.o \
	$(OUTDIR)/cli_resolver.o $(OUTDIR)/cli_rest.o $(OUTDIR)/cli_scene.o \
	$(OUTDIR)/cli_sensor.o $(OUTDIR)/cli_service.o $(OUTDIR)/cli_vdev.o \
	$(OUTDIR)/command_class.o $(OUTDIR)/command_parser.o \
	$(OUTDIR)/config.o $(OUTDIR)/data_callbacks.o \
	$(OUTDIR)/device_callbacks.o $(OUTDIR)/http_server.o \
	$(OUTDIR)/logging_modules.o $(OUTDIR)/operator.o \
	$(OUTDIR)/parser_dfa.o $(OUTDIR)/resolver.o $(OUTDIR)/scene.o \
	$(OUTDIR)/scene_action.o $(OUTDIR)/scene_manager.o \
	$(OUTDIR)/script_action_handler.o $(OUTDIR)/service_manager.o \
	$(OUTDIR)/socket_io.o $(OUTDIR)/user_manager.o \
	$(OUTDIR)/vdev_manager.o $(OUTDIR)/vty_io.o \
	$(OUTDIR)/zwave-automation-engine.o 
OBJ=$(COMMON_OBJ) $(CFG_OBJ)
ALL_OBJ=$(OUTDIR)/Conditional.o $(OUTDIR)/Expression.o \
	$(OUTDIR)/List.o $(OUTDIR)/builtin_service.o \
	$(OUTDIR)/builtin_service_manager.o $(OUTDIR)/cli_auth.o \
	$(OUTDIR)/cli_commands.o $(OUTDIR)/cli_logger.o \
	$(OUTDIR)/cli_resolver.o $(OUTDIR)/cli_rest.o $(OUTDIR)/cli_scene.o \
	$(OUTDIR)/cli_sensor.o $(OUTDIR)/cli_service.o $(OUTDIR)/cli_vdev.o \
	$(OUTDIR)/command_class.o $(OUTDIR)/command_parser.o \
	$(OUTDIR)/config.o $(OUTDIR)/data_callbacks.o \
	$(OUTDIR)/device_callbacks.o $(OUTDIR)/http_server.o \
	$(OUTDIR)/logging_modules.o $(OUTDIR)/operator.o \
	$(OUTDIR)/parser_dfa.o $(OUTDIR)/resolver.o $(OUTDIR)/scene.o \
	$(OUTDIR)/scene_action.o $(OUTDIR)/scene_manager.o \
	$(OUTDIR)/script_action_handler.o $(OUTDIR)/service_manager.o \
	$(OUTDIR)/socket_io.o $(OUTDIR)/user_manager.o \
	$(OUTDIR)/vdev_manager.o $(OUTDIR)/vty_io.o \
	$(OUTDIR)/zwave-automation-engine.o -l:libreadline.a -l:libncurses.a \
	-lrt 

COMPILE=/home/alex/pidev/pitools/arm-bcm2708/gcc-linaro-arm-linux-gnueabihf-raspbian-x64/bin/arm-linux-gnueabihf-gcc -c  -x c "-D_GNU_SOURCE" -O2 -std=c99 -o "$(OUTDIR)/$(*F).o" $(CFG_INC) $<
LINK=/home/alex/pidev/pitools/arm-bcm2708/gcc-linaro-arm-linux-gnueabihf-raspbian-x64/bin/arm-linux-gnueabihf-gcc  -O2 -LlibZaeUtil/Release -lZaeUtil -L/home/alex/pidev/pitools/arm-bcm2708/gcc-linaro-arm-linux-gnueabihf-raspbian/arm-linux-gnueabihf/libc/lib/arm-linux-gnueabihf -L../z-way-devel/lib -lzway -lzcommons -lpthread -lxml2 -lz -lm -lcrypto -larchive -llzma -lnettle -lacl -lattr -llzo2 -lbz2 -ljson-c -ldl -o "$(OUTFILE)" $(ALL_OBJ)

# Pattern rules
$(OUTDIR)/%.o : %.c
	$(COMPILE)

$(OUTDIR)/%.o : BuiltinServices/%.c
	$(COMPILE)

# Build rules
all: $(OUTFILE)

$(OUTFILE): $(OUTDIR) deps $(OBJ)
	$(LINK)
	-scp $(OUTDIR)/zwave-automation-engine osmc@osmc:/home/osmc/Projects/zwave-automation-engine/

$(OUTDIR):
	$(MKDIR) -p "$(OUTDIR)"

# Build dependencies
deps:
	@(cd libZaeUtil/;$(MAKE) -f Makefile CFG=$(CFG))
	@(cd services/Cron/;$(MAKE) -f Makefile CFG=$(CFG))
	@(cd services/DateTime/;$(MAKE) -f Makefile CFG=$(CFG))
	@(cd services/IFTTT/;$(MAKE) -f Makefile CFG=$(CFG))
	@(cd services/Mail/;$(MAKE) -f Makefile CFG=$(CFG))
	@(cd services/SMS/;$(MAKE) -f Makefile CFG=$(CFG))
	@(cd services/Timer/;$(MAKE) -f Makefile CFG=$(CFG))
	@(cd services/Weather/;$(MAKE) -f Makefile CFG=$(CFG))
	@(cd vdev/SurveillanceStation/;$(MAKE) -f Makefile CFG=$(CFG))

# Rebuild this project
rebuild: cleanall all

# Clean this project
clean:
	$(RM) -f $(OUTFILE)
	$(RM) -f $(OBJ)

# Clean this project and all dependencies
cleanall: clean
	@(cd libZaeUtil/;$(MAKE) -f Makefile cleanall CFG=$(CFG))
	@(cd services/Cron/;$(MAKE) -f Makefile cleanall CFG=$(CFG))
	@(cd services/DateTime/;$(MAKE) -f Makefile cleanall CFG=$(CFG))
	@(cd services/IFTTT/;$(MAKE) -f Makefile cleanall CFG=$(CFG))
	@(cd services/Mail/;$(MAKE) -f Makefile cleanall CFG=$(CFG))
	@(cd services/SMS/;$(MAKE) -f Makefile cleanall CFG=$(CFG))
	@(cd services/Timer/;$(MAKE) -f Makefile cleanall CFG=$(CFG))
	@(cd services/Weather/;$(MAKE) -f Makefile cleanall CFG=$(CFG))
	@(cd vdev/SurveillanceStation/;$(MAKE) -f Makefile cleanall CFG=$(CFG))
endif
