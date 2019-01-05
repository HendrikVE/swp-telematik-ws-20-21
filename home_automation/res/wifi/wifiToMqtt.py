import paho.mqtt.client as mqtt
import sys
import subprocess
# ====================================================
# MQTT Settings
MQTT_Broker = "192.168.178.119"
MQTT_Port = 1883
MQTT_USER_NAME="openhabian"
MQTT_USER_PASSWORD="VXGZgUvJUjaXYNf"
Keep_Alive_Interval = 45
MQTT_Topic_Wifi_Quality = "environment/wifi/quality"
# WIFI Settings
interface = "wlan0"
# ====================================================

def on_connect(client, userdata, rc):
    if rc != 0:
        pass
        print
        "Unable to connect to MQTT Broker..."
    else:
        print
        "Connected with MQTT Broker: " + str(MQTT_Broker)


def on_publish(client, userdata, mid):
    pass


def on_disconnect(client, userdata, rc):
    if rc != 0:
        pass


mqttc = mqtt.Client()
mqttc.username_pw_set(MQTT_USER_NAME, MQTT_USER_PASSWORD)
mqttc.on_connect = on_connect
mqttc.on_disconnect = on_disconnect
mqttc.on_publish = on_publish
mqttc.connect(MQTT_Broker, int(MQTT_Port), int(Keep_Alive_Interval))


def publish_To_Topic(topic, message):
    mqttc.publish(topic, message)

def get_quality(cell):
    quality = matching_line(cell,"Quality=").split()[0].split('/')
    return int(round(float(quality[0]) / float(quality[1]) * 100))

def matching_line(lines, keyword):
    for line in lines:
        matching=match(line,keyword)
        if matching!=None:
            return matching
    return None

def match(line,keyword):
    line=line.lstrip()
    length=len(keyword)
    if line[:length] == keyword:
        return line[length:]
    else:
        return None

def main():
    cells=[[]]
    parsed_cells=[]

    proc = subprocess.Popen(["iwlist", interface, "scan"],stdout=subprocess.PIPE, universal_newlines=True)
    out, err = proc.communicate()

    for line in out.split("\n"):
        cell_line = match(line,"Cell ")
        if cell_line != None:
            cells.append([])
            line = cell_line[-27:]
        cells[-1].append(line.rstrip())

    cells=cells[1:]

    for cell in cells:
        qual =  get_quality(cell)
        publish_To_Topic(MQTT_Topic_Wifi_Quality, qual)

if __name__ == '__main__':
  main()
