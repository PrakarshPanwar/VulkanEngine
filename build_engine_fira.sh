#!/bin/sh

progress_bar() {
	local TR_TXT=$(echo $1 | awk -F'/' '{print $1, "..", $7}')
	local CURRENT=$2
	local TOTAL=$3

	local FILLED_CHAR="\uEE04"
	local EMPTY_CHAR="\uEE01"
	local BAR_LENGTH=50
	local PERCENT=$((CURRENT * 100 / TOTAL))
	local COMPLETION=$((PERCENT * BAR_LENGTH / 100))

	local BEGIN
	[[ $CURRENT -gt 0 ]] && BEGIN="\uEE03" || BEGIN="\uEE00"

	local i
	local PGR
	for ((i = 0; i < COMPLETION; i++)); do
		PGR+=$FILLED_CHAR
	done

	for ((i = COMPLETION; i < BAR_LENGTH - 1; i++)); do
		PGR+=$EMPTY_CHAR
	done

	local END
	[[ $CURRENT -lt $TOTAL-1 ]] && END="\uEE02" || END="\uEE05"

	printf '\e[0K'
	echo -en "$BEGIN$PGR$END $CURRENT/$TOTAL ($PERCENT%) $TR_TXT\r"
}

# Build Debug Config
/opt/clion/bin/cmake/linux/x64/bin/cmake --build /home/pp2003linux/CLionProjects/VulkanEngine/cmake-build-debug --target all -j 10 | while IFS= read -r line; do
	if [[ $line =~ ^\[([0-9]+)/([0-9]+)\](.+) ]]; then
		CURRENT_NUM="${BASH_REMATCH[1]}"
		TOTAL_NUM="${BASH_REMATCH[2]}"
		BUILD_TXT="${BASH_REMATCH[3]}"

		progress_bar "$BUILD_TXT" "$CURRENT_NUM" "$TOTAL_NUM"
	else
		echo "$line"
	fi
done

echo # Separate Debug and Release

# Build Release Config
/opt/clion/bin/cmake/linux/x64/bin/cmake --build /home/pp2003linux/CLionProjects/VulkanEngine/cmake-build-release --target all -j 10 | while IFS= read -r line; do
	if [[ $line =~ ^\[([0-9]+)/([0-9]+)\](.+) ]]; then
		CURRENT_NUM="${BASH_REMATCH[1]}"
		TOTAL_NUM="${BASH_REMATCH[2]}"
		BUILD_TXT="${BASH_REMATCH[3]}"

		progress_bar "$BUILD_TXT" "$CURRENT_NUM" "$TOTAL_NUM"
	else
		echo "$line"
	fi
done

echo
