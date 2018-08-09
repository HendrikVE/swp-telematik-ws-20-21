#!/usr/bin/env python
# -*- coding: UTF-8 -*-

import argparse
import sys


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
            run_flash()

        if build:
            run_build()

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


def run_flash():
    print('flash')


def run_build():
    print('build')


def run_upload():
    print('upload')


if __name__ == '__main__':

    try:
        main(sys.argv[1:])

    except Exception as e:
        print(str(e))
