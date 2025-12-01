#!/bin/bash
set -e

cd client && make re
cd ..
cd server && make re
cd ..
