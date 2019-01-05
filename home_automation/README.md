# Usage

1. copy config_EXAMPLE.py and rename the copy to config.py
2. replace placeholder in config.py with your data
3. install fabric
    - create virtual environment
        - `python3 -m venv env`
        - `source ./env/bin/activate`
    - `pip install --upgrade pip`
    - `pip install --upgrade virtualenv`
    - `pip install -r requirements.txt`
4. run **"fab -f setup_openhab_pi.py setup"** to configure a Raspberry Pi which is already running openHABian

# Things to keep in mind
- the command `setup_ssl_for_mosquitto` (also executed in `setup_mosquitto:ssl=true`) downloads the certificates required for flashing the esp32 in to the project directory of the esp32 application
- if you dont run the aggregated `setup` command, please change the value "SSH_USERNAME" to your new user name you created with `add_sudo_user` as the user "openhab" does not work
    - this is especially important when executing `setup_ssl_for_mosquitto`