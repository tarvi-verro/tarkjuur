
. bot-cfg.sh

function send_file()
{
	curl -s -X POST "https://api.telegram.org/$bot/sendPhoto" \
		-F"chat_id=$chat_id" -F"photo=@$1" >/dev/null
}

function send_msg()
{
	curl -s -X POST "https://api.telegram.org/$bot/sendMessage" \
		-F"chat_id=$chat_id" -F"text=$1" >/dev/null
}


function graph_and_send()
{
	window="$1" ./graph.gnuplot
	send_file "/tmp/_graph200.png"
}

function configure_alarm()
{
	val=$(cut -sd' ' -f2 <<<"$1")

	if [ "$val" == "clear" ]; then
		old=$(< alarm)
		rm alarm && send_msg "Cleared alarm, was $old." \
			|| send_msg "Alarm was not set."
		rm alarm-active 2>/dev/null
		return
	fi

	# Validate value
	if ! [[ "$val" =~ ^[0-9]+(.[0-9]+)?$ ]] || [ $(bc -l <<<"$val > 0 && $val < 3") -ne 1 ]; then
		[ -r alarm ] && [[ "$val" =~ ^[[:space:]]*$ ]] \
			&& send_msg "Current alarm threshold: $(< alarm)" \
			|| send_msg "Invalid argument for alarm, expecting a number between 0 and 3."
		return
	fi

	echo "$val" >alarm
	send_msg "Set alarm at $val."
}

function check_alarm()
{
	if ! [ -r alarm ]; then
		return
	fi

	val=$(<alarm)

	# Get second to last data point (in case latest one is being written)
	f="../data-gathering/measurements-latest"
	ln=$(tail -n2 "$f" | head -n1)
	m=$(cut -d' ' -f4 <<<"$ln" | grep -o '[0-9]\+')

	if [ -r alarm-active ] && [ $(bc -l <<<"$m/1000 < $val - 0.1") -eq 1 ]; then
		alarm_start=$(cut -d' ' -f1 <alarm-active)
		duration=$(bc <<<"($(date +%s) - $alarm_start)/60")
		rm alarm-active && send_msg "Alarm cleared, duration $duration minutes."
	elif [ ! -r alarm-active ] && [ $(bc -l <<<"$m/1000 > $val") -eq 1 ]; then
		echo "$ln" > alarm-active
		send_msg "Plants need more water!"
	fi
}

function show_status()
{
	f="../data-gathering/measurements-latest"
	ln=$(tail -n2 "$f" | head -n1)

	t=$(cut -d' ' -f3<<<"$ln")
	m=$(cut -d' ' -f4<<<"$ln" | grep -o '[0-9]\+')
	send_msg "Temperature: $(bc -l <<<"scale=2; $t/1000")
Moisture measurement: $(bc -l <<<"scale=3; $m/1000")"
}

function process_commands()
{
	case "$1" in
	"/graph hour")
		send_msg "Processing graph.."
		graph_and_send "60*60" &
		;;
	"/graph day")
		send_msg "Processing graph.."
		graph_and_send "60*60*24" &
		;;
	"/graph")
		send_msg "Processing graph.."
		graph_and_send "60*60*24*4" &
		;;
	"/alarm"*)
		configure_alarm "$1"
		;;
	"/status")
		show_status
		;;
	"/"*)
		send_msg "Available commands:
/graph [(hour,day)]
/alarm (threshold or clear)
/status"
		;;
	*)
		;;
	esac

}

offset=0
while true; do
	check_alarm

	sleep 5
	result=$(curl -s -X POST  "https://api.telegram.org/$bot/getUpdates" \
		-F"chat_id=$chat_id"  -F"offset=$offset" -F"timeout=60")
	update_id=$(jq -r ".result[0].update_id" <<<"$result")

	if [ "$update_id" == "null" ]; then
		continue
	fi

	txt=$(jq -r ".result[0].message.text" <<<"$result")
	process_commands "$txt"

	offset=$[ update_id + 1 ]
done

