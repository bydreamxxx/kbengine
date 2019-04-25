#!/bin/bash


# 环境变量在Dockerfile中定义
echo KBE_ROOT = \"${KBE_ROOT}\"
echo KBE_RES_PATH = \"${KBE_RES_PATH}\"
echo KBE_BIN_PATH = \"${KBE_BIN_PATH}\"

sleep 10
CLUSTER_CONTROLLER="python ${KBE_ROOT}/kbe/tools/server/pycluster/cluster_controller.py"
${CLUSTER_CONTROLLER} startprocess logger     10000000000001 10001 kbe
${CLUSTER_CONTROLLER} startprocess interfaces 13000000000001 13001 kbe
${CLUSTER_CONTROLLER} startprocess dbmgr      01000000000001 01001 kbe
${CLUSTER_CONTROLLER} startprocess baseappmgr 03000000000001 03001 kbe
${CLUSTER_CONTROLLER} startprocess cellappmgr 04000000000001 04001 kbe
${CLUSTER_CONTROLLER} startprocess baseapp    06000000000001 06001 kbe
${CLUSTER_CONTROLLER} startprocess cellapp    05000000000001 05001 kbe
${CLUSTER_CONTROLLER} startprocess loginapp   02000000000001 02001 kbe

