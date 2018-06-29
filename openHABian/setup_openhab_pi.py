#!/usr/bin/env python3
#  -*- coding:utf-8 -*-

import os
import textwrap

from fabric.api import env, task, run, sudo, execute, put
from fabric.context_managers import cd
from fabric.contrib.files import append

from config import config as config

env.host_string = config.SERVER_IP
env.user = config.SSH_USERNAME


@task
def setup(install_display=False):
    execute(add_sudo_user)
    execute(copy_openhab_files)
    #execute(setup_mosquitto, True)

    if install_display:
        execute(install_adafruit_display)


@task
def add_sudo_user():
    print('add new user with root permission')

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
    print('copy config files for openhab2')

    res_path = os.path.join('res', 'openhab2')
    dest_path = os.path.join(os.sep, 'etc', 'openhab2')

    put(os.path.join(res_path, 'items'), dest_path, use_sudo=True)
    put(os.path.join(res_path, 'rules'), dest_path, use_sudo=True)
    put(os.path.join(res_path, 'services'), dest_path, use_sudo=True)
    put(os.path.join(res_path, 'sitemaps'), dest_path, use_sudo=True)

    path_mqtt_cfg = os.path.join(dest_path, 'services', 'mqtt.cfg')
    sudo('sed -i "s/<insert password>/{MQTT_PASSWORD}/g" {FILE}'.format(MQTT_PASSWORD=config.MQTT_PASSWORD, FILE=path_mqtt_cfg))

    sudo('chown -R openhab:openhabian %s/*' % dest_path)


@task
def setup_mosquitto(ssl=False):
    print("not implemented yet")

    """
    sudo('apt install mosquitto')
    
    if ssl:
        setup_ssl_for_mosquitto()
    """


@task
def setup_ssl_for_mosquitto():
    """
    for reference see: http://rockingdlabs.dunmire.org/exercises-experiments/ssl-client-certs-to-secure-mqtt
    certificates generated as shown here: https://jamielinux.com/docs/openssl-certificate-authority/index.html
    """

    print('setup SSL for Mosquitto MQTT broker')

    res_path = os.path.join('res', 'mosquitto')
    home_dir = _get_homedir_openhabian()

    with cd(home_dir):

        ca_dir = os.path.join(home_dir, 'mosquittoCA')

        sudo('mkdir %s' % ca_dir)
        sudo('chmod 700 %s' % ca_dir)

        with cd(ca_dir):

            put(os.path.join(res_path, 'generate-CA.sh'), 'generate-CA.sh', use_sudo=True)
            sudo('chmod +x generate-CA.sh')

            # create ca cert and server cert + key
            sudo('IPLIST="192.168.2.110" HOSTLIST="openHABianPi" ./generate-CA.sh')

            # create client cert + key
            sudo('IPLIST="192.168.2.110" HOSTLIST="openHABianPi" ./generate-CA.sh client esp32')

            # copy certificates to mosquitto
            sudo('cp ca.crt /etc/mosquitto/ca_certificates/')
            sudo('cp openHABianPi.crt openHABianPi.key /etc/mosquitto/certs/')

            # make references in mosquitto config
            listener_1883_config = 'listener 1883 localhost'
            listener_8883_config = textwrap.dedent("""
                listener 8883
                cafile /etc/mosquitto/ca_certificates/ca.crt
                certfile /etc/mosquitto/certs/openHABianPi.crt
                keyfile /etc/mosquitto/certs/openHABianPi.key
                require_certificate true
                use_identity_as_username true
            """)

            append('/etc/mosquitto/mosquitto.conf', listener_1883_config, use_sudo=True)
            append('/etc/mosquitto/mosquitto.conf', listener_8883_config, use_sudo=True)

            sudo('service mosquitto restart')

        sudo('chown -R openhab:openhabian %s' % ca_dir)


@task
def install_adafruit_display():
    print('install adafruit display')

    home_dir = _get_homedir_openhabian()

    with cd(home_dir):
        sudo('wget https://raw.githubusercontent.com/adafruit/Raspberry-Pi-Installer-Scripts/master/adafruit-pitft.sh')
        sudo('chmod +x adafruit-pitft.sh')
        sudo('./adafruit-pitft.sh -u %s' % home_dir)
        sudo('rm adafruit-pitft.sh')

    sudo('apt install wiringpi')


"""
def _put_as_user(src, dest, user, group=None):

    if group is None:
        group = user

    put(src, dest, use_sudo=True)

    if os.path.isfile(src) and dest.endswith(os.sep):
        path = os.path.join(dest, os.path.basename(src))

    elif os.path.isfile(src) and not dest.endswith(os.sep):
        path = dest

    else:
        path = os.path.join(dest, os.path.basename(src))

    sudo('chown -R {user}:{group} {path}'.format(user=user, group=group, path=path))
"""


def _get_homedir_openhabian():
    return os.path.join(os.sep, 'home', 'openhabian')
