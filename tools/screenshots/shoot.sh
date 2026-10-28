#!/bin/bash
# Takes the README screenshots with fictional data only: builds ubDMR for the
# desktop, starts tools/demo_server.py as a stand-in BrandMeister master and
# captures each page in an Xvfb screen. Run without network so nothing is
# looked up on radioid.net:
#
#   docker build -t ubdmr-shots tools/screenshots
#   docker run --rm --network none -v "$PWD":/src ubdmr-shots \
#       bash /src/tools/screenshots/shoot.sh
#
# Writes docs/screenshots/*.png.
set -e
SRC=/src; OUT=$SRC/docs/screenshots; HERE=$SRC/tools/screenshots
mkdir -p /work "$OUT" && cd /work
cmake -S $SRC -B . -DCMAKE_BUILD_TYPE=Release >/dev/null && make -j"$(nproc)" 2>&1 | grep -E " error" || true

# The default server name must reach the stand-in.
echo "127.0.0.1 2621.master.brandmeister.network" >> /etc/hosts

# Silent sound card, so the microphone can be opened for the PTT shot.
pulseaudio -D --exit-idle-time=-1 -n --load="module-null-sink sink_name=null" \
    --load="module-native-protocol-unix" 2>/dev/null || true
sleep 1; pactl set-default-source null.monitor 2>/dev/null || true

W=540; H=1080
Xvfb :9 -screen 0 ${W}x${H}x24 >/dev/null 2>&1 & sleep 2; export DISPLAY=:9
tab() { xdotool mousemove $(( W*$1/7 + W/14 )) $((H-20)) click 1; sleep 2; }
shot() { import -window root "$OUT/$1.png"; }

# Lookup cache with fictional stations, so the app shows callsigns offline.
DATA=~/.local/share/ubdmr/ubDMR; mkdir -p $DATA ~/.config/ubdmr
cat > $DATA/operators.json <<'J'
{"2629901":{"callsign":"DL0AAA","fname":"Anna","city":"Musterstadt","country":"Germany"},
 "2629902":{"callsign":"DL0BBB","fname":"Bernd","city":"Beispielhausen","country":"Germany"},
 "2629903":{"callsign":"DL0CCC","fname":"Clara","city":"Testdorf","country":"Germany"},
 "3100001":{"callsign":"N0XYZ","fname":"John","city":"Anytown","country":"United States"}}
J
cat > $DATA/talkgroups.json <<'J'
{"91":"Worldwide","92":"Europe","262":"Germany","2629":"Saxony","9990":"Parrot"}
J

NOW=$(date +%s)000
CONTACTS='[{"id":2629901,"callsign":"DL0AAA","name":"Anna, Musterstadt, Germany"},{"id":2629902,"callsign":"DL0BBB","name":"Bernd, Beispielhausen, Germany"},{"id":3100001,"callsign":"N0XYZ","name":"John, Anytown, United States"}]'
MSGS='[{"key":"p2629901","peer":2629901,"group":false,"outgoing":false,"from":2629901,"text":"Hi N0CALL, how is my signal on TG 262? 73","time":'$((NOW-900000))',"status":""},{"key":"p2629901","peer":2629901,"group":false,"outgoing":true,"from":0,"text":"Loud and clear, Anna. 73 de N0CALL","time":'$((NOW-840000))',"status":"delivered"},{"key":"g262","peer":262,"group":true,"outgoing":false,"from":2629902,"text":"CQ CQ on TG 262","time":'$((NOW-120000))',"status":""}]'
esc() { echo "$1" | sed 's/\\/\\\\/g; s/"/\\"/g'; }

run() {   # $1 = theme
  rm -f /tmp/demo_quiet; pkill -f demo_server.py 2>/dev/null || true; pkill ubdmr 2>/dev/null || true; sleep 1
  printf '[General]\nautoConnect=true\nhost=2621.master.brandmeister.network\nport=54006\ndmrId=1234567\npassword=demo\ncallsign=N0CALL\nvocoder=builtin\ntalkgroup=262\nthemeMode=%s\ncontacts="%s"\nmessages="%s"\nunreadMessages="{\\"g262\\":1}"\n' \
    "$1" "$(esc "$CONTACTS")" "$(esc "$MSGS")" > ~/.config/ubdmr/ubDMR.conf
  python3 $HERE/demo_server.py 400 &
  LANG=en_US.UTF-8 GRID_UNIT_PX=9 QT_QPA_PLATFORM=xcb ./ubdmr > /work/app_$1.log 2>&1 &
  sleep 22   # login, three short calls, one private call, then the long call
}

run light
tab 0; shot radio
tab 1; shot talkgroups
tab 5; shot heard
tab 4; shot contacts
tab 3; shot chats
xdotool mousemove 270 ${CHAT_ROW_Y:-160} click 1; sleep 2; shot chat
tab 6; shot settings
tab 2; shot echo_idle
if [ -n "$PTT_Y" ]; then
  touch /tmp/demo_quiet; sleep 4      # nobody else on air while transmitting
  xdotool mousemove 270 "$PTT_Y" mousedown 1; sleep 30; shot echo; xdotool mouseup 1; sleep 1
fi

run dark
tab 0; shot radio_dark
tab 3; xdotool mousemove 270 ${CHAT_ROW_Y:-160} click 1; sleep 2; shot chat_dark
pkill ubdmr || true

cd "$OUT"
for f in *.png; do mogrify -strip "$f"; done
ls -la "$OUT"
grep -a -E "Radio|Recorder|Rewind" /work/app_light.log | tail -25
