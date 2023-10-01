#!/bin/sh

hugo && rsync -av "$@" public/ marka@bionic.bloovis.com:/var/www/html/meguide
