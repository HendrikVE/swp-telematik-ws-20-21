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

arg_flash = None
arg_build = None
arg_upload = None
arg_all = None
arg_ota = None


def main(argv):

    parser = init_argparse()

    try:
        args = parser.parse_args(argv)

    except Exception as e:
        print(str(e))
        return

    evaluate_args(args)

    input = raw_input('number of batch jobs: ')
    try:
        count = int(input)

    except ValueError:
        sys.exit("Invalid number")

    for i in range(count):

        if arg_flash:
            room = raw_input('room: ')
            id = raw_input('deviceID: ')
            run_flash(room, id)

        if arg_build:
            room = raw_input('room: ')
            id = raw_input('deviceID: ')
            run_build(room, id)

        if arg_upload:
            run_upload()


def evaluate_args(args):

    global arg_flash, arg_build, arg_upload, arg_all, arg_ota

    arg_flash = args.flash
    arg_build = args.build
    arg_upload = args.upload
    arg_all = args.all
    arg_ota = args.ota

    if arg_all:
        arg_flash = True
        arg_build = True
        arg_upload = True

    if arg_ota:
        arg_build = True
        arg_upload = True

    if arg_upload:
        arg_build = True


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
