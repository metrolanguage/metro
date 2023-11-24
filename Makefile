TARGET		:= 	metro

TOPDIR		?= 	$(CURDIR)
BUILD			:= 	build
INCLUDE		:= 	include
SOURCE		:= 	src

CC			:= gcc
CXX			:= g++

OPTI			?= -O0 -g
COMMONFLAGS		:= $(OPTI) -Wall -Wextra -Wno-switch $(INCLUDES)
CFLAGS			:= $(COMMONFLAGS) -std=c17
CXXFLAGS		:= $(COMMONFLAGS) -std=c++20
LDFLAGS			:=

%.o: %.c
	@echo $(notdir $<)
	@$(CC) -MP -MMD -MF $*.d $(CFLAGS) -c -o $@ $<

%.o: %.cpp
	@echo $(notdir $<)
	@$(CXX) -MP -MMD -MF $*.d $(CXXFLAGS) -c -o $@ $<

ifneq ($(BUILD), $(notdir $(CURDIR)))

CFILES			= $(notdir $(foreach dir,$(SOURCE),$(wildcard $(dir)/*.c)))
CXXFILES		= $(notdir $(foreach dir,$(SOURCE),$(wildcard $(dir)/*.cpp)))

export OUTPUT		= $(TOPDIR)/$(TARGET)
export VPATH		= $(foreach dir,$(SOURCE),$(TOPDIR)/$(dir))
export INCLUDES		= $(foreach dir,$(INCLUDE),-I$(TOPDIR)/$(dir))
export OFILES		= $(CFILES:.c=.o) $(CXXFILES:.cpp=.o)

.PHONY: $(BUILD) all re clean

all: $(BUILD)
	@$(MAKE) --no-print-directory -C $(BUILD) -f $(TOPDIR)/Makefile

release: $(BUILD)
	@$(MAKE) --no-print-directory OPTI="-O3" \
		LDFLAGS="-Wl,--gc-sections,-s" -C $(BUILD) -f $(TOPDIR)/Makefile

$(BUILD):
	@[ -d $@ ] || mkdir -p $@

clean:
	rm -rf $(TARGET) $(BUILD)

re: clean all

else

DEPENDS		:= $(OFILES:.o=.d)

$(OUTPUT): $(OFILES)
	@echo linking...
	@$(CXX) -pthread $(LDFLAGS) -o $@ $^

-include $(DEPENDS)

endif