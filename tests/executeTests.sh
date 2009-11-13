#!/bin/sh

#hpunit --repeat 5 -d include_path=classes:/usr/share/php Redis_Test ./TestRedis.php
phpunit --repeat 1 Redis_Test ./TestRedis.php