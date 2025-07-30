#!/bin/bash
valgrind --leak-check=full --log-file=vg.output ./build/src/pipedald /etc/pipedal/config ./vite/dist -port 0.0.0.0:8080 -log-level debug
