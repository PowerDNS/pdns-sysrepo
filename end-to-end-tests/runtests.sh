#!/bin/sh
set -x

venv=".venv"

if [ ! -d "${venv}" ]; then
  python3 -m virtualenv -p python3 ${venv}
fi

. ${venv}/bin/activate
python -V
pip install -U pip
pip install -r requirements.txt

export EDITOR=""
export VISUAL=""
nosetests --with-xunit $@
