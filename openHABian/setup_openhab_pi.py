#!/usr/bin/env python3
#  -*- coding:utf-8 -*-

import os
import textwrap
import time

from fabric.api import env, task, run, sudo, execute, put, get
from fabric.context_managers import cd
from fabric.contrib.files import append

from config import config as config

env.host_string = config.SERVER_IP
env.user = config.SSH_USERNAME


@task
def setup(install_display=False):
    execute(add_sudo_user)
    execute(copy_openhab_files)
    # execute(setup_mosquitto, True)
    setup_ssl_for_mosquitto()

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
    _replace_inplace_file('<insert password>', config.MQTT_PASSWORD, path_mqtt_cfg)

    path_influxdb_cfg = os.path.join(dest_path, 'services', 'influxdb.cfg')
    _replace_inplace_file('<insert username>', config.INFLUXDB_USERNAME_OPENHAB, path_influxdb_cfg)
    _replace_inplace_file('<insert password>', config.INFLUXDB_PASSWORD_OPENHAB, path_influxdb_cfg)

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
def setup_influxDB_and_grafana():
    print('setup InfluxDB and Grafana')

    # print('Please install the following: Optional Components -> InfluxDB+Grafana')
    # sudo('openhabian-config')

    _setup_influxDB()
    _setup_grafana()


def _setup_influxDB():
    print('setup influxDB')

    run("""echo "CREATE DATABASE {DBNAME}" | influx""".format(DBNAME=config.INFLUXDB_DB_NAME))
    run("""echo "CREATE USER admin WITH PASSWORD '{PASSWORD}' WITH ALL PRIVILEGES" | influx""".format(PASSWORD=config.INFLUXDB_PASSWORD_ADMIN))
    run("""echo "CREATE USER {USERNAME} WITH PASSWORD '{PASSWORD}'" | influx""".format(USERNAME=config.INFLUXDB_USERNAME_OPENHAB, PASSWORD=config.INFLUXDB_PASSWORD_OPENHAB))
    run("""echo "CREATE USER {USERNAME} WITH PASSWORD '{PASSWORD}'" | influx""".format(USERNAME=config.INFLUXDB_USERNAME_GRAFANA, PASSWORD=config.INFLUXDB_PASSWORD_GRAFANA))
    run("""echo "GRANT ALL ON {DBNAME} TO {USERNAME}" | influx""".format(DBNAME=config.INFLUXDB_DB_NAME, USERNAME=config.INFLUXDB_USERNAME_OPENHAB))
    run("""echo "GRANT READ ON {DBNAME} TO {USERNAME}" | influx""".format(DBNAME=config.INFLUXDB_DB_NAME, USERNAME=config.INFLUXDB_USERNAME_GRAFANA))

    res_path = os.path.join('res', 'influxdb', 'influxdb.conf')
    dest_path = '/etc/influxdb/influxdb.conf'

    sudo('cp {FILE} {FILE}.old'.format(FILE=dest_path))
    put(res_path, dest_path, use_sudo=True)

    sudo('sudo systemctl restart influxdb.service')


def _setup_grafana():
    print('setup grafana')

    res_path = os.path.join('res', 'grafana', 'grafana.ini')
    dest_path = '/etc/grafana/grafana.ini'

    sudo('cp {FILE} {FILE}.old'.format(FILE=dest_path))
    put(res_path, dest_path, use_sudo=True)


@task
def setup_ssl_for_mosquitto():
    """
    for reference see: http://rockingdlabs.dunmire.org/exercises-experiments/ssl-client-certs-to-secure-mqtt
    certificates generated as shown here: https://jamielinux.com/docs/openssl-certificate-authority/index.html
    """

    def get_host_ipv4():
        return run('hostname -I | cut -d " " -f 1')

    def get_host_name():
        return run('hostname')

    print('setup SSL for Mosquitto MQTT broker')

    host_ipv4 = get_host_ipv4()
    host_name = get_host_name()
    client_name = 'esp32'

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
            sudo('IPLIST="{IP}" HOSTLIST="{HOSTNAME}" ./generate-CA.sh'.format(IP=host_ipv4, HOSTNAME=host_name))

            # create client cert + key
            sudo('IPLIST="{IP}" HOSTLIST="{HOSTNAME}" ./generate-CA.sh client {CLIENT_NAME}'.format(IP=host_ipv4, HOSTNAME=host_name, CLIENT_NAME=client_name))

            # copy certificates to mosquitto
            sudo('cp ca.crt /etc/mosquitto/ca_certificates/')
            sudo('cp {HOSTNAME}.crt {HOSTNAME}.key /etc/mosquitto/certs/'.format(HOSTNAME=host_name))

            # make references in mosquitto config
            listener_1883_config = '\nlistener 1883 localhost'
            listener_8883_config = textwrap.dedent("""
                listener 8883
                cafile /etc/mosquitto/ca_certificates/ca.crt
                certfile /etc/mosquitto/certs/{HOSTNAME}.crt
                keyfile /etc/mosquitto/certs/{HOSTNAME}.key
                require_certificate true
                # use_identity_as_username true
            """.format(HOSTNAME=host_name))

            append('/etc/mosquitto/mosquitto.conf', listener_1883_config, use_sudo=True)
            append('/etc/mosquitto/mosquitto.conf', listener_8883_config, use_sudo=True)

            sudo('service mosquitto restart')

            get('ca.crt', os.path.join('..', 'ESP32', 'window_alert', 'main', 'ca.crt'), use_sudo=True)
            get('%s.crt' % client_name, os.path.join('..', 'ESP32', 'window_alert', 'main', 'client.crt'), use_sudo=True)
            get('%s.key' % client_name, os.path.join('..', 'ESP32', 'window_alert', 'main', 'client.key'), use_sudo=True)

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


def _replace_inplace_file(pattern, replacement, file):
    sudo('sed -i "s/{PATTERN}/{REPLACEMENT}/g" {FILE}'.format(PATTERN=pattern, REPLACEMENT=replacement, FILE=file))
