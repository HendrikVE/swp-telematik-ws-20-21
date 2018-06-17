#!/usr/bin/env python3
#  -*- coding:utf-8 -*-

import os

from fabric.api import env, task, run, sudo, execute, put
from fabric.context_managers import cd

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
    run('echo "copy config files for openhab2"')

    res_path = os.path.join('res', 'openhab2')
    dest_path = os.path.join(os.sep, 'etc', 'openhab2')

    put(os.path.join(res_path, 'items'), dest_path, use_sudo=True)
    put(os.path.join(res_path, 'rules'), dest_path, use_sudo=True)
    put(os.path.join(res_path, 'services'), dest_path, use_sudo=True)
    put(os.path.join(res_path, 'sitemaps'), dest_path, use_sudo=True)

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

    home_dir = _get_homedir_openhabian()

    with cd(home_dir):

        ca_dir = os.path.join(home_dir, 'mosquittoCA')

        sudo('mkdir %s' % ca_dir)
        sudo('chmod 700 %s' % ca_dir)

        with cd(ca_dir):

            # prepare the directory
            sudo('mkdir certs crl newcerts private')
            sudo('chmod 700 private')
            sudo('touch index.txt')
            sudo('echo 1000 > serial')

            # copy config to remote
            put(os.path.join('res', 'mosquitto_certs', 'root-config.txt'), '%s/openssl.cnf' % ca_dir, use_sudo=True)

            # create the root key
            sudo('openssl genrsa -aes256 -out private/ca.key.pem 4096')
            sudo('chmod 400 private/ca.key.pem')

            # generate root certificate
            sudo('openssl req -config openssl.cnf '
                 '-key private/ca.key.pem '
                 '-new -x509 -days 7300 -sha256 -extensions v3_ca '
                 '-out certs/ca.cert.pem')
            #sudo('openssl x509 -noout -text -in certs/ca.cert.pem')

            # prepare the directory
            sudo('mkdir intermediate')

            ca_intermediate_dir = os.path.join(ca_dir, 'intermediate')

            with cd(ca_intermediate_dir):
                sudo('mkdir certs crl csr newcerts private')
                sudo('chmod 700 private')
                sudo('touch index.txt')
                sudo('echo 1000 > serial')

                # copy config to remote
                put(os.path.join('res', 'mosquitto_certs', 'intermediate-config.txt'), '%s/openssl.cnf' % ca_intermediate_dir, use_sudo=True)

            # create the intermediate key
            sudo('openssl genrsa -aes256 -out intermediate/private/intermediate.key.pem 4096')
            sudo('chmod 400 intermediate/private/intermediate.key.pem')

            # create the intermediate key
            sudo('openssl req '
                 '-config intermediate/openssl.cnf \
                 -new -sha256 -key intermediate/private/intermediate.key.pem \
                 -out intermediate/csr/intermediate.csr.pem')

            # create the intermediate certificate
            sudo('openssl ca -config openssl.cnf -extensions v3_intermediate_ca \
                 -days 3650 -notext -md sha256 \
                 -in intermediate/csr/intermediate.csr.pem \
                 -out intermediate/certs/intermediate.cert.pem')
            sudo('chmod 444 intermediate/certs/intermediate.cert.pem')

            # verify the intermediate certificate
            #sudo('openssl x509 -noout -text -in intermediate/certs/intermediate.cert.pem')
            sudo('openssl verify -CAfile certs/ca.cert.pem intermediate/certs/intermediate.cert.pem')

            sudo('cat intermediate/certs/intermediate.cert.pem certs/ca.cert.pem > intermediate/certs/ca-chain.cert.pem')
            sudo('chmod 444 intermediate/certs/ca-chain.cert.pem')

            return

            # copy certificates to mosquitto
            sudo('cp ca.crt /etc/mosquitto/ca_certificates/')
            sudo('cp myhost.crt myhost.key /etc/mosquitto/certs/')

            # make references in mosquitto config
            sudo('echo "cafile /etc/mosquitto/ca_certificates/ca.crt" >> /etc/mosquitto/mosquitto.conf')
            sudo('echo "certfile /etc/mosquitto/certs/myhost.crt" >> /etc/mosquitto/mosquitto.conf')
            sudo('echo "keyfile /etc/mosquitto/certs/myhost.key" >> /etc/mosquitto/mosquitto.conf')

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
