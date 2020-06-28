
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
	*)
		;;
	esac

}

offset=0
while true; do
	sleep 5
	result=$(curl -s -X POST  "https://api.telegram.org/$bot/getUpdates" \
		-F"chat_id=$chat_id"  -F"offset=$offset")
	update_id=$(jq -r ".result[0].update_id" <<<"$result")

	if [ "$update_id" == "null" ]; then
		continue
	fi

	txt=$(jq -r ".result[0].message.text" <<<"$result")
	process_commands "$txt"

	offset=$[ update_id + 1 ]
done

