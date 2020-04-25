OUT_DIR := output
Q := @
MINIBSDIFF ?= wine ../../build/Release/bin/minibsdiff.exe

all: all_rtp all_mbsdiff

all_rtp: $(OUT_DIR)/rf120_na.rtp $(OUT_DIR)/rf120_gr.rtp $(OUT_DIR)/rf120_fr.rtp $(OUT_DIR)/patchw32.dll
all_mbsdiff: $(OUT_DIR)/RF-1.21-steam.exe.mbsdiff $(OUT_DIR)/RF-1.21.exe.mbsdiff $(OUT_DIR)/RF-1.20-gr.exe.mbsdiff $(OUT_DIR)/RF-1.20-fr.exe.mbsdiff $(OUT_DIR)/tables-gog-gr.vpp.mbsdiff

clean:
	$(Q)rm -rf $(OUT_DIR)

$(OUT_DIR):
	$(Q)mkdir -p "$(OUT_DIR)"

$(OUT_DIR)/%.rtp : official/%.exe | $(OUT_DIR)
	$(Q)unzip -o -DD "$<" PATCH.RTP -d $(OUT_DIR)
	$(Q)mv "$(OUT_DIR)/PATCH.RTP" "$@"

$(OUT_DIR)/patchw32.dll : official/rf120_na.exe
	$(Q)unzip -o -DD "$<" patchw32.dll -d $(OUT_DIR)

$(OUT_DIR)/%.exe.mbsdiff : source/%.exe target/RF-1.20-na.exe | $(OUT_DIR)
	$(Q)$(MINIBSDIFF) gen "$<" "target/RF-1.20-na.exe" "$@"
	$(Q)sha1sum "$<" > "$(basename $@).sha1"

$(OUT_DIR)/%.vpp.mbsdiff : source/%.vpp target/tables.vpp | $(OUT_DIR)
	$(Q)$(MINIBSDIFF) gen "$<" "target/tables.vpp" "$@"
	$(Q)sha1sum "$<" > "$(basename $@).sha1"

.PHONY: all all_rtp all_mbsdiff clean
