#!/usr/bin/env python
# -*- coding: UTF-8 -*-

"""
execute this script with
    python -m tools.batch
"""

import argparse
import sys
from subprocess import PIPE, Popen, STDOUT

from fabric.api import env, sudo, put

from config import config as config

env.host_string = config.SERVER_IP
env.user = config.SSH_USERNAME


def main(argv):

    parser = init_argparse()

    try:
        args = parser.parse_args(argv)

    except Exception as e:
        print(str(e))
        return

    flash = args.flash
    build = args.build
    upload = args.upload
    all = args.all
    ota = args.ota

    if all:
        flash = True
        build = True
        upload = True

    if ota:
        build = True
        upload = True

    input = raw_input('number of batch jobs: ')
    try:
        count = int(input)

    except ValueError:
        sys.exit("Invalid number")

    for i in range(count):

        if flash:
            room = raw_input('room: ')
            id = raw_input('deviceID: ')
            run_flash(room, id)

        if build:
            room = raw_input('room: ')
            id = raw_input('deviceID: ')
            run_build(room, id)

        if upload:
            run_upload()


def init_argparse():

    parser = argparse.ArgumentParser(description='Create a batch of jobs')

    parser.add_argument('--flash',
                        dest='flash', action='store_true', default=False,
                        required=False)

    parser.add_argument('--build',
                        dest='build', action='store_true', default=False,
                        required=False)

    parser.add_argument('--upload',
                        dest='upload', action='store_true', default=False,
                        required=False)

    parser.add_argument('--all',
                        dest='all', action='store_true', default=False,
                        required=False)

    parser.add_argument('--ota',
                        dest='ota', action='store_true', default=False,
                        required=False)

    return parser


def run_flash(device_room, device_id):

    process = Popen(['./flash.sh', device_room, device_id], stdout=PIPE, stderr=STDOUT)

    for line in iter(process.stdout.readline, ''):
        print(line.strip())


def run_build(device_room, device_id):

    process = Popen(['./build.sh', device_room, device_id], stdout=PIPE, stderr=STDOUT)

    for line in iter(process.stdout.readline, ''):
        print(line.strip())


def run_upload():

    put('file', 'remote file')


if __name__ == '__main__':

    try:
        main(sys.argv[1:])

    except Exception as e:
        print(str(e))
