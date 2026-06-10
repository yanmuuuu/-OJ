# OnlineJudge 顶层构建入口
# 子项目: compile_server (代码编译运行) / oj_server (Web 判题服务)

PROJECT_ROOT := $(abspath .)
SUBDIRS      := compile_server oj_server
OUTPUT_DIR   := $(PROJECT_ROOT)/output

.PHONY: all output clean help $(SUBDIRS)

all: $(SUBDIRS)

$(SUBDIRS):
	$(MAKE) -C $@

# 打包可部署目录到 output/
output: all
	@mkdir -p $(OUTPUT_DIR)/compile_server/temp
	@mkdir -p $(OUTPUT_DIR)/oj_server
	cp -f compile_server/compile_server $(OUTPUT_DIR)/compile_server/
	cp -rf oj_server/conf      $(OUTPUT_DIR)/oj_server/
	cp -rf oj_server/questions $(OUTPUT_DIR)/oj_server/
	cp -rf oj_server/template    $(OUTPUT_DIR)/oj_server/
	cp -rf oj_server/wwwroot     $(OUTPUT_DIR)/oj_server/
	cp -rf oj_server/mysql       $(OUTPUT_DIR)/oj_server/
	cp -f oj_server/oj_server    $(OUTPUT_DIR)/oj_server/

clean:
	@for dir in $(SUBDIRS); do \
		$(MAKE) -C $$dir clean; \
	done
	rm -rf $(OUTPUT_DIR)

help:
	@echo "Usage:"
	@echo "  make         编译 compile_server 与 oj_server"
	@echo "  make output  编译并复制到 $(OUTPUT_DIR)/"
	@echo "  make clean   清理编译产物与 output/"
	@echo "  make help    显示本帮助"
