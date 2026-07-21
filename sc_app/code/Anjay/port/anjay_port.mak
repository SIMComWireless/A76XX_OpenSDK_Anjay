# anjay_port.mak
#
# Makefile snippet for integrating Anjay LWM2M client into SIMCOM SDK build
#
# Usage: Include this file in your application's Makefile or add it to the
#        SDK's build system.
#
# Prerequisites:
#   - Anjay source code in SIMCOM_SDK_SET/public_libs/Anjay/
#   - avs_commons submodule initialized

# Anjay directory
ANJAY_DIR := $(ROOT_DIR)/public_libs/Anjay
ANJAY_PORT_DIR := $(ANJAY_DIR)/port

# Anjay source files
ANJAY_CORE_SRC := $(wildcard $(ANJAY_DIR)/src/core/*.c) \
                  $(wildcard $(ANJAY_DIR)/src/core/**/*.c) \
                  $(wildcard $(ANJAY_DIR)/src/modules/*.c) \
                  $(wildcard $(ANJAY_DIR)/src/modules/**/*.c)

# avs_commons source files (required by Anjay)
AVS_COMMONS_DIR := $(ANJAY_DIR)/deps/avs_commons
AVS_COMMONS_SRC := $(wildcard $(AVS_COMMONS_DIR)/src/net/*.c) \
                   $(wildcard $(AVS_COMMONS_DIR)/src/net/**/*.c) \
                   $(wildcard $(AVS_COMMONS_DIR)/src/crypto/*.c) \
                   $(wildcard $(AVS_COMMONS_DIR)/src/crypto/**/*.c) \
                   $(wildcard $(AVS_COMMONS_DIR)/src/log/*.c) \
                   $(wildcard $(AVS_COMMONS_DIR)/src/list/*.c) \
                   $(wildcard $(AVS_COMMONS_DIR)/src/buffer/*.c) \
                   $(wildcard $(AVS_COMMONS_DIR)/src/sched/*.c) \
                   $(wildcard $(AVS_COMMONS_DIR)/src/stream/*.c) \
                   $(wildcard $(AVS_COMMONS_DIR)/src/url/*.c) \
                   $(wildcard $(AVS_COMMONS_DIR)/src/utils/*.c) \
                   $(wildcard $(AVS_COMMONS_DIR)/src/compat/threading/*.c)

# avs_coap source files
AVS_COAP_DIR := $(ANJAY_DIR)/deps/avs_coap
AVS_COAP_SRC := $(wildcard $(AVS_COAP_DIR)/src/*.c) \
                $(wildcard $(AVS_COAP_DIR)/src/**/*.c)

# Port layer source files
PORT_SRC := $(wildcard $(ANJAY_PORT_DIR)/src/*.c)

# All Anjay-related sources
ANJAY_ALL_SRC := $(ANJAY_CORE_SRC) $(AVS_COMMONS_SRC) $(AVS_COAP_SRC) $(PORT_SRC)

# Include directories
ANJAY_INCLUDES := -I$(ANJAY_DIR)/include_public \
                  -I$(AVS_COMMONS_DIR)/include_public \
                  -I$(AVS_COAP_DIR)/include_public \
                  -I$(ANJAY_PORT_DIR)/inc \
                  -I$(SDK_INC_DIR)

# Compile definitions
ANJAY_DEFINES := -DAVS_COMMONS_WITH_CUSTOM_TLS \
                 -DSIMCOM_SDK=ON

# Add to CFLAGS
CFLAGS += $(ANJAY_INCLUDES) $(ANJAY_DEFINES)

# Object files
ANJAY_OBJ_DIR := $(OUT_DIR)/anjay
ANJAY_OBJ := $(patsubst $(ROOT_DIR)/%.c,$(ANJAY_OBJ_DIR)/%.o,$(ANJAY_ALL_SRC))

# Compile rule for Anjay sources
$(ANJAY_OBJ_DIR)/%.o: $(ROOT_DIR)/%.c
	@mkdir -p $(dir $@)
	@echo "CC $<"
	@$(CC) $(CFLAGS) -c $< -o $@

# Add Anjay objects to the link list
OBJS += $(ANJAY_OBJ)

# Anjay clean target
anjay_clean:
	rm -rf $(ANJAY_OBJ_DIR)

.PHONY: anjay_clean
