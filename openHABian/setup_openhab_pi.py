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

    def create_key(output_file, bits, encryption=True):
        sudo('openssl genrsa {ENCRYPTION} \
             -out {FILE} {BITS}'.format(FILE=output_file, BITS=bits, ENCRYPTION='-aes256' if encryption else ''))
        sudo('chmod 400 {FILE}'.format(FILE=output_file))

    def create_cert_with_key(key_file, output_file, lifetime):
        sudo('openssl req \
             -config openssl.cnf \
             -new -x509 \
             -sha256 -extensions v3_ca \
             -days {LIFETIME} \
             -key {KEY_FILE} \
             -out {OUTPUT_FILE}'.format(KEY_FILE=key_file, OUTPUT_FILE=output_file, LIFETIME=lifetime))

    def create_cert_with_req(key_file, csr_output_file, cert_output_file, lifetime, extension, config_file_csr, config_file_cert):
        sudo('openssl req \
             -config {CONFIG_FILE_CSR} \
             -new -sha256 \
             -key {KEY_FILE} \
             -out {CSR_OUTPUT_FILE}'.format(KEY_FILE=key_file, CSR_OUTPUT_FILE=csr_output_file, CONFIG_FILE_CSR=config_file_csr))

        sudo('openssl ca \
             -config {CONFIG_FILE_CERT} \
             -extensions {EXTENSION} \
             -days {LIFETIME} \
             -notext -md sha256 \
             -in {CSR_OUTPUT_FILE} \
             -out {CERT_OUTPUT_FILE}'.format(LIFETIME=lifetime, CSR_OUTPUT_FILE=csr_output_file, CERT_OUTPUT_FILE=cert_output_file, CONFIG_FILE_CERT=config_file_cert, EXTENSION=extension))

        sudo('chmod 444 {CERT_OUTPUT_FILE}'.format(CERT_OUTPUT_FILE=cert_output_file))

    def verify_cert(ca_file, cert_file):
        sudo('openssl verify -CAfile {CA_FILE} {CERT_FILE}'.format(CA_FILE=ca_file, CERT_FILE=cert_file))

    print('setup SSL for Mosquitto MQTT broker')

    home_dir = _get_homedir_openhabian()

    with cd(home_dir):

        ca_dir = os.path.join(home_dir, 'mosquittoCA')

        sudo('mkdir %s' % ca_dir)
        sudo('chmod 700 %s' % ca_dir)

        with cd(ca_dir):

            # prepare the main directory
            sudo('mkdir certs crl newcerts private')
            sudo('chmod 700 private')
            sudo('touch index.txt')
            sudo('echo 1000 > serial')
            put(os.path.join('res', 'mosquitto_certs', 'root-config.txt'), '%s/openssl.cnf' % ca_dir, use_sudo=True)

            # prepare the directory for intermediates
            sudo('mkdir intermediate')

            ca_intermediate_dir = os.path.join(ca_dir, 'intermediate')
            with cd(ca_intermediate_dir):
                sudo('mkdir certs crl csr newcerts private')
                sudo('chmod 700 private')
                sudo('touch index.txt')
                sudo('echo 1000 > serial')
                put(os.path.join('res', 'mosquitto_certs', 'intermediate-config.txt'), '%s/openssl.cnf' % ca_intermediate_dir, use_sudo=True)

            # root cert and key
            create_key('private/ca.key.pem', 4096)
            create_cert_with_key('private/ca.key.pem', 'certs/ca.cert.pem', 7300)

            """
            # intermediate cert and key
            create_key('intermediate/private/intermediate.key.pem', 4096)

            create_cert_with_req('intermediate/private/intermediate.key.pem',
                                 'intermediate/csr/intermediate.csr.pem',
                                 'intermediate/certs/intermediate.cert.pem',
                                 3650,
                                 'v3_intermediate_ca',
                                 'intermediate/openssl.cnf', 'openssl.cnf')

            verify_cert('certs/ca.cert.pem', 'intermediate/certs/intermediate.cert.pem')

            sudo('cat intermediate/certs/intermediate.cert.pem certs/ca.cert.pem > intermediate/certs/ca-chain.cert.pem')
            sudo('chmod 444 intermediate/certs/ca-chain.cert.pem')
            """

            # server cert and key
            create_key('intermediate/private/server.key.pem', 2048, False)

            create_cert_with_req('intermediate/private/server.key.pem',
                                 'intermediate/csr/server.csr.pem',
                                 'intermediate/certs/server.cert.pem',
                                 375,
                                 'server_cert',
                                 'intermediate/openssl.cnf', 'intermediate/openssl.cnf')

            verify_cert('certs/ca.cert.pem', 'intermediate/certs/server.cert.pem')

            # copy certificates to mosquitto
            sudo('cp certs/ca.cert.pem /etc/mosquitto/ca_certificates/')
            sudo('cp intermediate/certs/server.cert.pem intermediate/private/server.key.pem /etc/mosquitto/certs/')

            # make references in mosquitto config
            sudo('echo "cafile /etc/mosquitto/ca_certificates/ca.cert.pem" >> /etc/mosquitto/mosquitto.conf')
            sudo('echo "certfile /etc/mosquitto/certs/server.cert.pem" >> /etc/mosquitto/mosquitto.conf')
            sudo('echo "keyfile /etc/mosquitto/certs/server.key.pem" >> /etc/mosquitto/mosquitto.conf')

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
