

COMPONENT_SRCDIRS += ./tasks ./authentication
COMPONENT_ADD_INCLUDEDIRS += ./inc ./tasks/inc ./authentication/inc

COMPONENT_EMBED_TXTFILES := certificate/thingspeak_https_certificate.pem
COMPONENT_EMBED_TXTFILES += certificate/thingspeak_mqtts_certificate.pem
COMPONENT_EMBED_TXTFILES += certificate/adafruit_mqtts_certificate.pem
COMPONENT_EMBED_TXTFILES += certificate/wifi_icon.png
COMPONENT_EMBED_TXTFILES += certificate/index.html
COMPONENT_EMBED_TXTFILES += authentication/full_gcloud.pem
