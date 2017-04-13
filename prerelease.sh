#!/bin/bash

echo
echo "-=[ THiNX: Overriding header ]=-"
echo

# Use this before your own internal releases. Build server does it automatically.

THINX_FILE=$(find . | grep "/Thinx.h")
THINX_OWNER="test" # todo: override with parameter
THINX_ALIAS="vanilla" # todo: override with parameter
THINX_CLOUD_URL="thinx.cloud"
THINX_MQTT_URL="mqtt://${THINX_CLOUD_URL}"

REPO_NAME='thinx-firmware-esp8266'
VERSION=$(git rev-list HEAD --count)
REPO_VERSION="1.0.${VERSION}"
BUILD_DATE=`date +%Y-%m-%d`

echo "//" > $THINX_FILE
echo "// This is an auto-generated file, it will be re-written by THiNX on cloud build." >> $THINX_FILE
echo "//" >> $THINX_FILE

echo "" >> $THINX_FILE

echo "static const String thinx_commit_id = \""$(git rev-parse HEAD)\"";" >> $THINX_FILE
echo "static const String thinx_owner = \"${THINX_OWNER}\";" >> $THINX_FILE
echo "static const String thinx_alias = \"${THINX_ALIAS}\";" >> $THINX_FILE
echo "static const String thinx_cloud_url = \"${THINX_CLOUD_URL}\";" >> $THINX_FILE
echo "static const String thinx_mqtt_url = \"${THINX_MQTT_URL}\";" >> $THINX_FILE
echo "static const String thinx_firmware_version = \"${REPO_NAME}-${REPO_VERSION}:${BUILD_DATE}\";" >> $THINX_FILE

echo "" >> $THINX_FILE

echo "int thinx_mqtt_port = 1883;" >> $THINX_FILE
