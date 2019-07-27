

COMPONENT_SRCDIRS += ./tasks
COMPONENT_ADD_INCLUDEDIRS += ./inc ./tasks/inc

COMPONENT_EMBED_TXTFILES := certificate/thingspeak_https_certificate.pem
COMPONENT_EMBED_TXTFILES += certificate/thingspeak_mqtts_certificate.pem
COMPONENT_EMBED_TXTFILES += certificate/adafruit_mqtts_certificate.pem
