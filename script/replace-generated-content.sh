#!/bin/sh
###########################################################################
#
#  Window Maker window manager
#
#  Copyright (c) 2021 Christophe CURIS
#  Copyright (c) 2021 Window Maker Team
#
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation; either version 2 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License along
#  with this program. If not, see <http://www.gnu.org/licenses/>.
#
###########################################################################
#
# replace-generated-content.sh:
#   Re-generate some list of things and replace it in an existing file
#
# The script can generate these kind of information:
#
#  - a list of man pages (--man-pages), in which case the following fields
# are available for the template replacement:
#     \1: file for the man page, with directories removed, with '.html' extension
#     \2: name of the man page, taken from the file name
#     \3: section of the page, taken from the file name
#     \4: one line description, taken from the man page's NAME section
#
# The 'template' argument provides the template on how to generate one line
# with the new content, the '\n' being replaced with the values explained
# above.
#
# The 'marker' arguments tells the script where the content is to be placed
# in the edited file:
#  - the beginning of generated content is located from the line containing
# the pattern 'start ' + the marker text
#  - the end is marked by 'end ' + marker
# No content outside the start/end is modified. When inserting the content
# the script is re-using the indentation that was found in the line with the
# start marker.
#
###########################################################################
#
# For portability, we stick to the same sh+awk constraint as Autotools to
# limit problems, see for example:
#   http://www.gnu.org/software/autoconf/manual/autoconf-2.69/html_node/Portable-Shell.html
#
###########################################################################

# Report an error on stderr and exit with status 2 to tell make that we could
# not do what we were asked
arg_error() {
    echo "`basename $0`: $@" >&2
    exit 2
}

# print help and exit with success status
print_help() {
    echo "$0: replace lines by new content"
    echo "Usage: $0 options... file"
    echo "valid options are:"
    echo "  --template string   : template text for generating the data"
    echo "  --man-pages list    : content from the list of these man pages"
    echo "  --marker mark       : comment mark surrounding the data to update"
    exit 0
}

# Extract command line arguments
while [ $# -gt 0 ]; do
    case $1 in
        --man-pages)
            shift
            man_pages="$man_pages $1 "
          ;;

        --marker)
            shift
            [ "x$mark" = "x" ] || arg_error "only 1 marker can be specified (option: --marker)"
            mark=$1
          ;;

        --template)
            shift
            [ "x$template" = "x" ] || arg_error "only 1 template can be specified (option: --template)"
            template="$1"
          ;;

        -h|-help|--help) print_help ;;
        -*) arg_error "unknow option '$1'" ;;

        *)
            [ -z "$edit_file" ] || arg_error "only 1 file to modify can be specified"
            edit_file=$1
          ;;
    esac
    shift
done

# Check consistency of command-line
[ -z "$edit_file" ] && arg_error "no file to update was specified"
[ -f "$edit_file" ] || arg_error "file \"$edit_file\" cannot be read"
[ -z "$template" ] && arg_error "Template for generation not specified"
[ -z "$mark" ] && arg_error "No content mark specified"
[ -z "$man_pages" ] && arg_error "No input list provided"

# Make sure the file to edit contains the appropriate marks
grep "start $mark" "$edit_file" > /dev/null || arg_error "Start mark \"start $mark\" not found in \"$edit_file\" (option: --marker)"
grep "end $mark"   "$edit_file" > /dev/null || arg_error "End mark \"end $mark\" not found in \"$edit_file\""

###########################################################################
# Generate the content from the list of man pages provided
###########################################################################
if [ -n "$man_pages" ]; then
    parse_man_page() {
        # Build the name of the HTML file from the man page name
        target=`echo $1 | sed 's,[^/]*/,,g ; s/\.[^.]*$//' `.html
        section=`echo $1 | sed 's/^.*\.\([^.]*\)$/\1/' `

        # Extract the name and the description from the .SH NAME
        sed -n '/^\.SH "*NAME"*$/ {
                   n
                   s/^\([^ ]*\) *\\- */'$target':\1:'$section':/
                 :append_next_line
                   N
                   /\n\./ b end_section_found
                   s/[ \t]*\n[ \t]*/ /
                   b append_next_line
                 :end_section_found
                   s/[ \t]*\n\..*$//
                   s/\\f[BIPR]//g
                   p
                }' $1
    }

    content=`
        for file in $man_pages ; do
            parse_man_page $file
        done
    `
    nb_fields=4
fi

###########################################################################
# Convert the content to the specified format
###########################################################################
for i in `seq 2 $nb_fields`; do
  sed_from="$sed_from\\([^:]*\\):"
done
sed_from="^$sed_from\\(.*\\)\$"

content=`echo "$content" | sed -e "s,$sed_from,$template," `

###########################################################################
# Replace the content in the file
###########################################################################

# We are working in 2 steps because we cannot edit in place
# In step one we remove the existing content from the file
purged_file=`sed -e "/start $mark/ {
          p
        :search_end
          N
          s/^.*\n//
          /end $mark/! b search_end
        }" "$edit_file"
    `

# Convert the content to an AWK array
awk_start=`echo "$content" | awk '
  BEGIN {
    print "BEGIN {";
  }
  {
    # Add a backslash before the double-quotes
    line = "";
    while (1) {
      idx = index($0, "\"");
      if (idx == 0) break;
      line = line substr($0, 1, idx - 1) "\\\\\"";
      $0 = substr($0, idx + 1);
    }
    print "  content[" NR "] = \"" line $0 "\";";
  }
  END {
    print "  nb_content = " NR ";";
    print "}";
  }
' `

# In step two we insert the generated content between the marks
awk_cmd='
# When the start mark is found, insert the generated content
/start '"$mark"'/ {
  print;

  # Isolate the indentation so it can be applied to all generated lines
  sub(/[^ \t].*$/, "");

  for (i = 1; i <= nb_content; i++) {
    print $0 content[i];
  }

  # Thanks to the pre-processing done, the next line will be the end mark
  next;
}

# All the rest of the file is kept as-is
{ print }
'

echo "$purged_file" | awk "$awk_start$awk_cmd" > "$edit_file" || exit $?
