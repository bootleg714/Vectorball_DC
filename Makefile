.DEFAULT_GOAL := all
TARGET = vectorball.elf
OBJS = src/main.o src/game.o src/vehicle.o src/render.o src/audio.o src/sound_bank.o src/frontend.o src/title_art.o src/match.o
PROJECT_CFLAGS = -std=gnu99 -O2 -Wall -Wextra

ifeq ($(strip $(KOS_BASE)),)
$(error KOS_BASE is not set. Open your DreamSDK/KallistiOS shell first)
endif
include $(KOS_BASE)/Makefile.rules

# kos-cc supplies KOS_CFLAGS and KOS_LDFLAGS itself. Passing those
# variables again duplicates the linker script in DreamSDK.
.PHONY: all clean cdi gdemu gdemu-clean check-disc-tools
all: $(TARGET)
$(TARGET): $(OBJS)
	kos-cc -o $@ $(OBJS) -lm
src/%.o: src/%.c
	kos-cc $(PROJECT_CFLAGS) -c $< -o $@
src/main.o src/render.o: src/render.h src/game.h src/frontend.h
src/main.o src/title_art.o: src/title_art.h
src/frontend.o: src/frontend.h
src/main.o src/render.o src/match.o: src/match.h
src/match.o: src/game.h
src/game.o src/vehicle.o: src/game.h src/vehicle.h
src/main.o src/audio.o: src/audio.h src/game.h
src/audio.o src/sound_bank.o: src/sound_bank.h
# DreamSDK audio/data self-boot workflow from the user's working project.
check-disc-tools:
	@for tool in elf2bin scramble makeip makedisc; do command -v $$tool >/dev/null 2>&1 || { echo "Missing DreamSDK tool: $$tool"; exit 1; }; done
gdemu: $(TARGET) ip.txt | check-disc-tools
	mkdir -p gdemu/01 gdemu/cd_root
	elf2bin $(TARGET)
	test -s vectorball.bin
	scramble vectorball.bin gdemu/cd_root/1ST_READ.BIN
	rm -f gdemu/IP.BIN
	makeip ip.txt gdemu/IP.BIN
	test -s gdemu/IP.BIN
	makedisc gdemu/01/disc.cdi gdemu/cd_root gdemu/IP.BIN VECTORBALL --audio
	@echo "Finished: gdemu/01/disc.cdi"
cdi: gdemu
gdemu-clean:
	-rm -f vectorball.bin gdemu/IP.BIN gdemu/cd_root/1ST_READ.BIN gdemu/01/disc.cdi
clean: gdemu-clean
	-rm -f $(TARGET) $(OBJS) vectorball.cdi
