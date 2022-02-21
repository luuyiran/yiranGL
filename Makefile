#####################################################
CC=gcc
APP=yiran
BIN_DIR:=bin
TARGET:=$(BIN_DIR)/$(APP)
BUILD_DIR:=build

SANITIZE= -g -fstack-protector-all -fsanitize=address -fno-omit-frame-pointer -fsanitize=leak
CFLAG_89 = -std=c89 -W -Wall -pedantic  -O3 $(SANITIZE) # for our projects
CFLAG_99 = -std=c99 -O3 $(SANITIZE) 				    # for  glad, stb
LIBS =-lm -ldl -lGL -lX11  $(SANITIZE)

#####################################################
SRC_EXT = src/glad.c src/stb_image.c
SRC = src/nuklear.c src/array.c  src/camera.c  src/draw.c  src/render_config.c \
	  src/utils.c   src/skybox.c src/model.c   src/x11.c   src/cube_map.c      \
	  src/mat4.c    src/rbtree.c src/shader.c  src/main.c  src/noise.c

OBJ_EXT = $(SRC_EXT:%.c=$(BUILD_DIR)/%.o)
OBJ = $(SRC:%.c=$(BUILD_DIR)/%.o)

#####################################################
$(TARGET):$(OBJ_EXT) $(OBJ)
	@mkdir -p $(BIN_DIR)
	$(call cmd,link)

$(OBJ):$(BUILD_DIR)/%.o:%.c
	@mkdir -p $(BUILD_DIR)
	@mkdir -p $(dir $@)
	$(call cmd,c89)

$(OBJ_EXT):$(BUILD_DIR)/%.o:%.c
	@mkdir -p $(BUILD_DIR)
	@mkdir -p $(dir $@)
	$(call cmd,c99)

#####################################################
.PHONY:clean
clean:
	@rm -rvf $(OBJ_EXT) $(OBJ) $(TARGET) $(BUILD_DIR)

#####################################################
ifndef alias
    alias=quiet_
endif

cmd =  @echo ' $($(alias)cmd_$(1))' && $(cmd_$(1))

quiet_cmd_c99 =  Building C99 object $@
      cmd_c99 = $(CC)  $(CFLAG_99) -c -o $@ $<

quiet_cmd_c89 =  Building C89 object $@
      cmd_c89 = $(CC)  $(CFLAG_89) -c -o $@ $<

quiet_cmd_link   = Linking Executable $@
      cmd_link   = $(CC) -o $@ $^ $(LIBS)

