#!/usr/bin/env python3
#  -*- coding:utf-8 -*-

import os

from fabric.api import env, task, run, sudo, execute, put
from fabric.context_managers import cd

from config import config as config

env.host_string = config.SERVER_IP
env.user = config.SSH_USERNAME

@task
def setup():
    execute(add_sudo_user)
    execute(copy_openhab_files)

@task
def add_sudo_user():
    run('echo "add new user with root permission"')

    sudo('adduser %s' % config.NEW_USERNAME)
    sudo('usermod -aG sudo %s' % config.NEW_USERNAME)

    with cd('/home/%s' % config.NEW_USERNAME):
        sudo('mkdir .ssh')
        sudo('touch .ssh/authorized_keys')
        sudo('chmod 600 .ssh/authorized_keys')
        sudo('echo "%s" >> .ssh/authorized_keys' % config.NEW_USER_PUBLIC_KEY)
        sudo('chown -R {user}:{user} .ssh'.format(user=config.NEW_USERNAME))


@task
def copy_openhab_files():

    def put_as_openhab(src, dest):
        put(src, dest, use_sudo=True)
        sudo('chown -R openhab:openhabian %s' % os.path.join(dest, os.path.basename(src)))

    res_path = os.path.join('res', 'openhab2')
    dest_path = os.path.join(os.sep, 'etc', 'openhab2')

    put_as_openhab(os.path.join(res_path, 'items'), dest_path)
    put_as_openhab(os.path.join(res_path, 'rules'), dest_path)
    put_as_openhab(os.path.join(res_path, 'services'), dest_path)
    put_as_openhab(os.path.join(res_path, 'sitemaps'), dest_path)
