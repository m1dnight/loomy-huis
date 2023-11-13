.PHONY: all
all: format

format:
	find main -type f -name "*.[hc]" -exec clang-format -i --style='file:.clang-format' {} \;
