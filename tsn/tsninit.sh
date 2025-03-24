cd /boot/ && /opt/microchip/dt-overlays/overlay.sh mpfs_coretse.dtbo
 
# Function to simulate progress
simulate_progress() {
  local task_name=$1
  echo -n "$task_name..."
  for i in {1..10}; do
    sleep 0.3
    printf "\r%s [%02d%%]" "$task_name..." $((i * 10))
  done
  echo
}
 
# Simulating configurations
simulate_progress "Configuring CoreTSE"
simulate_progress "Configuring TSN"
simulate_progress "Configuring Mikrobus Controller"
 
# TSN Demo configuration PCP 5 for VLAN 13
ip link add link eth1 name eth1.13 type vlan id 13 egress-qos-map 0:5
ip link set dev eth1.13 up
ip addr add 192.168.13.2/24 dev eth1.13
ifconfig eth1.13 mtu 1400
cd /opt/microchip/japll-pi-controller/ && ./japll-pi > /dev/null 2>&1 &
