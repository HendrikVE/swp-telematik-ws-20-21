#!/usr/bin/env python
# -*- coding: UTF-8 -*-

"""
execute this script with
    python -m tools.batch
"""

import argparse
import sys
import os

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

script_dir = os.path.dirname(os.path.realpath(__file__))


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
        sys.exit('Invalid number')

    for i in range(count):

        room = None
        device_id = None

        if arg_flash or arg_build:
            room = raw_input('room: ')

        if arg_flash or arg_build or arg_upload:
            device_id = raw_input('deviceID: ')

        if arg_flash:
            run_flash(room, device_id)

        if arg_build:
            run_build(room, device_id)

        if arg_upload:
            run_upload(device_id, get_version_code())


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

    process = Popen(['bash', script_dir + '/flash.sh', device_room, device_id], stdout=PIPE, stderr=STDOUT)

    for line in iter(process.stdout.readline, ''):
        print(line.strip())


def run_build(device_room, device_id):

    process = Popen(['bash', script_dir + '/build.sh', device_room, device_id], stdout=PIPE, stderr=STDOUT)

    for line in iter(process.stdout.readline, ''):
        print(line.strip())


def run_upload(device_id, version_code):

    src = os.path.normpath(script_dir + '/../../ESP32/window_alert/build/window_alert.bin')
    dest = '/var/www/html/{DEVICE_ID}/{VERSION_CODE}'.format(DEVICE_ID=device_id, VERSION_CODE=version_code)

    sudo("mkdir -p %s" % dest, user="www-data")

    put(src, dest, use_sudo=True)


def get_version_code():

    path = os.path.normpath(script_dir + '/../../ESP32/window_alert/main/MANIFEST.h')
    process = Popen(['grep "APP_VERSION_CODE" ' + path], stdout=PIPE, stderr=STDOUT, shell=True)

    output = process.communicate()[0]

    index_equal = output.rfind('=')
    index_semicolon = output.rfind(';')
    output_substring = output[index_equal+1:index_semicolon].strip()

    try:
        version_code = int(output_substring)

    except ValueError:
        sys.exit('Can not cast version code "%s" to int' % output_substring)

    return version_code


if __name__ == '__main__':

    try:
        main(sys.argv[1:])

    except Exception as e:
        print(str(e))
