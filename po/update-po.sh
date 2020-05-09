#!/bin/bash

sourced=0
if [ -n "$ZSH_EVAL_CONTEXT" ]; then
  case $ZSH_EVAL_CONTEXT in *:file) sourced=1;; esac
elif [ -n "$KSH_VERSION" ]; then
  [ "$(cd $(dirname -- $0) && pwd -P)/$(basename -- $0)" != "$(cd $(dirname -- ${.sh.file}) && pwd -P)/$(basename -- ${.sh.file})" ] && sourced=1
elif [ -n "$BASH_VERSION" ]; then
  (return 0 2>/dev/null) && sourced=1
else # All other shells: examine $0 for known shell binary filenames
  # Detects `sh` and `dash`; add additional shell filenames as needed.
  case ${0##*/} in sh|dash) sourced=1;; esac
fi

# We need to be in the working copy's root directory
if [ $sourced -eq 0 ]; then
	cd "`dirname "$0"`/.."
else
	# If sourced, current directory *must* be the working copy root
	echo "Treating as working copy root: \"$(pwd)\""
fi

echo "LC_ALL=${LC_ALL}"
export LC_ALL=C
export LC_COLLATE=C

find data -name '*.json' -type f '-!' -path 'data/mp/multiplay/maps/*' -exec \
	python3 po/parseJson.py '{}' ';' |
	python3 po/aggregateParsedJson.py > po/custom/fromJson.txt

# Add the comment to the top of the file
cat > po/POTFILES.in << EOF
# List of source files which contain translatable strings.
EOF

find lib src data po -type f |
	grep -e '\.c\(pp\|xx\)\?$' -e 'data.*strings.*\.txt$' -e 'data.*sequenceaudio.*\.tx.$' -e '\.slo$' -e '\.rmsg$' -e 'po/custom/.*\.txt' -e '\.js$' |
	grep -v -e '\.lex\.c\(pp\|xx\)\?$' -e '\.tab\.c\(pp\|xx\)\?$' -e 'lib/netplay/miniupnpc/*' -e 'lib/betawidget/*' -e '_moc\.' -e 'po/custom/files.js' |
	grep -v -e '_lexer\.cpp' -e '_parser\.cpp' |
	sort >> po/POTFILES.in
