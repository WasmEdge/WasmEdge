#!/usr/bin/env python
# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2019-2024 Second State INC

import os
import sys
from subprocess import Popen
import argparse
import shlex

def cmd_exec(command):
    cmd = shlex.split(command)
    proc = Popen(cmd)
    proc.wait()
    ret_code = proc.returncode
    if ret_code != 0:
        raise Exception("{} failed, return code is {}".format(cmd, ret_code))

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--path', help='Build path.')
    parser.add_argument('--command', help='Build command.')
    parser.add_argument('--enable', help='enable python.', nargs='*')
    args = parser.parse_args()

    if args.enable:
        if args.enable[0] == 'false':
            return

    if args.path:
        curr_dir = os.getcwd()
        os.chdir(args.path)
        if args.command:
            if '&&' in args.command:
                command = args.command.split('&&')
                for data in command:
                    cmd_exec(data)
            else:
                cmd_exec(args.command)
        os.chdir(curr_dir)

if __name__ == '__main__':
    sys.exit(main())
