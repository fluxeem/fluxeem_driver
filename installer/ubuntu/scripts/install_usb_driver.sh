#!/bin/bash
# SPDX-FileCopyrightText: 2024-present Fluxeem Technologies Co., Ltd.
# SPDX-License-Identifier: Apache-2.0
#
# 安装 Fluxeem USB 设备的 udev 规则
set -e

echo "正在安装 Fluxeem USB 设备的 udev 规则..."

UDEV_RULES_FILE="/etc/udev/rules.d/fluxeem.rules"

# 设备 ID 与打包的 INF 驱动文件保持一致
DEVICE_IDS=(
    "evk4:04b4:00f5"
    "evk5:04b4:00c4"
    "rdk3:04b4:00f4"
    "dvslume:04b5:0001"
    "apexvision-s1:04b4:0101"
    "sensing:04b4:00c4"
)

if [ "${EUID}" -ne 0 ]; then
    echo "This script must be run as root."
    exit 1
fi

if [ ! -f "${UDEV_RULES_FILE}" ]; then
    touch "${UDEV_RULES_FILE}"
fi

for entry in "${DEVICE_IDS[@]}"; do
    IFS=':' read -r name vid pid <<EOF
${entry}
EOF

    rule_line="SUBSYSTEM==\"usb\", ATTR{idVendor}==\"${vid}\", ATTR{idProduct}==\"${pid}\", MODE=\"0666\""

    if ! grep -Fqs "${rule_line}" "${UDEV_RULES_FILE}"; then
        echo "${rule_line}" >> "${UDEV_RULES_FILE}"
        echo "Added ${name} rule (${vid}:${pid})."
    else
        echo "${name} rule already exists (${vid}:${pid})."
    fi
done

if command -v udevadm >/dev/null 2>&1; then
    udevadm control --reload-rules
    udevadm trigger
    echo "udev rules reloaded."
else
    echo "Warning: udevadm not found; rule reload skipped."
fi

echo "USB driver udev rule installation complete."
