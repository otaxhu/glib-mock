all: format

format:
	ruff format glib-mock-run.py

.PHONY: all format
